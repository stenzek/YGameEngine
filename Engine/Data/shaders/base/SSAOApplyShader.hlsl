//------------------------------------------------------------------------------------------------------------
// SSAOShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

// Resources
Texture2D<float> AOTexture;
SamplerState AOTexture_SamplerState;

void PSMain(in float2 in_screenTexCoord : TEXCOORD0,
            out float4 out_target : SV_Target)
{
    // sample the ao buffer
    float occlusion = 1.0f - AOTexture.Sample(AOTexture_SamplerState, in_screenTexCoord);
    
    // clip samples that are not occluded or barely occluded to save bandwidth
    clip(occlusion - 0.01f);
    
    // blend state is (srccolor * 1) * (dstcolor * (1.0f - srcalpha))
    out_target = float4(0.0f, 0.0f, 0.0f, occlusion);
}
