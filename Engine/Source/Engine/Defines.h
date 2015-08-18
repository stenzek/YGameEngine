#pragma once

// Engine coordinate system
#define ENGINE_COORDINATE_SYSTEM COORDINATE_SYSTEM_Z_UP_RH

//--------------------------------- Drawing --------------------------------
enum RENDER_PASS
{
    //RENDER_PASS_Z_PREPASS                                   = (1 << 0),     // z pre-pass
    RENDER_PASS_LIGHTMAP                                    = (1 << 1),     // lightmaps
    RENDER_PASS_EMISSIVE                                    = (1 << 2),     // emissive
    RENDER_PASS_STATIC_LIGHTING                             = (1 << 3),     // light from static lights, applied dynamically
    RENDER_PASS_DYNAMIC_LIGHTING                            = (1 << 4),     // light from dynamic lights, applied dynamically
    RENDER_PASS_SHADOWED_LIGHTING                           = (1 << 5),     // light from dynamic lights that cast shadows, applied dynamically
    RENDER_PASS_SHADOW_MAP                                  = (1 << 6),     // shadow map occluders
    RENDER_PASS_OCCLUSION_CULLING_PROXY                     = (1 << 7),     // occlusion culling occluder pass
    RENDER_PASS_TINT                                        = (1 << 8),
    RENDER_PASS_POST_DRAW                                   = (1 << 9),
    
    // Combination enums.
    // Base passes
    RENDER_PASSES_DEFAULT                                   = RENDER_PASS_SHADOW_MAP |
                                                              RENDER_PASS_EMISSIVE |
                                                              RENDER_PASS_STATIC_LIGHTING |
                                                              RENDER_PASS_DYNAMIC_LIGHTING |
                                                              RENDER_PASS_SHADOWED_LIGHTING,

    // Render passes that are determined by material properties.
    RENDER_PASSES_AFFECTED_BY_MATERIAL                      = RENDER_PASS_EMISSIVE |
                                                              RENDER_PASS_STATIC_LIGHTING |
                                                              RENDER_PASS_DYNAMIC_LIGHTING |
                                                              RENDER_PASS_SHADOWED_LIGHTING,

    // Lighting render passes, used for masking out lights.
    RENDER_PASSES_LIGHTING                                  = RENDER_PASS_STATIC_LIGHTING |
                                                              RENDER_PASS_DYNAMIC_LIGHTING |
                                                              RENDER_PASS_SHADOWED_LIGHTING,
};

enum RENDER_LAYER
{
    RENDER_LAYER_NONE                                       = (0),
    RENDER_LAYER_CSG_GEOMETRY                               = (1 << 0),
    RENDER_LAYER_STATIC_GEOMETRY                            = (1 << 1),
    RENDER_LAYER_DYNAMIC_GEOMETRY                           = (1 << 2),
    RENDER_LAYER_EDITOR_VISUALS                             = (1 << 3),

    RENDER_LAYER_GAME_GROUP                                 = RENDER_LAYER_CSG_GEOMETRY | RENDER_LAYER_STATIC_GEOMETRY | RENDER_LAYER_DYNAMIC_GEOMETRY,
};

enum MATERIAL_DEBUG_MODE
{
    MATERIAL_DEBUG_MODE_NONE,
    MATERIAL_DEBUG_MODE_NORMALS,
    MATERIAL_DEBUG_MODE_FACE_NORMALS,
    MATERIAL_DEBUG_MODE_DIFFUSE_COLOR,
    MATERIAL_DEBUG_MODE_SPECULAR_COEFFICIENT,
    MATERIAL_DEBUG_MODE_COUNT,
};

enum LIGHTING_DEBUG_MODE
{
    LIGHTING_DEBUG_MODE_FLAT,
    LIGHTING_DEBUG_MODE_DETAIL,
    LIGHTING_DEBUG_MODE_COUNT,
};

enum MATERIAL_BLENDING_MODE
{
    MATERIAL_BLENDING_MODE_NONE,
    MATERIAL_BLENDING_MODE_ADDITIVE,
    MATERIAL_BLENDING_MODE_STRAIGHT,
    MATERIAL_BLENDING_MODE_PREMULTIPLIED,
    MATERIAL_BLENDING_MODE_MASKED,
    MATERIAL_BLENDING_MODE_SOFTMASKED,
    MATERIAL_BLENDING_MODE_COUNT,
};

enum MATERIAL_LIGHTING_TYPE
{
    MATERIAL_LIGHTING_TYPE_EMISSIVE,
    MATERIAL_LIGHTING_TYPE_REFLECTIVE,
    MATERIAL_LIGHTING_TYPE_REFLECTIVE_EMISSIVE,
    MATERIAL_LIGHTING_TYPE_REFLECTIVE_TWO_SIDED,
    MATERIAL_LIGHTING_TYPE_COUNT,
};

enum MATERIAL_LIGHTING_MODEL
{
    MATERIAL_LIGHTING_MODEL_PHONG,
    MATERIAL_LIGHTING_MODEL_BLINN_PHONG,
    MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED,
    MATERIAL_LIGHTING_MODEL_CUSTOM,
    MATERIAL_LIGHTING_MODEL_COUNT,
};

enum MATERIAL_LIGHTING_NORMAL_SPACE
{
    MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE,
    MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE,
    MATERIAL_LIGHTING_NORMAL_SPACE_COUNT,
};

enum MATERIAL_RENDER_MODE
{
    MATERIAL_RENDER_MODE_NORMAL,
    MATERIAL_RENDER_MODE_WIREFRAME,
    MATERIAL_RENDER_MODE_POST_PROCESS,
    MATERIAL_RENDER_MODE_COUNT,
};

