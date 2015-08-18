#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/TerrainLayerList.h"

class Image;
class ZipArchive;
class XMLReader;
class XMLWriter;

class TerrainLayerListGenerator
{
    friend class TerrainLayerListCompiler;

public:
    struct BaseLayer
    {
        uint32 Index;
        bool Allocated;
        String Name;
        uint32 TextureRepeatInterval;

        Image *pBaseMap;
        Image *pNormalMap;
    };

public:
    TerrainLayerListGenerator();
    ~TerrainLayerListGenerator();

    // creation
    void Create(uint32 diffuseMapSize, uint32 normalMapSize);
    void CreateCopy(const TerrainLayerListGenerator *pGenerator);
    bool Load(const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // output
    bool Save(ByteStream *pStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback) const;

    // accessors
    const uint32 GetBaseLayerBaseMapResolution() const { return m_baseLayerBaseMapResolution; }
    const uint32 GetBaseLayerNormalMapResolution() const { return m_baseLayerNormalMapResolution; }

    // base layer normal mapping
    bool IsBaseLayerNormalMappingEnabled() const { return m_baseLayerNormalMapping; }
    void SetBaseLayerNormalMappingEnabled(bool enabled) { m_baseLayerNormalMapping = enabled; }

    // base layers
    const BaseLayer *GetBaseLayer(uint32 index) const { DebugAssert(index < TERRAIN_MAX_LAYERS); return (m_baseLayers[index].Allocated) ? &m_baseLayers[index] : nullptr; }
    BaseLayer *GetBaseLayer(uint32 index) { DebugAssert(index < TERRAIN_MAX_LAYERS); return (m_baseLayers[index].Allocated) ? &m_baseLayers[index] : nullptr; }
    int32 GetBaseLayerIndexByName(const char *layerName) const;
    const BaseLayer *GetBaseLayerByName(const char *layerName) const;
    BaseLayer *GetBaseLayerByName(const char *layerName);
    BaseLayer *CreateBaseLayer(const char *layerName);
    void DeleteBaseLayer(uint32 index);

    // base layer manipulation
    bool SetBaseLayerBaseMap(uint32 index, const Image *pImage);
    bool SetBaseLayerNormalMap(uint32 index, const Image *pImage);

    // compiler
    bool Compile(ByteStream *pOutputStream) const;

private:
    BaseLayer m_baseLayers[TERRAIN_MAX_LAYERS];

    bool m_baseLayerNormalMapping; 
    uint32 m_baseLayerBaseMapResolution;
    uint32 m_baseLayerNormalMapResolution;

    bool LoadBaseLayers(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks);
    bool LoadBaseLayerTextures(ZipArchive *pArchive, BaseLayer *pBaseLayer, ProgressCallbacks *pProgressCallbacks);
    bool SaveBaseLayers(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const;
    bool SaveBaseLayerTextures(ZipArchive *pArchive, const BaseLayer *pBaseLayer, ProgressCallbacks *pProgressCallbacks) const;
};

