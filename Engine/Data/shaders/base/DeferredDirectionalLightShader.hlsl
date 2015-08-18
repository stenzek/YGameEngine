//------------------------------------------------------------------------------------------------------------
// DeferredDirectionalLightShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"
#include "DeferredCommon.hlsl"

// Maximum number of cascades
#define CASCADE_COUNT (3)

// skew value
static const float SHADOW_MAP_EPSILON_VALUE = 0.001;
static const float3 SHADOW_MAP_SHOW_CASCADE_COLORS[CASCADE_COUNT] = { float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) };

// Input variables.
cbuffer DeferredDirectionalLightParameters { struct
{
    float3 LightVector;
    float3 LightColor;
    float3 AmbientColor;
    float4 ShadowMapSize;                       // xy = width, height, zw = 1.0 / width, 1.0 / height
    float4x4 CascadeViewProjectionMatrix[CASCADE_COUNT];
    float CascadeSplitDepths[CASCADE_COUNT];
} cbDeferredDirectionalLightParameters; }

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
        shadowIndex += (eyeSpaceDepth > cbDeferredDirectionalLightParameters.CascadeSplitDepths[i]);
        
    return shadowIndex;
}

float FindShadow(float3 worldPosition)
{
    // select cascade index
    float cascadeIndex = SelectCascadeIndex(worldPosition);
    
    float4 lightSpacePosition = mul(cbDeferredDirectionalLightParameters.CascadeViewProjectionMatrix[cascadeIndex], float4(worldPosition, 1));
    lightSpacePosition.xyz /= lightSpacePosition.w;
    
    // to shadow space
    float2 shadowMapTextureCoordinates = lightSpacePosition.xy * 0.5f + float2(0.5, 0.5);
    shadowMapTextureCoordinates.y = 1.0 - shadowMapTextureCoordinates.y;
    
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
            float3 sampleCoordinates = float3(offsets[i] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex);
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
            float3 sampleCoordinates = float3(offsets[i] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex);
            shadowContribution += (shadowComparisonValue < ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[0] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r);
        }
        shadowContribution /= (float)NOFFSETS;
        
    #elif SHADOW_FILTER_3X3
        // fetch and weight 4 samples in parallel
        float4 sampleValues;
        sampleValues.x = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[0] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.y = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[1] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.z = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[2] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.w = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[3] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        shadowContribution = dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
        
        sampleValues.x = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[4] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.y = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[5] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.z = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[6] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        sampleValues.w = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[7] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r;
        shadowContribution += dot((sampleValues >= shadowComparisonValue), float4(1.0f, 1.0f, 1.0f, 1.0f));
        shadowContribution += (ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, float3(offsets[8] * cbDeferredDirectionalLightParameters.ShadowMapSize.zw + shadowMapTextureCoordinates, cascadeIndex), 0.0f).r >= shadowComparisonValue);
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

void PSMain(in float2 screenTexCoord : TEXCOORD0,
            in float3 in_viewRay : VIEWRAY,
            in float4 screenPosition : SV_Position,
            out float4 out_target : SV_Target)
{   
    // load gbuffer data
    GBufferData surfaceData;
    GetGBufferDataFromViewRay(in_viewRay, screenPosition.xy, surfaceData);
    
    // don't bother sampling anything that's not shaded
    if (!any(surfaceData.BaseColor))
        discard;
    
    // calculate camera vector
    float3 cameraVector = normalize(surfaceData.ViewSpacePosition);

    // calculate scene color
    float3 sceneColor;
    
    // ambient lighting
    sceneColor = surfaceData.BaseColor * cbDeferredDirectionalLightParameters.AmbientColor;
    
    // get shadow term
    float shadowTerm = 1.0f;
#if WITH_SHADOW_MAP
    float3 positionWS = mul(ViewConstants.InverseViewMatrix, float4(surfaceData.ViewSpacePosition, 1.0f)).xyz;
    //[branch] if (surfaceData.ShadowMask > 0.0f)
        shadowTerm = FindShadow(positionWS);
#endif
    
    // calculate directional lighting
    sceneColor += CalculateDeferredLighting(surfaceData, cbDeferredDirectionalLightParameters.LightVector, cbDeferredDirectionalLightParameters.LightColor, cameraVector) * shadowTerm;
    
    // cascade debug info
#if WITH_SHADOW_MAP && SHOW_CASCADES
    sceneColor *= SHADOW_MAP_SHOW_CASCADE_COLORS[SelectCascadeIndex(positionWS)];
#endif

    // return colour
    out_target = float4(sceneColor, 0.0f);
}


#if 0

Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;
Texture2D<float4> GBuffer0;
SamplerState GBuffer0_SamplerState;
Texture2D<float4> GBuffer1;
SamplerState GBuffer1_SamplerState;
Texture2D<float4> GBuffer2;
SamplerState GBuffer2_SamplerState;

