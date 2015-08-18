#pragma once
#include "Engine/Common.h"

#pragma pack(push, 4)

#ifdef Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable:4200)   // warning C4200: nonstandard extension used : zero-sized array in struct/union
    #pragma warning(disable:4510)   // warning C4510: 'DF_PROPERTY_MAPPING_HEADER' : default constructor could not be generated
    #pragma warning(disable:4512)   // warning C4512: 'DF_PROPERTY_MAPPING_HEADER' : assignment operator could not be generated
    #pragma warning(disable:4610)   // warning C4610: struct 'DF_PROPERTY_MAPPING_HEADER' can never be instantiated - user defined constructor required
#endif

//----------------------------------------- .font file ------------------------------------------
#define DF_FONT_HEADER_MAGIC ((uint32)'FNT2')

struct DF_FONT_HEADER
{
    uint32 Magic;
    uint32 Size;
    uint32 Type;
    uint32 BestRenderingSize;

    uint32 CharacterDataCount;
    uint32 CharacterDataOffset;

    uint32 TextureCount;
    uint32 TextureOffset;
};

struct DF_FONT_CHARACTER
{
    uint32 CodePoint;
    float StartU;
    float EndU;
    float StartV;
    float EndV;
    int32 DrawOffsetX;
    int32 DrawOffsetY;
    uint32 DrawWidth;
    uint32 DrawHeight;
    float Advance;
    uint32 TextureIndex;
    bool IsWhiteSpace;
};

struct DF_FONT_TEXTURE
{
    uint32 TextureSize;
    uint32 TextureType;
};

//--------------------------------------- .staticmesh file -------------------------------------
#define DF_STATICMESH_HEADER_MAGIC ((uint32)'STM2')

enum DF_STATICMESH_VERTEX_FLAGS
{
    DF_STATICMESH_VERTEX_FLAG_COLOR             = (1 << 0),
    DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT2   = (1 << 1),
    DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT3   = (1 << 2),
};

struct DF_STATICMESH_HEADER
{
    uint32 Magic;
    uint32 Size;
    uint64 CreationTime;
    float3 BoundingBoxMin;
    float3 BoundingBoxMax;
    float3 BoundingSphereCenter;
    float BoundingSphereRadius;
    uint32 CollisionShapeType;
    uint32 CollisionShapeSize;
    uint32 CollisionShapeOffset;
    uint32 VertexFlags;
    uint32 MaterialCount;
    uint32 MaterialNamesOffset;
    uint32 LODCount;
    uint32 LODOffsetsOffset;
};

struct DF_STATICMESH_LOD_HEADER
{
    uint32 VertexCount;
    uint32 VerticesOffset;
    uint32 IndexCount;
    uint32 IndexFormat;
    uint32 IndicesOffset;
    uint32 BatchCount;
    uint32 BatchesOffset;
};

struct DF_STATICMESH_VERTEX
{
    float Position[3];
    float Tangent[3];
    float Binormal[3];
    float Normal[3];
    float TexCoord[3];
    uint32 Color;
};

struct DF_STATICMESH_BATCH
{
    uint32 MaterialIndex;
    uint32 StartIndex;
    uint32 NumIndices;
};

//--------------------------------------- .skl file -------------------------------------
#define DF_SKELETON_HEADER_MAGIC ((uint32)'SKL2')

struct DF_SKELETON_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 BoneCount;
    uint32 BonesOffset;
};

struct DF_SKELETON_BONE
{
    uint32 BoneSize;
    uint32 BoneNameLength;
    uint32 ParentBoneIndex;
    float RelativeBaseFrameTransformPosition[3];
    float RelativeBaseFrameTransformRotation[4];
    float RelativeBaseFrameTransformScale[3];
    float AbsoluteBaseFrameTransformPosition[3];
    float AbsoluteBaseFrameTransformRotation[4];
    float AbsoluteBaseFrameTransformScale[3];
    uint32 ChildBoneCount;
    // <uint32> * ChildBoneCount follows
};

//--------------------------------------- .skm file -------------------------------------
#define DF_SKELETALMESH_HEADER_MAGIC ((uint32)'SKM1') // YTEX

