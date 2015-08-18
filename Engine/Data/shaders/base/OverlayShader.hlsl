//------------------------------------------------------------------------------------------------------------
// GUIShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D DrawTexture;
SamplerState DrawTexture_SamplerState;

//float4 TintColor;

void ScreenVS(in float2 in_pos : POSITION,
              in float2 in_uv : TEXCOORD,
              in float4 in_color : COLOR,
              out float2 out_uv : TEXCOORD,
              out float4 out_color : COLOR,
              out float4 out_pos : SV_Position)
{
    out_uv = in_uv;
    out_color = in_color;
    out_pos = mul(ViewConstants.ScreenProjectionMatrix, float4(in_pos, 0.0f, 1.0f));
}

void WorldVS(in float3 in_pos : POSITION,
             in float2 in_uv : TEXCOORD,
             in float4 in_color : COLOR,
             out float2 out_uv : TEXCOORD,
             out float4 out_color : COLOR,
             out float4 out_pos : SV_Position)
{
    out_uv = in_uv;
    out_color = in_color;
    out_pos = mul(ViewConstants.ViewProjectionMatrix, mul(ObjectConstants.WorldMatrix, float4(in_pos, 1.0f)));
}

void ColoredPS(in float2 in_uv : TEXCOORD,
               in float4 in_color : COLOR,
               out float4 out_color : SV_Target)
{
    out_color = in_color;
}

void TexturedPS(in float2 in_uv : TEXCOORD,
                in float4 in_color : COLOR,
                out float4 out_color : SV_Target)
{
    out_color = DrawTexture.Sample(DrawTexture_SamplerState, in_uv) * in_color;
}
