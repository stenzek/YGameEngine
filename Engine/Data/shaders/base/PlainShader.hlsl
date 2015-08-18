//------------------------------------------------------------------------------------------------------------
// PlainShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"
#include "VertexFactory.hlsl"

#if WITH_TEXTURE
    Texture2D PlainTexture;
	SamplerState PlainTexture_SamplerState;
#endif
#if WITH_MATERIAL
    #if MATERIAL_LIGHTING_MODEL_EMISSIVE
        #define MATERIAL_NEEDS_EMISSIVE_COLOR 1
    #else
        #define MATERIAL_NEEDS_DIFFUSE_COLOR 1
    #endif
    
    #include "Material.hlsl"
#endif

void VSMain(in VertexFactoryInput in_input,
#if WITH_VERTEX_COLOR
            out float4 out_vertexColor : COLOR0, 
#endif
#if WITH_TEXTURE
            out float2 out_texCoord : TEXCOORD0, 
#endif
#if WITH_MATERIAL
            out MaterialPSInterpolants out_interpolants,
#endif
			out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);
	
#if WITH_VERTEX_COLOR
    out_vertexColor = VertexFactoryGetVertexColor(in_input);
#endif
#if WITH_TEXTURE
    out_texCoord = VertexFactoryGetTexCoord(in_input).xy;
#endif
#if WITH_MATERIAL
    out_interpolants = GetMaterialPSInterpolantsVS(in_input);
#endif

    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));
}

void PSMain(
#if WITH_VERTEX_COLOR
            in float4 in_vertexColor : COLOR0,
#endif
#if WITH_TEXTURE
            in float2 in_texCoord : TEXCOORD0,
#endif
#if WITH_MATERIAL
            in MaterialPSInterpolants in_interpolants,
#endif
            out float4 OutColor : SV_Target
           )
{
#if BLEND_TEXTURE_AND_VERTEX_COLOR && WITH_TEXTURE && WITH_VERTEX_COLOR
    OutColor = PlainTexture.Sample(PlainTexture_SamplerState, in_texCoord) * in_vertexColor;
#elif WITH_TEXTURE
    OutColor = PlainTexture.Sample(PlainTexture_SamplerState, in_texCoord);
#elif WITH_VERTEX_COLOR
    OutColor = in_vertexColor;
#elif WITH_MATERIAL
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
    float3 sceneColor;    
    #if MATERIAL_LIGHTING_MODEL_EMISSIVE
        sceneColor = MaterialGetEmissiveColor(MPIParameters);
    #else
        sceneColor = MaterialGetDiffuseColor(MPIParameters);
    #endif
    
    OutColor = MaterialCalcOutputColor(MPIParameters, sceneColor);
#else
    #error Unknown configuration.
#endif
}