enum DF_SKELETALMESH_CHUNK
{
    DF_SKELETALMESH_CHUNK_HEADER,
    DF_SKELETALMESH_CHUNK_SKELETON,
    DF_SKELETALMESH_CHUNK_BONES,
    DF_SKELETALMESH_CHUNK_BONE_REFS,
    DF_SKELETALMESH_CHUNK_MATERIALS,
    DF_SKELETALMESH_CHUNK_VERTICES,
    DF_SKELETALMESH_CHUNK_INDICES,
    DF_SKELETALMESH_CHUNK_BATCHES,
    DF_SKELETALMESH_CHUNK_COLLISION_SHAPE,
    DF_SKELETALMESH_CHUNK_COUNT,
};

enum DF_SKELETALMESH_FLAGS
{
    DF_SKELETALMESH_FLAG_USE_TEXTURE_COORDINATES    = (1 << 0),
    DF_SKELETALMESH_FLAG_USE_VERTEX_COLORS          = (1 << 1),
};

struct DF_SKELETALMESH_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 Flags;
    float BoundingBoxMin[3];
    float BoundingBoxMax[3];
    float BoundingSphereCenter[3];
    float BoundingSphereRadius;
    uint32 MaterialCount;
    uint32 SkeletonBoneCount;
    uint32 BoneCount;
    uint32 BoneRefCount;
    uint32 VertexCount;
    uint32 IndexCount;
    uint32 BatchCount;
    uint32 CollisionShapeType;
};

struct DF_SKELETALMESH_SKELETON
{
    uint32 SkeletonNameStringIndex;
};

struct DF_SKELETALMESH_MATERIAL
{
    uint32 MaterialNameStringIndex;
};

struct DF_SKELETALMESH_VERTEX
{
    float Position[3];
    float Tangent[3];
    float Binormal[3];
    float Normal[3];
    float TextureCoordinates[3];
    uint32 Color;
    uint8 BoneIndices[4];
    float BoneWeights[4];
};

struct DF_SKELETALMESH_BONE
{
    uint32 SkeletonBoneIndex;
    float LocalToBonePosition[3];
    float LocalToBoneRotation[4];
    float LocalToBoneScale[3];
};

struct DF_SKELETALMESH_BATCH
{
    uint32 MaterialIndex;
    uint32 WeightCount;
    uint32 BaseBoneRef;
    uint32 BoneRefCount;
    uint32 BaseVertex;
    uint32 VertexCount;
    uint32 StartIndex;
    uint32 IndexCount;
};

//--------------------------------------- .ska file -------------------------------------
#define DF_SKELETALANIMATION_HEADER_MAGIC ((uint32)'ANM3')

struct DF_SKELETALANIMATION_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 Flags;
    uint32 SkeletonNameLength;
    uint32 SkeletonNameOffset;
    uint32 SkeletonBoneCount;
    float Duration;
    uint32 BoneTrackCount;
    uint32 BoneTrackOffset;
    uint32 RootMotionOffset;
};

struct DF_SKELETALANIMATION_TRANSFORM_TRACK_KEYFRAME
{
    float Time;
    float Position[3];
    float Rotation[4];
    float Scale[3];
};

struct DF_SKELETALANIMATION_BONE_TRACK_HEADER
{
    uint32 BoneIndex;
    float Duration;
    uint32 KeyFrameCount;
};

struct DF_SKELETALANIMATION_ROOT_MOTION_TRACK_HEADER
{
    float Duration;
    uint32 KeyFrameCount;
};

//--------------------------------------- .tex file -------------------------------------
#define DF_TEXTURE_HEADER_MAGIC 0x58455459 // YTEX

struct DF_TEXTURE_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 TextureType;
    uint32 TexturePlatform;
    uint32 TextureUsage;
    uint32 TextureFilter;
    uint32 BlendingMode;
    uint32 AddressModeU;
    uint32 AddressModeV;
    uint32 AddressModeW;
    int32 MinLOD;
    int32 MaxLOD;
    uint32 PixelFormat;
    uint32 ArraySize;
    uint32 Width;
    uint32 Height;
    uint32 Depth;
    uint32 MipLevels;
    uint32 ImageCount;
};

struct DF_TEXTURE_IMAGE_HEADER
{
    uint32 Size;
    uint32 RowPitch;
    uint32 SlicePitch;
};

//--------------------------------------- .mtl file ---------------------------------------
#define DF_MATERIAL_HEADER_MAGIC ((uint32)'MTL1')

struct DF_MATERIAL_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 ShaderNameLength;
    uint32 UniformParameterCount;
    uint32 TextureParameterCount;
    uint32 StaticSwitchMask;
    // <char> * ShaderNameLength immediately follows
};

