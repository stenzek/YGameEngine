#include "Common.hlsl"

void VSMain(in float3 in_pos : POSITION,
            out float4 out_pos : SV_Position)
{
    out_pos = mul(ViewConstants.ViewProjectionMatrix, float4(in_pos, 1.0f));
}
