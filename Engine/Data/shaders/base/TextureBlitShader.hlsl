//------------------------------------------------------------------------------------------------------------
// TextureCopyShader.hlsl
// Copies a texture to another, optionally cropping it.
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D SourceTexture;
SamplerState SourceTexture_SamplerState;

#if USE_TEXTURE_LOD
    uniform float SourceLevel;
#endif

void VSMain(in float2 in_pos : POSITION,
            in float2 in_uv : TEXCOORD,
            out float2 out_uv : TEXCOORD0,
            out float4 out_pos : SV_Position)
{
    out_uv = in_uv;
    out_pos = mul(ViewConstants.ScreenProjectionMatrix, float4(in_pos, 0.0f, 1.0f));
}

void PSMain(in float2 in_uv : TEXCOORD0,
            out float4 out_color : SV_Target)
{
#if USE_TEXTURE_LOD
    out_color = SourceTexture.SampleLevel(SourceTexture_SamplerState, in_uv, SourceLevel);
#else
    out_color = SourceTexture.Sample(SourceTexture_SamplerState, in_uv);
#endif
}

