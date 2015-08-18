#pragma once
#include "Engine/Common.h"

class Resource;
class Texture;
class Texture2D;
class Texture2DArray;
class TextureCube;
class MaterialShader;
class Material;
class StaticMesh;
class Font;
class BlockPalette;
class TerrainLayerList;
class BlockMesh;
class Skeleton;
class SkeletalMesh;
class SkeletalAnimation;
class ShaderGraphSchema;
class ShaderGraph;
class ParticleSystem;
class ResourceCompilerInterface;

class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    static const ResourceTypeInfo *GetResourceTypeForFile(const char *FileName);
    static const ResourceTypeInfo *GetResourceTypeForStream(const char *FileName, ByteStream *pStream);

    static const ResourceTypeInfo *GetResourceTypeForFile(const char *FileName, String &resourceName);
    static const ResourceTypeInfo *GetResourceTypeForStream(const char *FileName, ByteStream *pStream, String &resourceName);

    const Resource *GetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *name);
    const Texture *GetTexture(const char *name);
    const Texture2D *GetTexture2D(const char *name);
    const Texture2DArray *GetTexture2DArray(const char *name);
    const TextureCube *GetTextureCube(const char *name);
    const MaterialShader *GetMaterialShader(const char *name);
    const Material *GetMaterial(const char *name);
    const Font *GetFont(const char *name);
    const BlockPalette *GetBlockPalette(const char *name);
    const TerrainLayerList *GetTerrainLayerList(const char *name);
    const StaticMesh *GetStaticMesh(const char *name);
    const BlockMesh *GetBlockMesh(const char *name);
    const Skeleton *GetSkeleton(const char *name);
    const SkeletalMesh *GetSkeletalMesh(const char *name);
    const SkeletalAnimation *GetSkeletalAnimation(const char *name);
    const ParticleSystem *GetParticleSystem(const char *name);

    // safe resource getters, that return the default value if the resource is not found
    const Resource *SafeGetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *name);
    const Texture2D *SafeGetTexture2D(const char *name);
    const Texture2DArray *SafeGetTexture2DArray(const char *name);
    const TextureCube *SafeGetTextureCube(const char *name);
    const MaterialShader *SafeGetMaterialShader(const char *name);
    const Material *SafeGetMaterial(const char *name);
    const StaticMesh *SafeGetStaticMesh(const char *name);
    const BlockMesh *SafeGetBlockMesh(const char *name);
    const SkeletalMesh *SafeGetSkeletalMesh(const char *name);

    // don't cache the resource when returning it, if it wasn't already loaded
    const Resource *UncachedGetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *name);
    const Texture *UncachedGetTexture(const char *name);
    const Material *UncachedGetMaterial(const char *name);
    const MaterialShader *UncachedGetMaterialShader(const char *name);
    const Font *UncachedGetFont(const char *name);
    const BlockPalette *UncachedGetBlockPalette(const char *name);
    const TerrainLayerList *UncachedGetTerrainLayerList(const char *name);
    const StaticMesh *UncachedGetStaticMesh(const char *name);
    const BlockMesh *UncachedGetBlockMesh(const char *name);
    const Skeleton *UncachedGetSkeleton(const char *name);
    const SkeletalMesh *UncachedGetSkeletalMesh(const char *name);
    const SkeletalAnimation *UncachedGetSkeletalAnimation(const char *name);
    const ParticleSystem *UncachedGetParticleSystem(const char *name);

    void CreateDeviceResources();
    void ReleaseDeviceResources();

    void CompactResources();
    void ReleaseResources();

    const Texture *GetDefaultTexture(TEXTURE_TYPE TextureType);
    const Texture2D *GetDefaultTexture2D();
    const Texture2DArray *GetDefaultTexture2DArray();
    const TextureCube *GetDefaultTextureCube();
    const MaterialShader *GetDefaultMaterialShader();
    const Material *GetDefaultMaterial();
    const StaticMesh *GetDefaultStaticMesh();
    const BlockMesh *GetDefaultBlockMesh();
    const SkeletalMesh *GetDefaultSkeletalMesh();

    // resource modification detection
    bool IsResourceModificationDetectionEnabled() const { return (m_pResourceModificationChangeNotifier != nullptr); }
    void SetResourceModificationDetectionEnabled(bool enabled);
    void CheckForModifiedResources();

    // resource compiler interface
    ResourceCompilerInterface *GetResourceCompilerInterface();
    void ReleaseResourceCompilerInterface(ResourceCompilerInterface *pInterface);

    // resource maintenance, call every frame, or something close to that
    void Update();

