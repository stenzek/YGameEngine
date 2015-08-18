//------------------------------------------------------------------------------------------------------------
// DebugNormalsShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#define MATERIAL_NEEDS_NORMAL 1
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// If the material is working in tangent-space normals, we need to bring in the tangent-to-world matrix.
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    #define MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD 1
#endif

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
    
    // Extract the normal from the material.
    float3 materialWorldNormal = MaterialGetNormal(MPIParameters);

    // If the normal is in tangent-space, convert it to world-space first.
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    materialWorldNormal = mul(MPIParameters.TangentToWorld, materialWorldNormal);
#endif

	float3 sceneColor = PackToColorRange3(materialWorldNormal);
    out_target = MaterialCalcOutputColor(MPIParameters, sceneColor);
}