struct DF_MATERIAL_UNIFORM_PARAMETER
{
    uint32 UniformType;
    byte UniformValue[16];     // needs to be as large as MaterialShader::UniformParameter::Value
};

struct DF_MATERIAL_TEXTURE_PARAMETER
{
    uint32 TextureType;
    uint32 TextureNameLength;
    // <char> * TextureNameLength immediately follows
};

//--------------------------------------- .msh file ---------------------------------------
#define DF_MATERIAL_SHADER_HEADER_MAGIC ((uint32)'MSH1')

struct DF_MATERIAL_SHADER_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 BlendingMode;
    uint32 LightingType;
    uint32 LightingModel;
    uint32 LightingNormalSpace;
    uint32 RenderMode;
    uint32 RenderLayer;
    bool TwoSided;
    bool DepthClamping;
    bool DepthTests;
    bool DepthWrites;
    bool CastShadows;
    bool ReceiveShadows;
    uint32 SourceCRC;
    uint32 UniformParameterCount;
    uint32 TextureParameterCount;
    uint32 StaticSwitchParameterCount;
};

struct DF_MATERIAL_SHADER_UNIFORM_PARAMETER
{
    uint32 NameLength;
    uint32 Type;
    byte DefaultValue[16];     // needs to be as large as MaterialShader::UniformParameter::Value
    // <char> * NameLength immediately follows
};

struct DF_MATERIAL_SHADER_TEXTURE_PARAMETER
{
    uint32 NameLength;
    uint32 Type;
    uint32 DefaultValueLength;
    // <char> * NameLength immediately follows
    // <char> * DefaultValueLength immediately follows
};

struct DF_MATERIAL_SHADER_STATIC_SWITCH_PARAMETER
{
    uint32 NameLength;
    uint32 Mask;
    bool DefaultValue;
    // <char> * NameLength immediately follows
};

//--------------------------------------- .shg file ---------------------------------------
#define DF_SHADER_GRAPH_HEADER_MAGIC ((uint32)'SHG1')

//------------------------------------ .blp file ------------------------------------
#define DF_BLOCK_PALETTE_HEADER_MAGIC 0x4C425659 // YSHG

enum DF_BLOCK_PALETTE_CHUNK_TYPE
{
    DF_BLOCK_PALETTE_CHUNK_BLOCK_TYPES,
    DF_BLOCK_PALETTE_CHUNK_TEXTURES,
    DF_BLOCK_PALETTE_CHUNK_MATERIALS,
    DF_BLOCK_PALETTE_CHUNK_MESHES,
    DF_BLOCK_PALETTE_CHUNK_COUNT,
};

struct DF_BLOCK_PALETTE_LIST_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 BlockTypeCount;
    uint32 TextureCount;
    uint32 MaterialCount;
    uint32 MeshCount;
};

struct DF_BLOCK_PALETTE_BLOCK_TYPE
{
    uint32 BlockTypeIndex;
    uint32 NameStringIndex;
    uint32 Flags;
    uint32 ShapeType;

    struct VisualParameters
    {
        uint32 Type;
        uint32 MaterialIndex;
        uint32 Color;
        float MinUV[3];
        float MaxUV[3];
        float AtlasUVRange[4];
    };

    struct CubeShapeFace
    {
        VisualParameters Visual;
    };

    struct SlabShape
    {
        float Height;
    };

    struct PlaneShape
    {
        VisualParameters Visual;
        float OffsetX;
        float OffsetY;
        float Width;
        float Height;
        float BaseRotation;
        uint32 RepeatCount;
        float RepeatRotation;
    };

    struct MeshShape
    {
        uint32 MeshIndex;
        float Scale;
    };

    struct BlockLightEmitter
    {
        uint32 Radius;
    };

    struct PointLightEmitter
    {
        float Offset[3];
        uint32 Color;
        float Brightness;
        float Range;
        float Falloff;
    };

    CubeShapeFace CubeShapeFaces[CUBE_FACE_COUNT];
    SlabShape SlabSettings;
    PlaneShape PlaneSettings;
    MeshShape MeshSettings;
    BlockLightEmitter BlockLightEmitterSettings;
    PointLightEmitter PointLightEmitterSettings;
};

struct DF_BLOCK_PALETTE_TEXTURE
{
    uint32 TextureOffset;       // relative to this chunk
    uint32 TextureSize;
};

