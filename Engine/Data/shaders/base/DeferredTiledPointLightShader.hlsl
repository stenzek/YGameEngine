//------------------------------------------------------------------------------------------------------------
// DeferredTiledPointLightShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

// Defines, must match .cpp
#define MAX_LIGHTS_PER_DISPATCH (1024)
#define TILE_SIZE (32)

// Input parameters
uint TileCountX;
uint TileCountY;

// Input buffers
Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;
Texture2D<float4> GBuffer0;
SamplerState GBuffer0_SamplerState;
Texture2D<float4> GBuffer1;
SamplerState GBuffer1_SamplerState;
Texture2D<float4> GBuffer2;
SamplerState GBuffer2_SamplerState;

// Output buffers
RWTexture2D<float4> LightBuffer;

// Light information
struct Light
{
    float3 Position;
    float InverseRange;
    float3 Color;
    float FalloffExponent;
};
cbuffer DeferredTiledPointLightShaderLightBuffer { Light LightParameters[MAX_LIGHTS_PER_DISPATCH]; };
uint ActiveLightCount;

// Per-group shared memory
groupshared uint TileLightIndices[MAX_LIGHTS_PER_DISPATCH];
groupshared uint TileLightCount;

// convert pixel coordinates to view-space coordinates
float3 UnprojectPixelCoordinatesToViewSpace(uint2 pixelCoordinates, float z, uint2 targetDimensions)
{
    float4 clipSpace;
    clipSpace.x = 2.0f * (float)pixelCoordinates.x / (float)targetDimensions.x - 1.0f;
    clipSpace.y = 1.0f - 2.0f * (float)pixelCoordinates.y / (float)targetDimensions.y;
    clipSpace.z = z;
    clipSpace.w = 1.0f;
    
    float4 viewSpace = mul(ViewConstants.InverseProjectionMatrix, clipSpace);
    return viewSpace.xyz / viewSpace.w;
}

// bounding-box
struct BoundingBox
{
    float3 Min;
    float3 Max;
};

BoundingBox GetTileBoundingBoxVS(uint3 groupID, uint2 targetDimensions)
{
    float3 coords;
    BoundingBox aabb;
    
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy, 0.0f, targetDimensions);
    aabb.Min = coords; aabb.Max = coords;
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(TILE_SIZE, 0), 0.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(0, TILE_SIZE), 0.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(TILE_SIZE, TILE_SIZE), 0.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy, 1.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(TILE_SIZE, 0), 1.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(0, TILE_SIZE), 1.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    coords = UnprojectPixelCoordinatesToViewSpace(groupID.xy + uint2(TILE_SIZE, TILE_SIZE), 1.0f, targetDimensions);
    aabb.Min = min(aabb.Min, coords); aabb.Max = max(aabb.Max, coords);
    
    return aabb;
}    

// cull a light
groupshared BoundingBox TileBoundingBox;
bool CullLight(uint lightIndex)
{
    float3 lightPos = LightParameters[lightIndex].Position;
    float lightRange = 1.0f / LightParameters[lightIndex].InverseRange;
    
    float3 M = TileBoundingBox.Min;
    float3 N = TileBoundingBox.Max;
    float3 O = lightPos - lightRange;
    float3 P = lightPos + lightRange;
    
    if (any(M > P) || any(O > P))
        return false;
    else
        return true;
}

// Entry point
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 groupID : SV_GroupID,
          uint3 groupThreadID : SV_GroupThreadID,
          uint3 dispatchThreadID : SV_DispatchThreadID,
          uint groupIndex : SV_GroupIndex)
{
    // retrieve light buffer dimensions, quicker than reading from constant memory
    uint2 targetDimensions;
    LightBuffer.GetDimensions(targetDimensions.x, targetDimensions.y);
    
    // calculate the pixel coordinates that is being shaded by this thread
    uint2 pixelCoordinates = uint2(dispatchThreadID.xy);
    uint3 texelCoordinates = uint3(pixelCoordinates, 0);
    
    // initialize the per-group shared variables
    if (groupIndex == 0)
    {
        TileBoundingBox = GetTileBoundingBoxVS(groupID, targetDimensions);
        TileLightCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    // cull lights
    for (uint lightIndex = groupIndex; lightIndex < ActiveLightCount; lightIndex += (TILE_SIZE * TILE_SIZE))
    {
        // todo: cull the light
        if (CullLight(lightIndex))
        {
            uint freeIndex;
            InterlockedAdd(TileLightCount, 1, freeIndex);
            TileLightIndices[freeIndex] = lightIndex;
        }
    }
    
    // sync threads
    GroupMemoryBarrierWithGroupSync();
    
    // only process pixels that are in screen-range
    if (any(pixelCoordinates >= targetDimensions))
        return;
        
#if 1
    // visualize the amount of lights per-tile
    float color = ((float)TileLightCount / (float)ActiveLightCount);
    LightBuffer[pixelCoordinates] = float4(color, color, color, 0.0f);
#else
    // load the required data from the gbuffer
    float DepthBufferValue = DepthBuffer.Load(texelCoordinates);
    float4 GBuffer0Value = GBuffer0.Load(texelCoordinates);
    float4 GBuffer1Value = GBuffer1.Load(texelCoordinates);
    float4 GBuffer2Value = GBuffer2.Load(texelCoordinates);
    
    // extract parameters from gbuffer
    float3 normalVS = UnpackFromColorRange3(GBuffer1Value.xyz);
    float3 baseColor = GBuffer0Value.rgb;
    float specularCoefficient = GBuffer0Value.a;
    float specularExponent = GBuffer1Value.a;
    
    // foo
    //LightBuffer[pixelCoordinates] = float4(PackToColorRange3(normalVS) * 0.2f, 0.0f);
    //if (all(pixelCoordinates > (targetDimensions / 2)))
        //LightBuffer[pixelCoordinates] = float4(0.2f, 0.5f, 0.2f, 1.0f);
    //else
        //LightBuffer[pixelCoordinates] = float4(0.5f, 0.2f, 0.8f, 1.0f);
#endif
}
