#define MATERIAL_INCLUDED 1

struct MaterialVSInputParameters
{
    float3 WorldPosition;
    float3 WorldNormal;
    float4 TexCoord0;
    float4 TexCoord1;
    float4 VertexColor;
    float3x3 TangentToWorld;
};

struct MaterialGSInputParameters
{
    float3 WorldPosition;
    float3 WorldNormal;
    float4 TexCoord0;
    float4 TexCoord1;
    float4 VertexColor;
    float3x3 TangentToWorld;
};

struct MaterialPSInputParameters
{
    float3 WorldPosition;
    float3 WorldNormal;
    float4 TexCoord0;
    float4 TexCoord1;
    float4 VertexColor;
    float3x3 TangentToWorld;
    float4 ScreenPosition;
};

#if MATERIAL_NEEDS_LIGHT_REFLECTION
    #if MATERIAL_LIGHTING_MODEL_PHONG || MATERIAL_LIGHTING_MODEL_BLINN_PHONG
        #define MATERIAL_NEEDS_NORMAL 1
        #define MATERIAL_NEEDS_BASE_COLOR 1
        #define MATERIAL_NEEDS_SPECULAR_COEFFICIENT 1
        #define MATERIAL_NEEDS_SPECULAR_EXPONENT 1
    #elif MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED
        #define MATERIAL_NEEDS_NORMAL 1
        #define MATERIAL_NEEDS_BASE_COLOR 1
        #define MATERIAL_NEEDS_ROUGHNESS 1
    #elif MATERIAL_LIGHTING_MODEL_CUSTOM
        #define MATERIAL_NEEDS_CUSTOM_LIGHTING 1
    #endif
#endif

#if MATERIAL_NEEDS_OUTPUT_COLOR
	// If doing any sort of alpha blending, we require opacity.
	#if MATERIAL_BLENDING_MODE_STRAIGHT || MATERIAL_BLENDING_MODE_PREMULTIPLIED || MATERIAL_BLENDING_MODE_MASKED
		#define MATERIAL_NEEDS_OPACITY 1
	#endif
#endif

// Post processing-rendered materials have access to scene color and depth
#if MATERIAL_RENDER_MODE_POST_PROCESS
    Texture2D MTLPostProcessParameter_SceneColor;
    SamplerState MTLPostProcessParameter_SceneColor_SamplerState;
    Texture2D MTLPostProcessParameter_SceneDepth;
    SamplerState MTLPostProcessParameter_SceneDepth_SamplerState;
#endif

// Bring in normal and world position if required by the shaders
// If the material is working in tangent-space normals, we need to bring in the tangent-to-world matrix.
#if MATERIAL_NEEDS_WORLD_POSITION && !MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    #define MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION 1
#endif
#if MATERIAL_NEEDS_WORLD_NORMAL 
    #if !MATERIAL_NEEDS_NORMAL
        #define MATERIAL_NEEDS_NORMAL 1
    #endif
    #if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE && !MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
        #define MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD 1
    #endif        
#endif

//! Begin User Code

