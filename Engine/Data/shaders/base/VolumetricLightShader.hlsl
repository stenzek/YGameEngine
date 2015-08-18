//------------------------------------------------------------------------------------------------------------
// AmbientLightShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------

// From the material, we need the tangent normal, diffuse, and specular colors, as well as specular exponent.
#define MATERIAL_NEEDS_BASE_COLOR 1	
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

// Shader options
cbuffer VolumetricLightParameters { struct
{
    float3 LightColor;
    float3 LightPosition;
    float LightFalloff;
    float3 LightBoxExtents;
    float LightSphereRadius;
} cbVolumetricLightParameters; }

#if VOLUMETRIC_LIGHT_BOX_PRIMITIVE
#elif VOLUMETRIC_LIGHT_SPHERE_PRIMITIVE
#endif

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
			out MaterialPSInterpolants out_interpolants,
            out float3 out_lightSpaceVector : LIGHTSPACEPOS,
            out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);

#if MATERIAL_FEATURE_LEVEL >= FEATURE_LEVEL_ES3
    MaterialVSInputParameters MVIParameters = GetMaterialVSInputParametersFromVertexFactory(in_input);
    worldPosition += MaterialGetWorldPositionOffset(MVIParameters);
#endif
	
	out_interpolants = GetMaterialPSInterpolantsVS(in_input);    

#if VOLUMETRIC_LIGHT_SPHERE_PRIMITIVE
    out_lightSpaceVector = (cbVolumetricLightParameters.LightPosition - worldPosition) / LightSphereRadius;
#else
    out_lightSpaceVector = (cbVolumetricLightParameters.LightPosition - worldPosition);
#endif

    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(in MaterialPSInterpolants in_interpolants,
            in float3 in_lightSpaceVector : LIGHTSPACEPOS,
            out float4 out_target : SV_Target)
{
	// Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);

    // Work out light intensity
    float lightIntensity;
#if VOLUMETRIC_LIGHT_SPHERE_PRIMITIVE
    lightIntensity = pow(1.0f - saturate(length(in_lightSpaceVector)), cbVolumetricLightParameters.LightFalloff);
#else
    lightIntensity = 1.0f;
#endif

	float3 sceneColor = MaterialGetBaseColor(MPIParameters) * cbVolumetricLightParameters.LightColor * lightIntensity;
    out_target = MaterialCalcOutputColor(MPIParameters, sceneColor);
}
