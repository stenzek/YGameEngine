//------------------------------------------------------------------------------------------------------------
// FogShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------

// Now, include the headers.
#include "Common.hlsl"

// Parameters
float FogStartDistance;
float FogEndDistance;
float FogDistance;
float FogDensity;
float3 FogColor;

// Depth buffer parameter
Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;

// Entry point
void PSMain(in float2 screenTexCoord : TEXCOORD0,
            in float3 in_viewRay : VIEWRAY,
            //in float4 screenPosition : SV_Position,
            out float4 out_target : SV_Target)
{
    // sample the gbuffers
    float DepthBufferValue = DepthBuffer.SampleLevel(DepthBuffer_SamplerState, screenTexCoord, 0.0f);
    
    // kill pixels without depth
    if (DepthBufferValue == 1.0f)
        discard;

    // reconstruct view-space position
    float3 positionVS = in_viewRay * LinearizeDepth(DepthBufferValue);
    //float pixelDistance = positionVS.z;
    float pixelDistance = length(positionVS);
    
    // kill pixels that are out-of-range
    //clip(FogEndDistance - pixelDistance);
    if (pixelDistance < FogStartDistance)
        discard;
    
    float fogFactor;    
#if FOG_MODE_LINEAR
    // linear fog
    fogFactor = saturate((FogEndDistance - pixelDistance) / FogDistance);
#elif FOG_MODE_EXP
    // exponential fog
    float d = pixelDistance * FogDensity;
    fogFactor = (1.0f / exp(d));
#elif FOG_MODE_EXP2
    // exponential fog squared
    float d = pixelDistance * fogDensity;
    fogFactor = (1.0f / exp(d * d));
#else
    fogFactor = 0.0f;
#endif

    out_target = float4(FogColor.xyz, 1.0f - fogFactor);
}
