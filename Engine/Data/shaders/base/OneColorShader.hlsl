//------------------------------------------------------------------------------------------------------------
// OneColorShader.hlsl
// Shader for rendering a single color.
//------------------------------------------------------------------------------------------------------------

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"

// Parameters
float4 DrawColor;

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
             out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(out float4 out_target : SV_Target)
{
	// Return color value.
	out_target = DrawColor;
}
