//------------------------------------------------------------------------------------------------------------
// DeferredPointLightShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

// skew value
static const float SHADOW_MAP_DEPTH_BIAS = 0.001f;
static const float SHADOW_MAP_Z_NEAR = 0.1f;

// Input variables.
cbuffer DeferredPointLightParameters { struct
{
    float3 LightColor;
    float3 LightPosition;
    float3 LightPositionVS;
    float LightRange;
    float LightInverseRange;
    float LightFalloffExponent;
} cbDeferredPointLightParameters; }

Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;
Texture2D<float4> GBuffer0;
SamplerState GBuffer0_SamplerState;
Texture2D<float4> GBuffer1;
SamplerState GBuffer1_SamplerState;
Texture2D<float4> GBuffer2;
SamplerState GBuffer2_SamplerState;

// Shadow map
#if WITH_SHADOW_MAP

#include "ShadowMapFilters.hlsl"

TextureCube<float> ShadowMapTexture;
#if USE_HARDWARE_PCF
	SamplerComparisonState ShadowMapTexture_SamplerState;
#else
	SamplerState ShadowMapTexture_SamplerState;
#endif

float LightVectorToDepthValue(float3 lightVector)
{
    float zNear = SHADOW_MAP_Z_NEAR;
    float zFar = 1.0f / cbDeferredPointLightParameters.LightInverseRange;
    
    float3 absVec = abs(lightVector);
    float maxDepth = max(absVec.x, max(absVec.y, absVec.z));
    
    //float rz = (zFar / (zNear - zFar)) * -maxDepth + (zNear * zFar / (zNear - zFar)) * 1.0f;
    //float rw = -1.0f * -maxDepth;
    //return rz / rw;
    
    return ((zFar / (zNear - zFar)) * -maxDepth + (zNear * zFar / (zNear - zFar))) / maxDepth;
}

#endif      // WITH_SHADOW_MAP

void VSMain(in float3 in_position : POSITION,
            out float4 out_positionVS : VIEWPOSITION,
            out float4 out_positionCS : SV_Position)
{
    // multiply and pass-through
    float4 worldPosition = float4(in_position * cbDeferredPointLightParameters.LightRange + cbDeferredPointLightParameters.LightPosition, 1.0f);
    out_positionVS = mul(ViewConstants.ViewMatrix, worldPosition);
    out_positionCS = mul(ViewConstants.ViewProjectionMatrix, worldPosition);
}

void PSMain(in float4 in_positionVS : VIEWPOSITION,
            in float4 screenPosition : SV_Position,
            out float4 out_target : SV_Target)
{
    // sample the gbuffers -- move to deferredshading common
    int3 texelCoordinates = int3(int2(screenPosition.xy), 0);
    float DepthBufferValue = DepthBuffer.Load(texelCoordinates);
    float4 GBuffer0Value = GBuffer0.Load(texelCoordinates);
    float4 GBuffer1Value = GBuffer1.Load(texelCoordinates);
    float4 GBuffer2Value = GBuffer2.Load(texelCoordinates);
    
    // reconstruct the view-space position of the pixel
    // https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    //float4 positionVSW = mul(ViewConstants.InverseProjectionMatrix, screenPosition);
    //float3 positionVS = positionVSW.xyz / positionVSW.w;
    float3 viewRay = float3(in_positionVS.xy / in_positionVS.z, 1.0f);
    float3 positionVS = viewRay * LinearizeDepth(DepthBufferValue);
    
    // extract parameters from gbuffer
    float3 normalVS = UnpackFromColorRange3(GBuffer1Value.xyz);
    float3 baseColor = GBuffer0Value.rgb;
    float specularCoefficient = GBuffer0Value.a;
    float specularExponent = GBuffer1Value.a;
    
    // calculate light vectors
    float3 lightVectorF = positionVS - cbDeferredPointLightParameters.LightPositionVS;
    float lightDistance = length(lightVectorF);
    float3 lightVector = normalize(lightVectorF);
    float3 cameraVector = normalize(positionVS);
    
    // calculate point light falloff
    float lightAmount = pow(saturate(1.0f - lightDistance * cbDeferredPointLightParameters.LightInverseRange), cbDeferredPointLightParameters.LightFalloffExponent);
    
    // todo: material modes..
    
    // calculate diffuse intensity
    float diffuseIntensity = CalculateLambertianDiffuse(normalVS, lightVector);
    float3 diffuseColor = ((baseColor * cbDeferredPointLightParameters.LightColor) * diffuseIntensity);
    
    // calculate specular intensity
    //float specularIntensity = CalculateBlinnPhongSpecular(normalVS, specularExponent, lightVector, cameraVector) * specularCoefficient;        
    float specularIntensity = 0.0f;
    float3 specularColor = (cbDeferredPointLightParameters.LightColor * specularIntensity);
    
    // multiply by shadow term
#if WITH_SHADOW_MAP
    //[branch] if (GBuffer2Value.g > 0.0f)
    {
        // possibly just transform VS light vector to WS?
        float3 positionWS = mul(ViewConstants.InverseViewMatrix, float4(positionVS, 1.0f)).xyz;
        float3 lightVectorWS = positionWS - cbDeferredPointLightParameters.LightPosition;
        float shadowComparisonValue = LightVectorToDepthValue(lightVectorWS) - SHADOW_MAP_DEPTH_BIAS;
        #if USE_HARDWARE_PCF
            lightAmount *= ShadowMapTexture.SampleCmpLevelZero(ShadowMapTexture_SamplerState, lightVectorWS.xzy, shadowComparisonValue);
        #else
            float shadowValue = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, lightVectorWS.xzy, 0.0f);
            lightAmount *= (shadowValue >= shadowComparisonValue);
        #endif
    }
#endif    

    // calculate surface colour
    float3 sceneColor = ((diffuseColor + specularColor) * lightAmount);

    // return colour
    out_target = float4(sceneColor, 0.0f);
    //out_target = float4(PackToColorRange3(lightVectorWS), 0.0f);
    //out_target = float4(saturate(sceneColor), 0.0f);
    //out_target = float4(saturate(baseColor * cbDeferredDirectionalLightParameters.AmbientColor), 0.0f);
    //out_target = float4(frac(positionWS.xy), 0.0f, 0.0f);
    //out_target = float4(saturate(positionWS.z / 60.0f), 0.0f, 0.0f, 0.0f);
    //out_target = float4(screenTexCoord.xy, 0.0f, 0.0f);
    //float ld = LinearizeDepth(DepthBufferValue);
    //out_target = float4(saturate(ld - positionVS.z), 0.0f, 0.0f, 0.0f);
    //out_target = float4(distance(ReconstructViewSpacePosition(screenTexCoord, DepthBufferValue), positionVS), 0.0f, 0.0f, 0.0f);
}