enum MATERIAL_RENDER_LAYER
{
    MATERIAL_RENDER_LAYER_SKYBOX,
    MATERIAL_RENDER_LAYER_NORMAL,
    MATERIAL_RENDER_LAYER_COUNT,
};

enum MAP_SKY_TYPE
{
    MAP_SKY_TYPE_SKYBOX,
    MAP_SKY_TYPE_SKYSPHERE,
    MAP_SKY_TYPE_SKYDOME,
    MAP_SKY_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(MaterialBlendingMode);
    Y_Declare_NameTable(MaterialLightingType);
    Y_Declare_NameTable(MaterialLightingModel);
    Y_Declare_NameTable(MaterialLightingNormalSpace);
    Y_Declare_NameTable(MaterialRenderMode);
    Y_Declare_NameTable(MaterialRenderLayer);
}

#define MATERIAL_MAX_PARAMETER_NAME_LENGTH (256)
#define MATERIAL_MAX_STATIC_SWITCH_COUNT (16)

//-------------------------------- Lighting --------------------------------
enum LIGHT_TYPE
{
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
    LIGHT_TYPE_VOLUMETRIC,
    LIGHT_TYPE_COUNT,
};

enum LIGHT_SHADOW_FLAG
{
    LIGHT_SHADOW_FLAG_CAST_STATIC_SHADOWS           = (1 << 0),
    LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS          = (1 << 1),
};

enum ENTITY_SHADOW_FLAGS
{
    ENTITY_SHADOW_FLAG_CAST_STATIC_SHADOWS          = (1 << 0),
    ENTITY_SHADOW_FLAG_RECEIVE_STATIC_SHADOWS       = (1 << 1),
    ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS         = (1 << 2),
    ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS      = (1 << 3),
};

enum VOLUMETRIC_LIGHT_PRIMITIVE
{
    VOLUMETRIC_LIGHT_PRIMITIVE_BOX,
    VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE,
    VOLUMETRIC_LIGHT_PRIMITIVE_COUNT,
};

//-------------------------------- Textures --------------------------------
enum TEXTURE_TYPE
{
    TEXTURE_TYPE_1D,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_3D,
    TEXTURE_TYPE_CUBE,
    TEXTURE_TYPE_1D_ARRAY,
    TEXTURE_TYPE_2D_ARRAY,
    TEXTURE_TYPE_CUBE_ARRAY,
    TEXTURE_TYPE_DEPTH,
    TEXTURE_TYPE_COUNT,
};

enum TEXTURE_USAGE
{
    TEXTURE_USAGE_NONE,
    TEXTURE_USAGE_COLOR_MAP,
    TEXTURE_USAGE_GLOSS_MAP,
    TEXTURE_USAGE_ALPHA_MAP,
    TEXTURE_USAGE_NORMAL_MAP,
    TEXTURE_USAGE_HEIGHT_MAP,
    TEXTURE_USAGE_UI_ASSET,
    TEXTURE_USAGE_UI_LUMINANCE_ASSET,
    TEXTURE_USAGE_COUNT,
};

enum TEXTURE_FILTER
{
    TEXTURE_FILTER_MIN_MAG_MIP_POINT,
    TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR,
    TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT,
    TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT,
    TEXTURE_FILTER_MIN_MAG_MIP_LINEAR,
    TEXTURE_FILTER_ANISOTROPIC,
    TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
    TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
    TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
    TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
    TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
    TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
    TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
    TEXTURE_FILTER_COMPARISON_ANISOTROPIC,
    TEXTURE_FILTER_COUNT,
};

enum TEXTURE_ADDRESS_MODE
{
    TEXTURE_ADDRESS_MODE_WRAP,
    TEXTURE_ADDRESS_MODE_MIRROR,
    TEXTURE_ADDRESS_MODE_CLAMP,
    TEXTURE_ADDRESS_MODE_BORDER,
    TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
    TEXTURE_ADDRESS_MODE_COUNT,
};

enum CUBEMAP_FACE
{
    CUBEMAP_FACE_POSITIVE_X,
    CUBEMAP_FACE_NEGATIVE_X,
    CUBEMAP_FACE_POSITIVE_Y,
    CUBEMAP_FACE_NEGATIVE_Y,
    CUBEMAP_FACE_POSITIVE_Z,
    CUBEMAP_FACE_NEGATIVE_Z,
    CUBEMAP_FACE_COUNT,
};

enum TEXTURE_PLATFORM
{
    TEXTURE_PLATFORM_DXTC,
    TEXTURE_PLATFORM_PVRTC,
    TEXTURE_PLATFORM_ATC,
    TEXTURE_PLATFORM_ETC,
    TEXTURE_PLATFORM_ES2_NOTC,
    TEXTURE_PLATFORM_ES2_DXTC,
    NUM_TEXTURE_PLATFORMS
};

#define TEXTURE_MAX_MIPMAP_COUNT (15)

namespace NameTables {
    Y_Declare_NameTable(TextureType);
    Y_Declare_NameTable(TextureClassNames);
    Y_Declare_NameTable(TextureUsage);
    Y_Declare_NameTable(TextureFilter);
    Y_Declare_NameTable(TextureAddressMode);
    Y_Declare_NameTable(TexturePlatform);
    Y_Declare_NameTable(TexturePlatformFileExtension);
}

//-------------------------------- CSG -------------------------------

enum CSG_OPERATOR
{
    CSG_OPERATOR_ADD,				// "Union"
    CSG_OPERATOR_SUBTRACT,			// "Difference"
    CSG_OPERATOR_INTERSECTION,
    CSG_OPERATOR_DEINTERSECTION,
    CSG_OPERATOR_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(CSGOperator);
}
