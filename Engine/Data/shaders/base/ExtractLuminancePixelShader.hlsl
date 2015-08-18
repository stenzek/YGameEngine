//------------------------------------------------------------------------------------------------------------
// ExtractLuminancePixelShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D<float3> SceneTexture;
SamplerState SceneTexture_SamplerState;

void PSMain(in float2 in_screenTexCoord : TEXCOORD,
            out float out_target : SV_Target)
{
    float3 color = SceneTexture.SampleLevel(SceneTexture_SamplerState, in_screenTexCoord, 0);
       
    // extract luminance
    float luminance = CalculateLuminance(color);
    
    // to srgb?
    //luminance = pow(luminance, 2.2f);
    //out_target = log(luminance);
    out_target = max(luminance, 0.0001f);
}
