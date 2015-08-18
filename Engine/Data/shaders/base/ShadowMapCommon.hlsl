// ShadowMapCommon.hlsl provides a FindShadow function depending on the shadow technique

#if DIRECTIONAL_SHADOW_TECHNIQUE_SSM

Texture2D ShadowMapTexture;
SamplerState ShadowMapSampler;
//SamplerComparisonState ShadowMapSampler;
float2 ShadowMapSize;
float4x4 LightViewProjectionMatrix;
//static const float SHADOW_MAP_EPSILON_VALUE = 0.00005;
//static const float SHADOW_MAP_EPSILON_VALUE = 0.0001;
static const float SHADOW_MAP_EPSILON_VALUE = 0.001;

float FindShadow(float3 worldPosition)
{
    float4 lightSpacePosition = mul(LightViewProjectionMatrix, float4(worldPosition, 1));
    lightSpacePosition.xyz /= lightSpacePosition.w;
    
    float2 shadowMapTextureCoordinates = 0.5 * lightSpacePosition.xy + float2(0.5, 0.5);
    shadowMapTextureCoordinates.y = 1.0 - shadowMapTextureCoordinates.y;
    
    // sample shadow map
    float shadowMapSample = ShadowMapTexture.Sample(ShadowMapSampler, shadowMapTextureCoordinates).r;
    
    // test if inside map or not
    float shadowContribution = (shadowMapSample < (lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE)) ? 0.0 : 1.0;
    
    // outside depth range?
    shadowContribution = (lightSpacePosition.z >= 1.0) ? 1.0 : shadowContribution;
    
    // return
    return shadowContribution;    
}

#endif      // DIRECTIONAL_SHADOW_TECHNIQUE_UNIFORM

#if DIRECTIONAL_SHADOW_TECHNIQUE_PSSM

#define DIRECTIONAL_SHADOW_TECHNIQUE_PSSM_MAXIMUM_CASCADES (3)

float PoissonDiscFilter(Texture2D textureObject, SamplerState samplerState, float2 textureCoordinates, float2 texelSize, float comparisonValue)
{	
    const float2 poissonDisk[24] = { 
        float2(0.5713538f, 0.7814451f),
        float2(0.2306823f, 0.6228884f),
        float2(0.1000122f, 0.9680607f),
        float2(0.947788f, 0.2773731f),
        float2(0.2837818f, 0.303393f),
        float2(0.6001099f, 0.4147638f),
        float2(-0.2314563f, 0.5434746f),
        float2(-0.08173513f, 0.0796717f),
        float2(-0.4692954f, 0.8651238f),
        float2(0.2768489f, -0.3682062f),
        float2(-0.5900795f, 0.3607553f),
        float2(-0.1010569f, -0.5284956f),
        float2(-0.4741178f, -0.2713854f),
        float2(0.4067073f, -0.00782522f),
        float2(-0.4603044f, 0.0511527f),
        float2(0.9820454f, -0.1295522f),
        float2(0.8187376f, -0.4105208f),
        float2(-0.8115796f, -0.106716f),
        float2(-0.4698426f, -0.6179109f),
        float2(-0.8402727f, -0.4400948f),
        float2(-0.2302377f, -0.879307f),
        float2(0.2748472f, -0.708988f),
        float2(-0.7874522f, 0.6162704f),
        float2(-0.9310728f, 0.3289311f)
    };
    
    // Number of samples
    #define TOTAL_SAMPLES 20

	float shadowAmount = 0;
	float sampleDiscSize = 1.7f;
	float2 pixelSize = texelSize * sampleDiscSize;

	// Sample the texture at various offsets

	[unroll]
	for (int i = 0; i < TOTAL_SAMPLES; i++)
		shadowAmount += (textureObject.Sample(samplerState, textureCoordinates + poissonDisk[i] * pixelSize).r >= comparisonValue);

	shadowAmount /= (TOTAL_SAMPLES + 1);
	return shadowAmount;
    
    #undef TOTAL_SAMPLES
}

float PCFFilter(Texture2D textureObject, SamplerState samplerState, float2 textureCoordinates, float2 texelSize, float comparisonValue)
{
    float value;
    float2 offset = texelSize * 0.5;
    //float2 offset = texelSize;
    
    value = (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, offset.y)).r >= comparisonValue);
    
    return value / 4;
}

float PCFFilter4(Texture2D textureObject, SamplerState samplerState, float2 textureCoordinates, float2 texelSize, float comparisonValue)
{
    float value;
    float2 offset = texelSize * 0.5;
    //float2 offset = texelSize;
    
    value = (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, offset.y)).r >= comparisonValue);
    
    return value / 4;
}

Texture2D ShadowMapTexture;
SamplerState ShadowMapSampler;
SamplerComparisonState ShadowMapComparisonSampler;
float4 ShadowMapSize;       // xy = width, height, zw = 1.0 / width, 1.0 / height

