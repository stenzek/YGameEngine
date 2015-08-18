//------------------------------------------------------------------------------------------------------------
// EmissiveShader.glsl
// 
//------------------------------------------------------------------------------------------------------------

// From the material, we need the tangent normal, diffuse, and specular colors, as well as specular exponent.
#define MATERIAL_NEEDS_BASE_COLOR 1
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
			out MaterialPSInterpolants out_interpolants,
            out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);

#if MATERIAL_FEATURE_LEVEL >= FEATURE_LEVEL_ES3
    MaterialVSInputParameters MVIParameters = GetMaterialVSInputParametersFromVertexFactory(in_input);
    worldPosition += MaterialGetWorldPositionOffset(MVIParameters);
#endif
	
	out_interpolants = GetMaterialPSInterpolantsVS(in_input);    
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(in MaterialPSInterpolants in_interpolants,
            out float4 out_target : SV_Target)
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
	
	// Get emissive color
	float3 emissiveColor = MaterialGetBaseColor(MPIParameters);

    // Determine scene color.
    out_target = MaterialCalcOutputColor(MPIParameters, emissiveColor);
}
