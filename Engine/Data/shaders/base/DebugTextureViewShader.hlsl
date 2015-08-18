//------------------------------------------------------------------------------------------------------------
// DebugTextureViewPixelShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

float4 OutputRedMask;
float4 OutputGreenMask;
float4 OutputBlueMask;
float4 OutputAlphaMask;
float InputTextureLayer;

#if WITH_TEXTURE2D
    Texture2D InputTexture;
#elif WITH_TEXTURE2DARRAY
    Texture2DArray InputTexture;
#elif WITH_TEXTURECUBE
    TextureCube InputTexture;
#endif

void VSMain(in float2 in_pos : POSITION,
            in float2 in_tex : TEXCOORD,
            out float2 out_tex : TEXCOORD,
            out float4 out_pos : SV_Position)
{

}

void PSMain(in float2 in_tex : TEXCOORD,
            out float4 out_color : SV_Target)
{

}