private:
    // resource loaders
    Texture *LoadTexture(const char *name);
    Material *LoadMaterial(const char *name);
    MaterialShader *LoadMaterialShader(const char *name);
    Font *LoadFont(const char *name);
    BlockPalette *LoadBlockPalette(const char *name);
    TerrainLayerList *LoadTerrainLayerList(const char *name);
    StaticMesh *LoadStaticMesh(const char *name);
    BlockMesh *LoadBlockMesh(const char *name);
    Skeleton *LoadSkeleton(const char *name);
    SkeletalMesh *LoadSkeletalMesh(const char *name);
    SkeletalAnimation *LoadSkeletalAnimation(const char *name);
    ParticleSystem *LoadParticleSystem(const char *name);

    // resource lock
    ReadWriteLock m_resourceLock;

    // resource storage
    typedef CIStringHashTable<Texture *> TextureTable;
    TextureTable m_htTextures;

    typedef CIStringHashTable<MaterialShader *> MaterialShaderTable;
    MaterialShaderTable m_htMaterialShaders;

    typedef CIStringHashTable<Material *> MaterialTable;
    MaterialTable m_htMaterials;

    typedef CIStringHashTable<StaticMesh *> StaticMeshTable;
    StaticMeshTable m_htStaticMeshes;

    typedef CIStringHashTable<Font *> FontTable;
    FontTable m_htFonts;

    typedef CIStringHashTable<BlockPalette *> BlockPaletteTable;
    BlockPaletteTable m_htBlockPalette;

    typedef CIStringHashTable<TerrainLayerList *> TerrainLayerListTable;
    TerrainLayerListTable m_htTerrainLayerList;

    typedef CIStringHashTable<BlockMesh *> BlockMeshTable;
    BlockMeshTable m_htBlockMesh;

    typedef CIStringHashTable<Skeleton *> SkeletonTable;
    SkeletonTable m_htSkeleton;

    typedef CIStringHashTable<SkeletalMesh *> SkeletalMeshTable;
    SkeletalMeshTable m_htSkeletalMesh;

    typedef CIStringHashTable<SkeletalAnimation *> SkeletalAnimationTable;
    SkeletalAnimationTable m_htSkeletalAnimation;

    typedef CIStringHashTable<ParticleSystem *> ParticleSystemTable;
    ParticleSystemTable m_htParticleSystem;

    // default resources
    const Texture2D *m_pDefaultTexture2D;
    const Texture2DArray *m_pDefaultTexture2DArray;
    const TextureCube *m_pDefaultTextureCube;
    const MaterialShader *m_pDefaultMaterialShader;
    const Material *m_pDefaultMaterial;
    const StaticMesh *m_pDefaultStaticMesh;
    const BlockMesh *m_pDefaultBlockMesh;
    const SkeletalMesh *m_pDefaultSkeletalMesh;

    // maintenance timer
    Timer m_lastMaintenanceTime;

    // resource modification detection
    FileSystem::ChangeNotifier *m_pResourceModificationChangeNotifier;

    // resource compiler interface
    ResourceCompilerInterface *m_pResourceCompilerInterface;
    Timer m_resourceCompilerSpawnTime;
    RecursiveMutex m_resourceCompilerLock;
    bool m_resourceCompilerInUse;
};

extern ResourceManager *g_pResourceManager;

