//------------------------------------------------------------------------------------------------------------
// BasePassShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------

#if WITH_EMISSIVE
	#define MATERIAL_NEEDS_BASE_COLOR 1
#endif

// Bring in light map value from vertex factory if requested
#if WITH_LIGHTMAP
    #define MATERIAL_NEEDS_BASE_COLOR 1
    #define VERTEX_FACTORY_NEEDS_LIGHTMAP 1
#endif

#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
            out MaterialPSInterpolants out_interpolants,
#if WITH_LIGHTMAP
            out VertexFactoryLightMapInterpolants out_lightMapInterpolants,
#endif
			out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);

#if MATERIAL_FEATURE_LEVEL >= FEATURE_LEVEL_ES3
    MaterialVSInputParameters MVIParameters = GetMaterialVSInputParametersFromVertexFactory(in_input);
    worldPosition += MaterialGetWorldPositionOffset(MVIParameters);
#endif

#if WITH_LIGHTMAP
    out_lightMapInterpolants = VertexFactoryGetLightMapInterpolants(in_input);
#endif
        
	out_interpolants = GetMaterialPSInterpolantsVS(in_input);
	out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(in MaterialPSInterpolants in_interpolants,
#if WITH_LIGHTMAP
            in VertexFactoryLightMapInterpolants in_lightMapInterpolants,
#endif
            out float4 out_target : SV_Target)
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
#if WITH_EMISSIVE
    // just sample and write emissive colour
    out_target = MaterialCalcOutputColor(MPIParameters, MaterialGetBaseColor(MPIParameters));
#elif WITH_LIGHTMAP
    // Sample the light map
    float3 lightMapValue = VertexFactoryGetLightMapColor(in_lightMapInterpolants);
    out_target = MaterialCalcOutputColor(MPIParameters, MaterialGetBaseColor(MPIParameters) * lightMapValue);
#else
    // used to write depth
    out_target = MaterialCalcOutputColor(MPIParameters, float3(0, 0, 0));
#endif
}
