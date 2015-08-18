//------------------------------------------------------------------------------------------------------------
// BloomShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D<float3> HDRTexture;
SamplerState HDRTexture_SamplerState;

float BloomThreshold;

void Main(in float2 screenTexCoord : TEXCOORD0,
          out float3 out_target : SV_Target)
{
    float3 hdrColor = HDRTexture.SampleLevel(HDRTexture_SamplerState, screenTexCoord, 0);
    out_target = saturate((hdrColor - BloomThreshold) / (1.0f - BloomThreshold));
}