enum DF_BLOCK_PALETTE_MATERIAL_TYPE
{
    DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_COLOR,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_COLOR,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS_WITH_NORMAL_MAP,
    DF_BLOCK_PALETTE_MATERIAL_TYPE_COUNT,
};

enum DF_BLOCK_PALETTE_MATERIAL_FLAGS
{
    DF_BLOCK_PALETTE_MATERIAL_FLAG_SCROLLED_TEXTURE     = (1 << 0),
    DF_BLOCK_PALETTE_MATERIAL_FLAG_ANIMATED_TEXTURE     = (1 << 1),
};

struct DF_BLOCK_PALETTE_MATERIAL
{
    uint32 MaterialType;
    uint32 MaterialFlags;
    uint32 MaterialNameStringIndex;

    uint32 DiffuseMapTextureIndex;
    uint32 SpecularMapTextureIndex;
    uint32 NormalMapTextureIndex;

    float TextureScrollVector[2];
};

struct DF_BLOCK_PALETTE_MESH
{
    uint32 MeshNameStringIndex;
};

//------------------------------------ .blocklist file ------------------------------------
#define DF_BLOCK_TERRAIN_BLOCK_LIST_HEADER_MAGIC 0x4C425660 // YSHG

enum DF_BLOCK_TERRAIN_BLOCK_LIST_CHUNK_TYPE
{
    DF_BLOCK_TERRAIN_BLOCK_LIST_CHUNK_BLOCK_TYPES,
    DF_BLOCK_TERRAIN_BLOCK_LIST_CHUNK_TEXTURES,
    DF_BLOCK_TERRAIN_BLOCK_LIST_CHUNK_MATERIALS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_CHUNK_COUNT,
};

struct DF_BLOCK_TERRAIN_BLOCK_LIST_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    float BlockScale;
    uint32 BlockTypeCount;
    uint32 TextureCount;
    uint32 MaterialCount;
};

struct DF_BLOCK_TERRAIN_BLOCK_LIST_BLOCK_TYPE
{
    uint32 BlockTypeIndex;
    uint32 NameStringIndex;
    uint32 Flags;
    uint32 ShapeType;

    struct CubeShapeFace
    {
        uint32 VisualType;
        uint32 MaterialIndex;
        uint32 SilhouetteMaterialIndex;
        uint32 Color;
        float MinUV[3];
        float MaxUV[3];
        float AtlasUVRange[4];
    };

    CubeShapeFace CubeShapeFaces[CUBE_FACE_COUNT];
};

struct DF_BLOCK_TERRAIN_BLOCK_LIST_TEXTURE
{
    uint32 TextureNameStringIndex;
    uint32 TextureOffset;       // relative to this chunk
    uint32 TextureSize;
};

enum DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE
{
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_EXTERNAL,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_STATIC_COLOR,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_STATIC_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_STATIC_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_STATIC_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_SCROLLED_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_SCROLLED_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_SCROLLED_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_SILHOUETTE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_COLOR,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SCROLLED_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SCROLLED_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SCROLLED_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_STATIC_COLOR,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_STATIC_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_STATIC_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_STATIC_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_SCROLLED_TEXTURE,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_SCROLLED_TEXTURE_ARRAY,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SILHOUETTE_SCROLLED_TEXTURE_ATLAS,
    DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL_TYPE_COUNT,
};

struct DF_BLOCK_TERRAIN_BLOCK_LIST_MATERIAL
{
    uint32 MaterialNameStringIndex;
    uint32 MaterialType;

    uint32 AutoGenDiffuseMapTextureIndex;
    uint32 AutoGenSpecularMapTextureIndex;
    uint32 AutoGenNormalMapTextureIndex;

    float AutoGenTextureScrollVector[2];
};

//------------------------------------ .blocklist file ------------------------------------
#define DF_TERRAIN_LAYER_LIST_HEADER_MAGIC 0x4C425660 // YSHG

enum DF_TERRAIN_LAYER_LIST_CHUNK_TYPE
{
    DF_TERRAIN_LAYER_LIST_CHUNK_TEXTURES,
    DF_TERRAIN_LAYER_LIST_CHUNK_MATERIALS,
    DF_TERRAIN_LAYER_LIST_CHUNK_BASE_LAYERS,
    DF_TERRAIN_LAYER_LIST_CHUNK_COUNT,
};

