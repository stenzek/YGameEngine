#include "Common.hlsl"
#include "VertexFactory.hlsl"

#if WITH_MATERIAL

// Shader with material
#define MATERIAL_NEEDS_OUTPUT_COLOR 1
#include "Material.hlsl"

void VSMain(in VertexFactoryInput in_input,
			out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);
	out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

float4 PSMain() : SV_Target
{
    return float4(0, 0, 0, 0);
}

#else

// Shader without material
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

float4 PSMain(in MaterialPSInterpolants in_interpolants) : SV_Target
{
    // Handle masked/blended/etc materials
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    return MaterialCalcOutputColor(inputParameters, float3(0, 0, 0));
}

#endif
