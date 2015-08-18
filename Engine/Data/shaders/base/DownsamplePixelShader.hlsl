//------------------------------------------------------------------------------------------------------------
// DownsampleShader.hlsl
// Averages a 4x4 pixel region and writes to a smaller render target.
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

static const int BOX_SIZE = 4;

Texture2D InputTexture;
SamplerState InputTexture_SamplerState;
uint MipLevel;

void PSMain(in float2 in_screenTexCoord : TEXCOORD0,
            out float4 out_target : SV_Target)
{
    float2 dimensions;
    uint nMipLevels;
    InputTexture.GetDimensions(MipLevel, dimensions.x, dimensions.y, nMipLevels);
    float2 texelSize = float2(1.0f, 1.0f) / dimensions;
    float fMipLevel = (float)MipLevel;

    float2 base = (float)(-BOX_SIZE) * 0.5f + 0.5f;
    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    [unroll] for (int x = 0; x < 4; x++)
    {
        [unroll] for (int y = 0; y < 4; y++)
        {
            float2 offset = float2(int2(x, y));
            offset += base;
            offset *= texelSize;
            
            result += InputTexture.SampleLevel(InputTexture_SamplerState, in_screenTexCoord + offset, fMipLevel);
        }
    }
   
    out_target = result / (float)(BOX_SIZE * BOX_SIZE);
}
