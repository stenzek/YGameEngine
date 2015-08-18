//------------------------------------------------------------------------------------------------------------
// ToneMapShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D<float3> HDRTexture;
SamplerState HDRTexture_SamplerState;

Texture2D<float> AverageLuminanceTexture;
SamplerState AverageLuminanceTexture_SamplerState;

Texture2D<float3> BloomTexture;
SamplerState BloomTexture_SamplerState;

uint AverageLuminanceTextureMip;
float WhiteLevel;
float LuminanceSaturation;
float ManualExposure;
float MaximumExposure;
float BloomMagnitude;
bool UseManualExposure;
bool EnableBloom;

float3 ApplyExposure(float3 color)
{
    float exposure;
    if (UseManualExposure)
    {
        exposure = ManualExposure;
    }
    else
    {
        float avgLuminance = max(AverageLuminanceTexture.Load(int3(0, 0, AverageLuminanceTextureMip)), 0.001f);
        //float keyValue = 1.03f - (2.0f / (2.0f + log10(avgLuminance + 1.0f)));
        float keyValue = 0.18f;
        float linearExposure = (keyValue / avgLuminance);
        exposure = min(log2(max(linearExposure, 0.0001f)), MaximumExposure);
    }
    
    return exp2(exposure) * color;
}

float3 CalculateBloom(float2 texCoord)
{
    float3 bloom = BloomTexture.SampleLevel(BloomTexture_SamplerState, texCoord, 0);
    bloom *= BloomMagnitude;
    return bloom;
}

float3 ToneMap(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = pixelLuminance * (1.0f + pixelLuminance / (WhiteLevel * WhiteLevel)) / (1.0f + pixelLuminance);
    return toneMappedLuminance * pow(color / pixelLuminance, LuminanceSaturation);
    //float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1);
    //return toneMappedLuminance * pow(color / pixelLuminance, LuminanceSaturation);
}

void PSMain(in float2 screenTexCoord : TEXCOORD0,
            out float3 out_target : SV_Target)
{
    float3 hdrColor = HDRTexture.SampleLevel(HDRTexture_SamplerState, screenTexCoord, 0);
    
    float3 exposedColor = ApplyExposure(hdrColor);
    
    float3 ldrColor = ToneMap(exposedColor);
    if (EnableBloom)
        ldrColor += CalculateBloom(screenTexCoord);
    
    out_target = ApplyGammaCorrection(ldrColor);
}