struct DF_TERRAIN_LAYER_LIST_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 TextureCount;
    uint32 BaseLayerArraySize;
    uint32 BaseLayerCount;
    int32 CombinedBaseLayerBaseTextureIndex;
    int32 CombinedBaseLayerNormalTextureIndex;
};

struct DF_TERRAIN_LAYER_LIST_TEXTURE
{
    uint32 TextureOffset;       // relative to this chunk
    uint32 TextureSize;
};

struct DF_TERRAIN_LAYER_LIST_BASE_LAYER
{
    uint32 LayerIndex;
    uint32 NameStringIndex;
    uint32 TextureRepeatInterval;
    
    int32 BaseTextureIndex;
    int32 BaseTextureArrayIndex;

    int32 NormalTextureIndex;
    int32 NormalTextureArrayIndex;
};

#define DF_TERRAIN_SECTION_HEADER_MAGIC 0x4C425660

struct DF_TERRAIN_SECTION_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 PointCount;
    uint32 HeightMapValueSize;
    uint32 HeightMapRowPitch;
    uint32 SplatMapCount;
};

struct DF_TERRAIN_SECTION_SPLAT_MAP_HEADER
{
    uint8 Layers[4];
    uint32 LayerCount;
    uint32 ChannelCount;
    uint32 RowPitch;
};

#define DF_TERRAIN_QUADTREE_HEADER_MAGIC 0x4C425680

struct DF_TERRAIN_QUADTREE_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 LODCount;
    uint32 NodeCount;
};

struct DF_TERRAIN_QUADTREE_NODE
{
    uint32 LODLevel;
    uint32 StartQuadX;
    uint32 StartQuadY;
    uint32 NodeSize;
    float BoundingBoxMin[3];
    float BoundingBoxMax[3];
    float BoundingSphereCenter[3];
    float BoundingSphereRadius;
    uint8 IsLeafNode;
    uint8 IsFlat;
    int32 ChildNodeIndices[4];
};

//--------------------------------- .staticblockmesh file ---------------------------------
#define DF_BLOCK_MESH_HEADER_MAGIC ((uint32)'BLM1') 

enum DF_STATIC_BLOCK_MESH_CHUNK_TYPE
{
    DF_STATIC_BLOCK_MESH_CHUNK_EMBEDDED_BLOCKLIST,
    DF_STATIC_BLOCK_MESH_CHUNK_BLOCK_DATA,
    DF_STATIC_BLOCK_MESH_CHUNK_VERTICES,
    DF_STATIC_BLOCK_MESH_CHUNK_INDICES,
    DF_STATIC_BLOCK_MESH_CHUNK_BATCHES,
    DF_STATIC_BLOCK_MESH_CHUNK_COUNT,
};

struct DF_STATIC_BLOCK_MESH_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 BlockListNameStringIndex;
    int32 MinCoordinates[3];
    int32 MaxCoordinates[3];
    uint32 WidthInBlocks;
    uint32 LengthInBlocks;
    uint32 HeightInBlocks;
    float MinBounds[3];
    float MaxBounds[3];
    uint32 VertexCount;
    uint32 IndexCount;
    uint32 IndexSize;
    uint32 BatchCount;
};

struct DF_STATIC_BLOCK_MESH_VERTEX
{
    float Position[3];
    float Tangent[3];
    float Binormal[3];
    float Normal[3];
    float TexCoord[3];
    float AtlasTexCoord[4];
    uint32 Color;
};

struct DF_STATIC_BLOCK_MESH_BATCH
{
    uint32 MaterialIndex;
    uint32 StartIndex;
    uint32 IndexCount;
    uint32 MinVertexIndex;
    uint32 MaxVertexIndex;
};

//--------------------------------- .particlesystem file ---------------------------------
#define DF_PARTICLE_SYSTEM_HEADER_MAGIC ((uint32)'PTS1') 

struct DF_PARTICLE_SYSTEM_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 ClassTableOffset;
    uint32 ClassTableSize;
    uint32 EmitterOffset;
    uint32 EmitterCount;
};

struct DF_PARTICLE_SYSTEM_EMITTER_HEADER
{
    uint32 TotalSize;
    uint32 TypeIndex;
    uint32 PropertiesOffset;
    uint32 ModuleOffset;
    uint32 ModuleCount;
};

struct DF_PARTICLE_SYSTEM_MODULE_HEADER
{
    uint32 TotalSize;
    uint32 TypeIndex;
};

