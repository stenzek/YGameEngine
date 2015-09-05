//------------------------------------------------------------------------------------------------------------
// ShadowMapShader.hlsl
// There is two versions of this shader, one for materials with transparency/masked, and one without.
//------------------------------------------------------------------------------------------------------------

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"

#if WITH_MATERIAL
    #if MATERIAL_BLENDING_MODE_MASKED
        #define MATERIAL_NEEDS_OUTPUT_COLOR 1
    #else
        #define MATERIAL_NEEDS_OPACITY 1
    #endif
	#include "Material.hlsl"
#endif

#if !WITH_MATERIAL

// Shader without materials, only provides a vertex shader
void VSMain(in VertexFactoryInput in_input,
            out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

#else

// Shader with materials
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

void PSMain(in MaterialPSInterpolants in_interpolants
#if !HAS_FEATURE_LEVEL_SM4
            , out float4 out_target : SV_Target
#endif
           )
{
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
#if MATERIAL_BLENDING_MODE_MASKED
    // If using masked blending, use CalcOutputColor to clip the pixel
    MaterialCalcOutputColor(MPIParameters, float3(0, 0, 0));
    #if !HAS_FEATURE_LEVEL_SM4
        out_target = float4(0, 0, 0, 0);
    #endif
#else
    // Get the opacity, then clip from that
    clip(MaterialGetOpacity(MPIParameters) - 0.8);
    out_target = float4(0, 0, 0, 0);
#endif    
}

#endif      // WITH_MATERIAL