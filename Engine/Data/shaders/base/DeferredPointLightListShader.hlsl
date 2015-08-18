//------------------------------------------------------------------------------------------------------------
// DeferredPointLightListShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"
#include "DeferredCommon.hlsl"

// Defines
#define MAX_LIGHTS (256)

// Input variables.
struct PointLight
{
    float3 Position;
    float Range;
    float3 Color;
    float FalloffExponent;
};
PointLight LightParameters[MAX_LIGHTS];

void VSMain(in float3 in_position : POSITION,
            in uint in_instanceID : SV_InstanceID,
            out float4 out_positionVS : VIEWPOSITION,
            out nointerpolation float3 out_lightPositionVS : VIEWLIGHTPOSITION,
            out nointerpolation float out_inverseLightRange : INVERSELIGHTRANGE,
            out nointerpolation uint out_lightIndex : LIGHTINDEX,
            out float4 out_positionCS : SV_Position)
{
    // copy parameters
    out_lightPositionVS = mul(ViewConstants.ViewMatrix, float4(LightParameters[in_instanceID].Position, 1.0f)).xyz;
    out_inverseLightRange = 1.0f / LightParameters[in_instanceID].Range;
    out_lightIndex = in_instanceID;
    
    // multiply and pass-through
    float4 worldPosition = float4(in_position * LightParameters[in_instanceID].Range + LightParameters[in_instanceID].Position, 1.0f);
    out_positionVS = mul(ViewConstants.ViewMatrix, worldPosition);
    out_positionCS = mul(ViewConstants.ViewProjectionMatrix, worldPosition);
}

void PSMain(in float4 in_positionVS : VIEWPOSITION,
            in nointerpolation float3 in_lightPositionVS : VIEWLIGHTPOSITION,
            in nointerpolation float in_inverseLightRange : INVERSELIGHTRANGE,
            in nointerpolation uint in_lightIndex : LIGHTINDEX,
            in float4 screenPosition : SV_Position,
            out float4 out_target : SV_Target)
{
    // construct view ray
    float3 viewRay = float3(in_positionVS.xy / in_positionVS.z, 1.0f);
    
    // load gbuffer data
    GBufferData surfaceData;
    GetGBufferDataFromViewRay(viewRay, screenPosition.xy, surfaceData);
        
    // calculate light vectors
    float3 lightVectorF = surfaceData.ViewSpacePosition - in_lightPositionVS;
    float lightDistance = length(lightVectorF);
    float3 lightVector = normalize(lightVectorF);
    float3 cameraVector = normalize(surfaceData.ViewSpacePosition);
    
    // calculate point light falloff
    float lightAmount = pow(saturate(1.0f - lightDistance * in_inverseLightRange), LightParameters[in_lightIndex].FalloffExponent);
    
    // calculate lighting
    float3 sceneColor = CalculateDeferredLighting(surfaceData, lightVector, LightParameters[in_lightIndex].Color * lightAmount, cameraVector);
    out_target = float4(sceneColor, 0.0f);
}
