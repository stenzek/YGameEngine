//------------------------------------------------------------------------------------------------------------
// DeferredGBufferShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
// From the material, we need the tangent normal, diffuse, and specular colors, as well as specular exponent.
#define MATERIAL_NEEDS_NORMAL 1
#define MATERIAL_NEEDS_BASE_COLOR 1
#define MATERIAL_NEEDS_SPECULAR_COEFFICIENT 1
#define MATERIAL_NEEDS_SPECULAR_EXPONENT 1
#define MATERIAL_NEEDS_REFLECTIVITY 1

// If the material is working in tangent-space normals, we need to bring in the tangent-to-world matrix.
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    #define MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD 1
#endif

// Masked materials go through the deferred shading path.
#if MATERIAL_BLENDING_MODE_MASKED
    #define MATERIAL_NEEDS_OPACITY 1
#endif

// Bring in light map value from vertex factory if requested
#if WITH_LIGHTMAP
    #define VERTEX_FACTORY_NEEDS_LIGHTMAP 1
#endif

// Shadow mask
#if MATERIAL_RECEIVES_SHADOWS
    static const float MATERIAL_SHADOW_MASK = 1.0f;
#else
    static const float MATERIAL_SHADOW_MASK = 0.0f;
#endif

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"
#include "DeferredCommon.hlsl"

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

	// Write parameters
    out_interpolants = GetMaterialPSInterpolantsVS(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(in MaterialPSInterpolants in_interpolants,
#if WITH_LIGHTMAP
            in VertexFactoryLightMapInterpolants in_lightMapInterpolants,
#endif
            out float4 out_GBuffer0 : SV_Target0,
            out float4 out_GBuffer1 : SV_Target1,
            out float4 out_GBuffer2 : SV_Target2
#if WITH_LIGHTBUFFER
            , out float4 out_lightBuffer : SV_Target3
#endif
            )
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
    // Masked materials go through the opaque render queue, so handle these.
    #if MATERIAL_BLENDING_MODE_MASKED
        clip(MaterialGetOpacity(MPIParameters) - 0.8);
    #endif
    
    // Fill gbuffer values.
    GBufferWriteData data;
    
    // Extract parameters from the material.
#if NO_ALBEDO
    data.BaseColor = float3(0.0f, 0.0f, 0.0f);
    data.SpecularFactorOrMetallic = 0.0f;
    data.SpecularPowerOrSpecular = 0.0f;
    data.Roughness = 0.0f;
    data.ShadowMask = 0.0f;
    data.ShadingModel = 0;
    data.TwoSidedLighting = false;
#else
    data.BaseColor = MaterialGetBaseColor(MPIParameters);
    data.SpecularFactorOrMetallic = MaterialGetSpecularCoefficient(MPIParameters);
    data.SpecularPowerOrSpecular = EncodeSpecularPower(MaterialGetSpecularExponent(MPIParameters));
    data.Roughness = 0.0f;
    data.ShadowMask = MATERIAL_SHADOW_MASK;
    data.ShadingModel = SelectShadingModelFromMaterialDefines();
    data.TwoSidedLighting = false;
    
    // Since the light blending is associative, we can apply the blend colour to 
	// the material base colour, rather than on the gbuffer/lightbuffer outputs.
	#if MATERIAL_TINT
		data.BaseColor *= ObjectConstants.MaterialTintColor.rgb;
	#endif
#endif
    
    // Extract the normal from the material.
    float3 materialWorldNormal = MaterialGetNormal(MPIParameters);

    // If the normal is in tangent-space, convert it to world-space first.
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    materialWorldNormal = mul(MPIParameters.TangentToWorld, materialWorldNormal);
#endif
    
    // Bring the normal to world-space.
    data.ViewNormal = normalize(mul((float3x3)ViewConstants.ViewMatrix, materialWorldNormal));

    // Write gbuffer values.
    WriteGBufferData(data, out_GBuffer0, out_GBuffer1, out_GBuffer2);
        
#if WITH_LIGHTBUFFER
    // Write emissive or lightmap value
    #if MATERIAL_LIGHTING_TYPE_EMISSIVE
        out_lightBuffer = float4(MaterialGetBaseColor(MPIParameters), 1.0f);
    #elif WITH_LIGHTMAP
        float3 lightMapValue = VertexFactoryGetLightMapColor(in_lightMapInterpolants);
        out_lightBuffer = float4(MaterialGetBaseColor(MPIParameters) * lightMapValue, 1.0f);
    #else
        out_lightBuffer = float4(0.0f, 0.0f, 0.0f, 0.0f);
    #endif
#endif
}
