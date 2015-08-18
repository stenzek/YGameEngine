#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/BlockPalette.h"
#include "Core/Image.h"
#include "YBaseLib/ProgressCallbacks.h"

class ZipArchive;
class XMLReader;
class XMLWriter;

class BlockPaletteGenerator
{
public:
    struct BlockType
    {
        uint32 BlockTypeIndex;
        String Name;
        uint32 Flags;
        BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE ShapeType;

        struct VisualParameters
        {
            BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE Type;
            uint32 Color;
            uint32 TextureIndex;
            String MaterialName;
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
            String MeshName;
            float Scale;
        };

        struct BlockLightEmitter
        {
            bool Enabled;
            uint32 Radius;
        };

        struct PointLightEmitter
        {
            bool Enabled;
            float3 Offset;
            uint32 Color;
            float Brightness;
            float Range;
            float Falloff;
        };

        CubeShapeFace CubeShapeFaces[CUBE_FACE_COUNT];
        SlabShape SlabShapeSettings;
        PlaneShape PlaneShapeSettings;
        MeshShape MeshShapeSettings;
        BlockLightEmitter BlockLightEmitterSettings;
        PointLightEmitter PointLightEmitterSettings;
    };

    struct Texture
    {
        uint32 Index;
        String Name;

        BLOCK_MESH_TEXTURE_BLENDING Blending;
        BLOCK_MESH_TEXTURE_EFFECT Effect;
        bool AllowTextureFiltering;
        float2 ScrollDirection;
        float ScrollSpeed;
        uint32 AnimationFrameCount;
        float AnimationSpeed;

        Image *pDiffuseMap;
        Image *pSpecularMap;
        Image *pNormalMap;
        //uint32 TextureCount;
    };

public:
    BlockPaletteGenerator();
    ~BlockPaletteGenerator();

    // creation
    void Create(uint32 diffuseMapSize, uint32 specularMapSize, uint32 normalMapSize);
    bool Load(const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // output
    bool Save(ByteStream *pStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback) const;

    // map resolutions
    const uint32 GetDiffuseMapSize() const { return m_diffuseMapSize; }
    const uint32 GetSpecularMapSize() const { return m_specularMapSize; }
    const uint32 GetNormalMapSize() const { return m_normalMapSize; }
    void SetDiffuseMapSize(uint32 size) { DebugAssert(size > 0); m_diffuseMapSize = size; }
    void SetSpecularMapSize(uint32 size) { DebugAssert(size > 0); m_specularMapSize = size; }
    void SetNormalMapSize(uint32 size) { DebugAssert(size > 0); m_normalMapSize = size; }

    // block type accessing
    const BlockType *GetBlockType(uint32 i) const { DebugAssert(i < BLOCK_MESH_MAX_BLOCK_TYPES); return (m_allocatedBlockTypes[i]) ? &m_blockTypes[i] : NULL; }
    BlockType *GetBlockType(uint32 i) { DebugAssert(i < BLOCK_MESH_MAX_BLOCK_TYPES); return (m_allocatedBlockTypes[i]) ? &m_blockTypes[i] : NULL; }

    // find by name
    const BlockType *GetBlockTypeByName(const char *name) const;
    BlockType *GetBlockTypeByName(const char *name);
    int32 FindBlockTypeByName(const char *name) const;

    // block management
    BlockType *CreateBlockType(const char *blockName, BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType, uint32 flags);
    BlockType *CreateBlockType(uint32 blockTypeIndex, const char *blockName, BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType, uint32 flags);
    void RemoveBlockType(uint32 blockTypeIndex);

    // texture lookup
    const Texture *GetTexture(uint32 i) const { DebugAssert(i < m_textures.GetSize()); return m_textures[i]; }
    const Texture *GetTextureByName(const char *name) const;
    const uint32 GetTextureCount() const { return m_textures.GetSize(); }

    // texture management
    const Texture *CreateTexture(const char *textureName, BLOCK_MESH_TEXTURE_BLENDING blending, bool allowTextureFiltering);
    bool SetTextureDiffuseMap(uint32 textureIndex, const Image *pImage);
    bool SetTextureSpecularMap(uint32 textureIndex, const Image *pImage);
    bool SetTextureNormalMap(uint32 textureIndex, const Image *pImage);
    void SetTextureBlending(uint32 textureIndex, BLOCK_MESH_TEXTURE_BLENDING blending);
    void SetTextureEffectNone(uint32 textureIndex);
    void SetTextureEffectScrolled(uint32 textureIndex, const float2 &scrollDirection, float scrollSpeed);
    void SetTextureEffectAnimation(uint32 textureIndex, uint32 animationFrameCount, float animationSpeed);

    // texture atlas importing
    bool ImportTextureAtlas(const char *namePrefix, const char *diffuseMapFileName, const char *specularMapFileName, const char *normalMapFileName, uint32 tilesWide, uint32 tilesHigh, const bool *pImportTileIndices, uint32 maxImportTileIndices, String *pOutTextureNames, uint32 maxOutTextureNames);

    // compiler
    bool Compile(TEXTURE_PLATFORM texturePlatform, ByteStream *pOutputStream) const;

    // assignment operator
    BlockPaletteGenerator &operator=(const BlockPaletteGenerator &copyFrom);

private:
    bool LoadXML(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks);
    bool LoadTextures(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks);
    bool SaveXML(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const;
    bool SaveTextures(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const;
    bool SaveTexture(ZipArchive *pArchive, const Image *pImage, const char *filename) const;
    static Image *LoadTextureImage(ByteStream *pStream, const char *filename);

    bool m_allocatedBlockTypes[BLOCK_MESH_MAX_BLOCK_TYPES];
    BlockType m_blockTypes[BLOCK_MESH_MAX_BLOCK_TYPES];
    uint32 m_diffuseMapSize;
    uint32 m_specularMapSize;
    uint32 m_normalMapSize;

    PODArray<Texture *> m_textures;
};