float CascadeCount;
float4x4 CascadeViewProjectionMatrix[DIRECTIONAL_SHADOW_TECHNIQUE_PSSM_MAXIMUM_CASCADES];
float CascadeSplitDepths[DIRECTIONAL_SHADOW_TECHNIQUE_PSSM_MAXIMUM_CASCADES];

static const float SHADOW_MAP_EPSILON_VALUE = 0.001;

float SelectCascadeIndex(float3 worldPosition)
{
/*
    float4 worldPosProjSpace = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
    float depthVal = worldPosProjSpace.z / worldPosProjSpace.w;
    
    float camNear = 0.0008;
    float camFar = 1.0;
    float linearZ = (2.0 * camNear) / (camFar + camNear - depthVal * (camFar - camNear));
    
    int shadowIndex = 0;
    float extend = 1.5;
    
    [unroll]
    for (int i = 0; i < DIRECTIONAL_SHADOW_TECHNIQUE_PSSM_MAXIMUM_CASCADES; i++)
        shadowIndex += (linearZ > CascadeSplitDepths[i] * extend);
        
    return shadowIndex;
*/

    //float eyeSpaceDepth = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1)).z;
    float eyeSpaceDepth = length(worldPosition - ViewConstants.EyePosition);
    float shadowIndex = 0;
    
    [unroll]
    for (int i = 0; i < DIRECTIONAL_SHADOW_TECHNIQUE_PSSM_MAXIMUM_CASCADES; i++)
        shadowIndex += (eyeSpaceDepth > CascadeSplitDepths[i]);
        
    return shadowIndex;
}

float FindShadow(float3 worldPosition)
{
    // select cascade index
    float cascadeIndex = SelectCascadeIndex(worldPosition);
    
    float4 lightSpacePosition = mul(CascadeViewProjectionMatrix[cascadeIndex], float4(worldPosition, 1));
    lightSpacePosition.xyz /= lightSpacePosition.w;
    
    // to shadow space
    float2 shadowMapTextureCoordinates = 0.5 * lightSpacePosition.xy + float2(0.5, 0.5);
    shadowMapTextureCoordinates.y = 1.0 - shadowMapTextureCoordinates.y;
    
    // convert them to atlas coordinates
    //shadowMapTextureCoordinates.x = (float)cascadeIndex * CascadeAtlasOffsetRange.x + shadowMapTextureCoordinates.x * CascadeAtlasOffsetRange.x;
    shadowMapTextureCoordinates.x += cascadeIndex;
    shadowMapTextureCoordinates.x /= CascadeCount;
    
    // sample shadow map, test value
    //float shadowMapSample = ShadowMapTexture.Sample(ShadowMapSampler, shadowMapTextureCoordinates).r;
    //float shadowContribution = shadowMapSample >= (lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE);
    
    // poisson disc filter
    float shadowContribution = PoissonDiscFilter(ShadowMapTexture, ShadowMapSampler, shadowMapTextureCoordinates, ShadowMapSize.zw, lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE);
    
    // pcf filter
    //float shadowContribution = PCFFilter(ShadowMapTexture, ShadowMapSampler, shadowMapTextureCoordinates, ShadowMapSize.zw, lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE);
    
    // comparison filter
    //float shadowContribution = ShadowMapTexture.SampleCmpLevelZero(ShadowMapComparisonSampler, shadowMapTextureCoordinates, lightSpacePosition.z - SHADOW_MAP_EPSILON_VALUE);
       
    // outside depth range?
    shadowContribution = (lightSpacePosition.z >= 1.0) ? 1.0 : shadowContribution;
    
    // return
    return shadowContribution;
}   

#endif      // DIRECTIONAL_SHADOW_TECHNIQUE_PSSM

#if POINT_SHADOW_TECHNIQUE_CUBE

/*
	float3 lvec = (LightPosition - in_parameters.WorldPosition);
    float3 ShadowTexCoords = normalize(-lvec);
    float Shadow = ShadowMapTexture.Sample(ShadowMapSampler, ShadowTexCoords).r;
    //float ShadowVal = float(length(lvec) < Shadow);
	//LightLuminosity *= ShadowVal;
	if (length(in_parameters.WorldLightVector) > Shadow)
		LightLuminosity = 0;
*/

#endif

#if 0
/*struct ShadowMapPSInputParameters
{
    float4 LightSpacePosition : TEXCOORD10;
}

ShadowMapPSInputParameters GetShadowMapPSInputParameters(float3 worldPosition)
{
    ShadowMapPSInputParameters parameters;
    parameters.LightSpacePosition = mul(LightViewProjectionMatrix, float4(worldPosition, 1.0)).xyzw;
    return parameters;
}*/

//float FindShadow(ShadowMapPSInputParameters parameters)
#endif
