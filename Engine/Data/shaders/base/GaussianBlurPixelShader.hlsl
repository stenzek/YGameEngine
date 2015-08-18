//------------------------------------------------------------------------------------------------------------
// ExtractLuminancePixelShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D<float3> SourceTexture;
SamplerState SourceTexture_SamplerState;
float2 BlurDirection;
float BlurSigma;

void Main(in float2 in_screenTexCoord : TEXCOORD,
          out float3 out_target : SV_Target)
{
    float3 outColor = 0;
    
    [unroll] for (int i = -6; i < 6; i++)
    {
        float fi = (float)i;
        float g = 1.0f / sqrt(2.0f * 3.14159f * BlurSigma * BlurSigma);
        float weight = (g * exp(-(fi * fi) / (2.0f * BlurSigma * BlurSigma)));
        float2 texCoord = ((float2(fi, fi) / ViewportConstants.ViewportSize) * BlurDirection) + in_screenTexCoord;
        float3 sample = SourceTexture.SampleLevel(SourceTexture_SamplerState, texCoord, 0);
        outColor += sample * weight;
    }
    
    out_target = outColor;
}
