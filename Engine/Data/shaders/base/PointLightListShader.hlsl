//------------------------------------------------------------------------------------------------------------
// PointLightListShader.hlsl
// Shader for rendering an object with lighting contributions from a point light.
//------------------------------------------------------------------------------------------------------------
#define MATERIAL_NEEDS_WORLD_POSITION 1
#define MATERIAL_NEEDS_WORLD_NORMAL 1
#define MATERIAL_NEEDS_LIGHT_REFLECTION 1
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

// Maximum number of lights in a single pass
#define MAX_LIGHTS (8)
struct PointLight
{
    float3 Position;
    float InverseRange;
    float3 Color;
    float FalloffExponent;
};

// Input variables.

cbuffer PointLightListParameters : register(b3) { struct
{
    PointLight Lights[MAX_LIGHTS];
    uint ActiveLightCount;
} cbPointLightListParameters; }

// VS to PS interpolants
struct VSToPSParameters
{
    float3 CameraVector  : CameraVector;
};

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
			out VSToPSParameters out_parameters,
            out MaterialPSInterpolants out_interpolants,
			out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);

#if MATERIAL_FEATURE_LEVEL >= FEATURE_LEVEL_ES3
    MaterialVSInputParameters MVIParameters = GetMaterialVSInputParametersFromVertexFactory(in_input);
    worldPosition += MaterialGetWorldPositionOffset(MVIParameters);
#endif
	
	// Calculate camera vector (world space)
    float3 worldCameraVector = ViewConstants.EyePosition - worldPosition;
	out_parameters.CameraVector = worldCameraVector; 
    
    out_interpolants = GetMaterialPSInterpolantsVS(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));    
}

void PSMain(in VSToPSParameters in_parameters,
			in MaterialPSInterpolants in_interpolants,            
            out float4 out_target : SV_Target)
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
    // Get world-space position and normal of the pixel
    float3 worldPosition = MaterialGetWorldPosition(MPIParameters);
    float3 worldNormal = MaterialGetWorldNormal(MPIParameters);
    float3 baseColor = MaterialGetBaseColor(MPIParameters);
    float specularCoefficient = MaterialGetSpecularCoefficient(MPIParameters);
    float specularExponent = MaterialGetSpecularExponent(MPIParameters);
    
    // Iterate over lights
    float3 litDiffuse = float3(0.0f, 0.0f, 0.0f);
    float3 litSpecular = float3(0.0f, 0.0f, 0.0f);
    float3 viewVector = normalize(in_parameters.CameraVector);
    [loop] for (uint i = 0; i < cbPointLightListParameters.ActiveLightCount; i++)
    {
        float3 lightToPosition = worldPosition - cbPointLightListParameters.Lights[i].Position;
        float3 lightColor = cbPointLightListParameters.Lights[i].Color;
        float3 lightVector = normalize(lightToPosition);
        float lightDistance = length(lightToPosition);
        float lightFalloff = pow(saturate(1.0f - lightDistance * cbPointLightListParameters.Lights[i].InverseRange), cbPointLightListParameters.Lights[i].FalloffExponent);
        
        litDiffuse += CalculateLambertianDiffuse(worldNormal, lightVector) * lightFalloff * lightColor;
        #if MATERIAL_LIGHTING_MODEL_PHONG
            litSpecular += CalculatePhongSpecular(worldNormal, specularExponent, lightVector, viewVector) * lightFalloff * lightColor;        
        #else
            litSpecular += CalculateBlinnPhongSpecular(worldNormal, specularExponent, lightVector, viewVector) * lightFalloff * lightColor;        
        #endif
    }
    
    // Calculate final colour
    float3 finalColor = baseColor * ((litSpecular * specularCoefficient) + litDiffuse);
    out_target = MaterialCalcOutputColor(MPIParameters, finalColor);
}
