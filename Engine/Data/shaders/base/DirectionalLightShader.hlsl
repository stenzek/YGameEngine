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

// Maximum number of cascades
#define CASCADE_COUNT (3)

// skew value
static const float SHADOW_MAP_EPSILON_VALUE = 0.001;
static const float3 SHADOW_MAP_SHOW_CASCADE_COLORS[CASCADE_COUNT] = { float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) };

// Input variables.
cbuffer DirectionalLightParameters { struct
{
    float3 LightVector;
    float3 LightColor;
    float3 AmbientColor;
    float4 ShadowMapSize;                       // xy = width, height, zw = 1.0 / width, 1.0 / height
    float4x4 CascadeViewProjectionMatrix[CASCADE_COUNT];
    float CascadeSplitDepths[CASCADE_COUNT];
} cbDirectionalLightParameters; }

// Shadow map
#if WITH_SHADOW_MAP

#include "ShadowMapFilters.hlsl"

Texture2DArray ShadowMapTexture;// : register(t8);
#if USE_HARDWARE_PCF
	SamplerComparisonState ShadowMapTexture_SamplerState;// : register(s7);
#else
	SamplerState ShadowMapTexture_SamplerState;// : register(s7);
#endif

float SelectCascadeIndex(float3 worldPosition)
{
    float eyeSpaceDepth = length(worldPosition - ViewConstants.EyePosition);
    float shadowIndex = 0;
    
    [unroll]
    for (int i = 0; i < CASCADE_COUNT; i++)
        shadowIndex += (eyeSpaceDepth > cbDirectionalLightParameters.CascadeSplitDepths[i]);
        
    return shadowIndex;
}

float FindShadow(float3 worldPosition)
{
    // select cascade index
    float cascadeIndex = SelectCascadeIndex(worldPosition);
    
    float4 lightSpacePosition = mul(cbDirectionalLightParameters.CascadeViewProjectionMatrix[cascadeIndex], float4(worldPosition, 1));
    lightSpacePosition.xyz /= lightSpacePosition.w;
    
    // fix depth range
#ifdef __OPENGL__
    lightSpacePosition.z = (lightSpacePosition.z * 0.5f) + 0.5f;
#endif
    
    // to shadow space
    float2 shadowMapTextureCoordinates = 0.5 * lightSpacePosition.xy + float2(0.5, 0.5);
#ifndef __OPENGL__
    shadowMapTextureCoordinates.y = 1.0 - shadowMapTextureCoordinates.y;
#endif
    
    // get comparison value
    float shadowComparisonValue = lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE;
    float shadowContribution;
    
    // depending on filter...
#if SHADOW_FILTER_3X3
    const int NOFFSETS = 3 * 3;
    const float2 offsets[NOFFSETS] = {
        float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f),
        float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f),
        float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f)
    };

#elif SHADOW_FILTER_5X5
    const int NOFFSETS = 5 * 5;
    const float2 offsets[NOFFSETS] = {
        float2(-2.0f, -2.0f), float2(-1.0f, -2.0f), float2(0.0f, -2.0f), float2(1.0f, -2.0f), float2(2.0f, -2.0f),
        float2(-2.0f, -1.0f), float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f), float2(2.0f, -1.0f),
        float2(-2.0f, 0.0f), float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(2.0f, 0.0f),
        float2(-2.0f, 1.0f), float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(2.0f, 1.0f),
        float2(-2.0f, 2.0f), float2(-1.0f, 2.0f), float2(0.0f, 2.0f), float2(1.0f, 2.0f), float2(2.0f, 2.0f)
    };
	
#endif

	// using hardware pcf?
#if USE_HARDWARE_PCF
    // >1x1 path loops, 1x1 path doesn't
	#if !SHADOW_FILTER_1X1
        shadowContribution = 0.0f;
        [loop]
        for (int i = 0; i < NOFFSETS; i++)
        {
            float3 sampleCoordinates = float3(offsets[i] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex);
            shadowContribution += ShadowMapTexture.SampleCmpLevelZero(ShadowMapTexture_SamplerState, sampleCoordinates, shadowComparisonValue);
        }
		shadowContribution /= (float)NOFFSETS;
	#else
		shadowContribution = ShadowMapTexture.SampleCmpLevelZero(ShadowMapTexture_SamplerState, float3(shadowMapTextureCoordinates, cascadeIndex), shadowComparisonValue);
    #endif
#else
    // >1x1 path loops, 1x1 path doesn't
	#if SHADOW_FILTER_5X5
        shadowContribution = 0.0f;
        [loop]
        for (int i = 0; i < NOFFSETS; i++)
        {
            float3 sampleCoordinates = float3(offsets[i] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex);
            shadowContribution += (shadowComparisonValue < ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, sampleCoordinates, 0.0f).r);
        }
        shadowContribution /= (float)NOFFSETS;
        
    #elif SHADOW_FILTER_3X3
        // fetch and weight 4 samples in parallel
        float4 sampleValues;
        sampleValues.x = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[0] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.y = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[1] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.z = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[2] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.w = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[3] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        shadowContribution = dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
        
        sampleValues.x = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[4] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.y = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[5] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.z = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[6] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.w = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[7] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        shadowContribution += dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
        shadowContribution += (ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[8] * cbDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r >= shadowComparisonValue);
        shadowContribution /= (float)NOFFSETS;
        
    #else       // SHADOW_FILTER_1X1
        shadowContribution = (shadowComparisonValue < (ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(shadowMapTextureCoordinates, cascadeIndex), 0.0f).r));
    #endif
#endif      // USE_HARDWARE_PCF
          
    // outside depth range?
    shadowContribution = (lightSpacePosition.z >= 1.0) ? 1.0 : shadowContribution;
    
    // return
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
    out_parameters.TangentLightVector = VertexFactoryTransformWorldToTangentSpace(tangentBasis, cbDirectionalLightParameters.LightVector);
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
    float3 lightDirection = cbDirectionalLightParameters.LightVector;
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
    float3 sceneColor = MaterialGetLightReflection(MPIParameters, lightDirection, cbDirectionalLightParameters.LightColor, lightCoefficient, viewDirection);
    
    // Add in ambient contribution
    sceneColor += MaterialGetBaseColor(MPIParameters) * cbDirectionalLightParameters.AmbientColor;
    
#if WITH_SHADOW_MAP && SHOW_CASCADES
    sceneColor *= SHADOW_MAP_SHOW_CASCADE_COLORS[SelectCascadeIndex(in_parameters.WorldPosition)];
#endif

    // Calculate output colour    
    out_target = MaterialCalcOutputColor(MPIParameters, sceneColor);
}
