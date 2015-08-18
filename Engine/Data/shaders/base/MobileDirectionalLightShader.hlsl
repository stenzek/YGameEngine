//------------------------------------------------------------------------------------------------------------
// DirectionalLightShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------

// From the material, we need the tangent normal, diffuse, and specular colors, as well as specular exponent.
#define MATERIAL_NEEDS_BASE_COLOR 1
#define MATERIAL_NEEDS_LIGHT_REFLECTION 1
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

// skew value
static const float SHADOW_MAP_EPSILON_VALUE = 0.001;

// Input variables.
BEGIN_CONSTANT_BUFFER(MobileDirectionalLightParametersBuffer, MobileDirectionalLightParameters)
{
    float3 LightVector;
    float3 LightColor;
    float3 AmbientColor;
    float4 ShadowMapSize;                       // xy = width, height, zw = 1.0 / width, 1.0 / height
    float4x4 ShadowMapViewProjectionMatrix;
}
END_CONSTANT_BUFFER(MobileDirectionalLightParametersBuffer, MobileDirectionalLightParameters)

// Shadow map
#if WITH_SHADOW_MAP

Texture2D ShadowMapTexture;
SamplerState ShadowMapTexture_SamplerState;

float FindShadow(float3 worldPosition)
{
    float4 lightSpacePosition = mul(MobileDirectionalLightParameters.ShadowMapViewProjectionMatrix, float4(worldPosition, 1));
    lightSpacePosition.xyz /= lightSpacePosition.w;
    
    // fix depth range
#ifdef GLSL_VERSION
    lightSpacePosition.z = (lightSpacePosition.z * 0.5f) + 0.5f;
#endif
    
    // to shadow space
    float2 shadowMapTextureCoordinates = 0.5 * lightSpacePosition.xy + float2(0.5, 0.5);
#ifndef GLSL_VERSION
    shadowMapTextureCoordinates.y = 1.0 - shadowMapTextureCoordinates.y;
#endif

    // get comparison value
    float shadowComparisonValue = lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE;
    float shadowContribution;
    
// >1x1 path loops, 1x1 path doesn't
#if SHADOW_FILTER_3X3
    // removed array since GLSL ES 1.0 doesn't support constant arrays
    const int NOFFSETS = 3 * 3;
    
    // fetch and weight 4 samples in parallel
    float4 sampleValues;
    sampleValues.x = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(-1.0f, -1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.y = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(0.0f, -1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.z = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(1.0f, -1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.w = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(-1.0f, 0.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    shadowContribution = dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
    
    sampleValues.x = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(0.0f, 0.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.y = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(1.0f, 0.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.z = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(-1.0f, 1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    sampleValues.w = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(0.0f, 1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r;
    shadowContribution += dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
    shadowContribution += (ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, float2(1.0f, 1.0f) * MobileDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates).r >= shadowComparisonValue);
    shadowContribution /= (float)NOFFSETS;
    
#else       // SHADOW_FILTER_1X1
    float shadowValue = ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, shadowMapTextureCoordinates).r;
    shadowContribution = (shadowComparisonValue < shadowValue);
    //shadowContribution = (shadowComparisonValue < (ShadowMapTexture.Sample(ShadowMapTexture_SamplerState, shadowMapTextureCoordinates).r));
    
#endif

    return shadowContribution;
}

#endif      // WITH_SHADOW_MAP

// VS to PS interpolants
struct VSToPSParameters
{
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    float3 TangentLightVector   : TEXCOORD4;
    float3 TangentCameraVector  : TEXCOORD5;
#endif

#if WITH_SHADOW_MAP || MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE
    float3 WorldPosition : TEXCOORD6;
#endif
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
            
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    // Retreive tangent basis vectors.
    float3x3 tangentBasis = VertexFactoryGetTangentBasis(in_input);
    
    // Calculate light and camera vectors (world space)
    float3 cameraVector = ViewConstants.EyePosition - worldPosition;
	
    // Transform the light/camera vectors to tangent space, and store in output.
    out_parameters.TangentLightVector = VertexFactoryTransformWorldToTangentSpace(tangentBasis, MobileDirectionalLightParameters.LightVector);
    out_parameters.TangentCameraVector = VertexFactoryTransformWorldToTangentSpace(tangentBasis, cameraVector);
#endif

#if WITH_SHADOW_MAP || MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE
    out_parameters.WorldPosition = worldPosition;
#endif
	
	// Write parameters
    out_interpolants = GetMaterialPSInterpolantsVS(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(in VSToPSParameters in_parameters,
			in MaterialPSInterpolants in_interpolants,
            out float4 out_target : SV_Target)
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    // Determine the direction and luminosity of the light at this pixel.
    float3 lightDirection = normalize(in_parameters.TangentLightVector);
    float3 viewDirection = normalize(in_parameters.TangentCameraVector);
#else
	// Determine the direction and luminosity of the light at this pixel.
    float3 lightDirection = MobileDirectionalLightParameters.LightVector;
    float3 viewDirection = normalize(ViewConstants.EyePosition - in_parameters.WorldPosition);
#endif
    
    // get light coefficient
    float lightCoefficient;

    // Shadows
#if WITH_SHADOW_MAP
    lightCoefficient = FindShadow(in_parameters.WorldPosition);
#else
    lightCoefficient = 1.0f;
#endif

    // Calculate the lighting results, and return them.
    float3 sceneColor = MaterialGetLightReflection(MPIParameters, lightDirection, MobileDirectionalLightParameters.LightColor, lightCoefficient, viewDirection);
    
    // Add in ambient contribution
    sceneColor += MaterialGetBaseColor(MPIParameters) * MobileDirectionalLightParameters.AmbientColor;

    // Calculate output colour    
    out_target = MaterialCalcOutputColor(MPIParameters, sceneColor);
}
