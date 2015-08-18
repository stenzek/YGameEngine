#pragma once
#include "Engine/Common.h"
#include "Engine/Texture.h"
#include "Engine/Material.h"

#define TERRAIN_MAX_LAYERS (64)
#define TERRAIN_MAX_STORAGE_LODS (4)
#define TERRAIN_MAX_RENDER_LODS (10)

struct TerrainLayerListBaseLayer
{
    uint32 Index;
    bool Allocated;
    String Name;
    uint32 TextureRepeatInterval;

    int32 BaseTextureIndex;
    int32 BaseTextureArrayIndex;

    int32 NormalTextureIndex;
    int32 NormalTextureArrayIndex;
};

class TerrainLayerList : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(TerrainLayerList, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(TerrainLayerList);
   
public:
    TerrainLayerList(const ResourceTypeInfo *pTypeInfo = &s_TypeInfo);
    ~TerrainLayerList();

    const TerrainLayerListBaseLayer *GetBaseLayer(uint32 index) const { DebugAssert(index < m_baseLayerArraySize); return (m_pBaseLayers[index].Allocated) ? &m_pBaseLayers[index] : nullptr; }
    const TerrainLayerListBaseLayer *GetBaseLayerByName(const char *name) const;
    const uint32 GetBaseLayerArraySize() const { return m_baseLayerArraySize; }

    const Texture *GetTexture(uint32 index) const { DebugAssert(index < m_textureCount); return m_ppTextures[index]; }
    const uint32 GetTextureCount() const { return m_textureCount; }

    // create a material that can be used to render these layers, specify -1 to not render a layer
    const Material *CreateBaseLayerRenderMaterial(const int32 *pLayerIndices, uint32 nLayerIndices, GPUTexture2D *pNormalMapTexture, GPUTexture **pWeightTextures, uint32 nWeightTextures) const;

    // find the first available layer
    uint32 GetFirstLayerIndex() const;

    // serialization
    bool Load(const char *FileName, ByteStream *pStream);

    // gpu resources
    bool CreateGPUResources();
    void ReleaseGPUResources();

private:
    TerrainLayerListBaseLayer *m_pBaseLayers;
    uint32 m_baseLayerArraySize;

    const Texture **m_ppTextures;
    uint32 m_textureCount;

    int32 m_combinedBaseLayerBaseTextureIndex;
    int32 m_combinedBaseLayerNormalTextureIndex;
};