void PSMain(in float2 screenTexCoord : TEXCOORD0,
            in float3 in_viewRay : VIEWRAY,
            //in float4 screenPosition : SV_Position,
            out float4 out_target : SV_Target)
{
    // sample the gbuffers
    float DepthBufferValue = DepthBuffer.SampleLevel(DepthBuffer_SamplerState, screenTexCoord, 0.0f);
    float4 GBuffer0Value = GBuffer0.SampleLevel(GBuffer0_SamplerState, screenTexCoord, 0.0f);
    float4 GBuffer1Value = GBuffer1.SampleLevel(GBuffer1_SamplerState, screenTexCoord, 0.0f);
    float4 GBuffer2Value = GBuffer2.SampleLevel(GBuffer2_SamplerState, screenTexCoord, 0.0f);
    //int3 texelCoordinates = int3(int2(screenPosition.xy), 0);
    //float DepthBufferValue = DepthBuffer.Load(texelCoordinates);
    //float4 GBuffer0Value = GBuffer0.Load(texelCoordinates);
    //float4 GBuffer1Value = GBuffer1.Load(texelCoordinates);
    //float4 GBuffer2Value = GBuffer2.Load(texelCoordinates);
    
    // reconstruct the world-space position of the pixel
    //float4 screenPositionW = float4(screenTexCoord.x * 2.0f - 1.0f, (1.0f - screenTexCoord.y) * 2.0f - 1.0f, DepthBufferValue, 1.0f);
    //float4 worldPositionW = mul(ViewConstants.InverseViewProjectionMatrix, screenPositionW);
    //float3 worldPosition = worldPositionW.xyz / worldPositionW.w;
    //float3 viewSpacePosition = in_viewRay * LinearizeDepth(DepthBufferValue);
    //float3 worldPosition = mul(ViewConstants.InverseViewMatrix, float4(viewSpacePosition, 1.0f)).xyz;
    //float3 viewRay = normalize(in_viewRay);
    //float viewZDist = dot(float3(ViewConstants.ViewMatrix[0][2], ViewConstants.ViewMatrix[1][2], ViewConstants.ViewMatrix[2][2]), viewRay);
    //float viewZDist = dot(float3(ViewConstants.ViewMatrix[2].xyz), viewRay);
    //float3 worldPosition = ViewConstants.EyePosition + viewRay * (LinearizeDepth(DepthBufferValue) / viewZDist);    
    //float3 viewSpacePosition = in_viewRay * LinearizeDepth(DepthBufferValue);
    //float3 worldPosition = mul(ViewConstants.InverseViewMatrix, float4(viewSpacePosition, 1.0f)).xyz;
    //float viewSpaceZ = ViewConstants.ProjectionMatrix._34 / (DepthBufferValue - ViewConstants.ProjectionMatrix._33);
    //float3 viewSpacePosition = in_viewRay * viewSpaceZ;
    //float3 worldPosition = ReconstructWorldSpacePosition(screenTexCoord, DepthBufferValue);
    //float3 cameraVector = normalize(ViewConstants.EyePosition - worldPosition);
    
    //float3 positionVS = ReconstructViewSpacePosition(screenTexCoord, DepthBufferValue);
    //float3 cameraVector = normalize(positionVS);
    
    float3 positionVS = in_viewRay * LinearizeDepth(DepthBufferValue);
    float3 cameraVector = normalize(positionVS);
    
    // extract parameters from gbuffer
    float3 worldNormal = UnpackFromColorRange3(GBuffer1Value.xyz);
    float3 baseColor = GBuffer0Value.rgb;
    float specularCoefficient = GBuffer0Value.a;
    float specularExponent = GBuffer1Value.a;
    
    // todo: material modes..
    // calculate ambient amount
    float3 ambientColor = baseColor * cbDeferredDirectionalLightParameters.AmbientColor;
    
    // calculate diffuse intensity
    float diffuseIntensity = CalculateLambertianDiffuse(worldNormal, cbDeferredDirectionalLightParameters.LightVector);
    float3 diffuseColor = ((baseColor * cbDeferredDirectionalLightParameters.LightColor) * diffuseIntensity);
    
    // calculate specular intensity
    //float specularIntensity = CalculateBlinnPhongSpecular(worldNormal, specularExponent, cbDeferredDirectionalLightParameters.LightVector, cameraVector) * specularCoefficient;        
    float specularIntensity = 0.0f;
    float3 specularColor = (cbDeferredDirectionalLightParameters.LightColor * specularIntensity);
    
    // get shadow term
    float shadowTerm = 1.0f;
#if WITH_SHADOW_MAP
    float3 positionWS = mul(ViewConstants.InverseViewMatrix, float4(positionVS, 1.0f)).xyz;
    //[branch] if (GBuffer2Value.g > 0.0f)
        shadowTerm = FindShadow(positionWS);
#endif

    // calculate surface colour
    float3 sceneColor = ambientColor + ((diffuseColor + specularColor) * shadowTerm);

    // cascade debug info
#if WITH_SHADOW_MAP && SHOW_CASCADES
    sceneColor *= SHADOW_MAP_SHOW_CASCADE_COLORS[SelectCascadeIndex(positionWS)];
#endif

    // return colour
    out_target = float4(sceneColor, 0.0f);
    //out_target = float4(saturate(sceneColor), 0.0f);
    //out_target = float4(saturate(baseColor * cbDeferredDirectionalLightParameters.AmbientColor), 0.0f);
    //out_target = float4(frac(positionWS.xy), 0.0f, 0.0f);
    //out_target = float4(saturate(positionWS.z / 60.0f), 0.0f, 0.0f, 0.0f);
    //out_target = float4(screenTexCoord.xy, 0.0f, 0.0f);
    //float ld = LinearizeDepth(DepthBufferValue);
    //out_target = float4(saturate(ld - positionVS.z), 0.0f, 0.0f, 0.0f);
    //out_target = float4(distance(ReconstructViewSpacePosition(screenTexCoord, DepthBufferValue), positionVS), 0.0f, 0.0f, 0.0f);
}

#endif