//---------------------------------  shader program ---------------------------------
#define DF_SHADER_PROGRAM_COMMON_HEADER_MAGIC 0x45454545
struct DF_SHADER_PROGRAM_COMMON_HEADER
{
    uint32 Magic;
    uint32 ShaderStoreCRC;
    uint32 BaseShaderParameterCRC;
    uint32 VertexFactoryParameterCRC;
    uint32 MaterialShaderCRC;
};

//--------------------------------- .map file ---------------------------------
//#define DF_MAP_HEADER_MAGIC 0x50414D59 // YMAP
//#define DF_MAP_TERRAIN_HEADER_MAGIC 0x50414D60 // YMAP
//#define DF_MAP_BLOCK_TERRAIN_HEADER_MAGIC 0x50414D61 // YMAP
#define DF_MAP_HEADER_FILENAME "map.dat"
#define DF_MAP_HEADER_VERSION 7
#define DF_MAP_REGIONS_HEADER_FILENAME "regions.dat"
#define DF_MAP_TERRAIN_HEADER_FILENAME "terrain.dat"
#define DF_MAP_TERRAIN_LAYERS_FILENAME_PREFIX "terrain.layers"
#define DF_MAP_BLOCK_TERRAIN_HEADER_FILENAME "block_terrain.dat"
#define DF_MAP_GLOBAL_ENTITIES_FILENAME "global_entities.dat"
#define DF_MAP_CLASS_TABLE_FILENAME "class_table.dat"

// lives in 'map.dat'
struct DF_MAP_HEADER
{
    uint32 HeaderSize;
    uint32 Version;
    uint32 RegionSize;
    uint32 RegionLODLevels;
    uint32 RegionLoadRadius;
    uint32 RegionActivationRadius;
    float WorldBoundingBoxMin[3];
    float WorldBoundingBoxMax[3];
    uint32 HasRegions;
    uint32 HasTerrain;
    uint32 HasGlobalEntities;

    // if HasTerrain
    // DF_MAP_TERRAIN_HEADER x 1

    // <int2> x RegionCount

    // <byte> x GlobalEntityDataSize
};

struct DF_MAP_REGIONS_HEADER
{
    uint32 HeaderSize;
    uint32 RegionCount;
};

struct DF_MAP_TERRAIN_HEADER
{
    uint32 HeaderSize;

    uint32 HeightStorageFormat;
    int32 MinHeight;
    int32 MaxHeight;
    int32 BaseHeight;
    uint32 Scale;
    uint32 SectionSize;
    uint32 LODCount;
};

struct DF_MAP_BLOCK_TERRAIN_HEADER
{
    uint32 HeaderSize;
    uint32 PaletteNameLength;
    uint32 Scale;
    uint32 SectionSize;
    uint32 ChunkSize;
    uint32 StorageLODCount;
    int32 MinSectionX;
    int32 MinSectionY;
    int32 MaxSectionX;
    int32 MaxSectionY;
    uint32 SectionCount;

    // <char> x PaletteNameLength follows immediately after, then
    // <int2> x SectionCount
};

struct DF_MAP_GLOBAL_ENTITIES_HEADER
{
    uint32 HeaderSize;
    uint32 EntityCount;
    uint32 EntityDataSize;
};

struct DF_MAP_REGION_HEADER
{
    uint32 HeaderSize;
    uint32 LODLevel;
    int32 RegionX;
    int32 RegionY;
    uint32 TerrainSectionCount;
    uint32 TerrainDataSize;
    uint32 EntityCount;
    uint32 EntityDataSize;
};

struct DF_MAP_REGION_TERRAIN_SECTION_HEADER
{
    int32 SectionX;
    int32 SectionY;
    uint32 LODLevel;
    uint32 DataSize;

    // <byte> x DataSize follows
};

struct DF_MAP_ENTITY_HEADER
{
    uint32 HeaderSize;
    uint32 EntityNameLength;
    uint32 EntityTypeIndex;
    uint32 EntitySize;
    uint32 ComponentsSize;
    uint32 ComponentCount;
};

struct DF_MAP_ENTITY_COMPONENT_HEADER
{
    uint32 ComponentSize;
    uint32 ComponentNameLength;
    uint32 ComponentTypeIndex;
};

#ifdef Y_COMPILER_MSVC
    #pragma warning(pop)
#endif
#pragma pack(pop)
