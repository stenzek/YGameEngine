#pragma once
#include "MapCompiler/Common.h"
#include "YBaseLib/ProgressCallbacks.h"

#define BUILDER_BRUSH_ENTITY_ID 0

class MapSource;
class MapSourceEntityData;
class ObjectTemplate;
class ChunkFileWriter;
class ZipArchive;
class ClassTable;
class ClassTableGenerator;

class MapCompiler
{
public:
    MapCompiler(MapSource *pMapSource);
    ~MapCompiler();

    bool StartFreshCompile(ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool StartReuseCompile(ByteStream *pInputStream, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // overrides
    void SetOverrideRegionLODLevels(uint32 lodLevels) { DebugAssert(lodLevels >= 1); m_regionLODLevels = lodLevels; }
    void SetOverrideRegionLoadRadius(uint32 loadRadius) { DebugAssert(loadRadius > 0); m_regionLoadRadius = loadRadius; }
    void SetOverrideRegionActivationRadius(uint32 activationRadius) { DebugAssert(activationRadius > 0); m_regionActivationRadius = activationRadius; }
    
    // build everything for a map, if mapLOD is set to -1, all lods will be built
    bool BuildAll(int32 mapLOD = -1, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // build the global entities
    bool BuildGlobalEntities(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // build the entirety of a region
    bool BuildRegion(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // build the entities component of a region
    bool BuildRegionEntities(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // build the terrain component of a region
    bool BuildRegionTerrain(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // finalize the compile
    bool FinalizeCompile(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // discard the compile results
    void DiscardCompile();

private:
    //////////////////////////////////////////////////////////////////////////
    // LOD LEVEL GENERATION
    //////////////////////////////////////////////////////////////////////////
    bool BuildRegionLOD(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool BuildRegionLODEntities(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool BuildRegionLODTerrain(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

private:
    //////////////////////////////////////////////////////////////////////////
    // HEADER
    //////////////////////////////////////////////////////////////////////////

    // common stuff
    MapSource *m_pMapSource;
    ByteStream *m_pInputStream;
    ByteStream *m_pOutputStream;
    ZipArchive *m_pOutputArchive;

    // class table
    ClassTableGenerator *m_pClassTableGenerator;

    // lod levels
    uint32 m_regionSize;
    uint32 m_regionLODLevels;
    uint32 m_regionLoadRadius;
    uint32 m_regionActivationRadius;

    // world bounding box
    AABox m_worldBoundingBox;
    void UpdateWorldBoundingBox(const AABox &boundingBox);
    void UpdateWorldBoundingBox(const float3 &point);

    //////////////////////////////////////////////////////////////////////////
    // COMPILATION
    //////////////////////////////////////////////////////////////////////////

    // this structure contains a binary block of entity data for a region
    struct RegionEntityData
    {
        RegionEntityData(uint32 entityCount, BinaryBlob *data) : EntityCount(entityCount), pData(data) {}
        ~RegionEntityData() { pData->Release(); }

        uint32 EntityCount;
        BinaryBlob *pData;
    };

    // build region entity data from a list of entities
    bool CompileEntityDataSingle(const MapSourceEntityData *pEntityData, const ObjectTemplate *pTemplate, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks);
    bool CompileEntityData(const MapSourceEntityData *const *ppEntities, uint32 entityCount, RegionEntityData **ppOutData, ProgressCallbacks *pProgressCallbacks);

    // this structure contains a binary block of terrain data for a section inside a region
    struct RegionTerrainSectionData
    {
        RegionTerrainSectionData(int32 sectionX, int32 sectionY, BinaryBlob *data) : SectionX(sectionX), SectionY(sectionY), pData(data) {}
        ~RegionTerrainSectionData() { pData->Release(); }

        int32 SectionX;
        int32 SectionY;
        BinaryBlob *pData;
    };

    // build terrain data for a terrain section inside a region
    bool CompileTerrainSection(int32 sectionX, int32 sectionY, RegionTerrainSectionData **ppOutData);

    // run preparation steps
    bool PrepareSourceForCompiling(ProgressCallbacks *pProgressCallbacks);

    //////////////////////////////////////////////////////////////////////////
    // REGIONS
    //////////////////////////////////////////////////////////////////////////

    // region information
    struct Region
    {
        Region(MapCompiler *pMapCompiler, int32 regionX, int32 regionY, uint32 lodLevel);
        ~Region();

        MapCompiler *pMapCompiler;
        int32 RegionX;
        int32 RegionY;
        uint32 RegionLODLevel;
        bool RegionExistsInCurrentStream;
        bool RegionExistsInNewStream;
        bool RegionLoaded;
        bool RegionChanged;

        // terrain information from the NEW source
        MemArray<int2> NewTerrainSections;

        // terrain information for either freshly compiled source, or current compiled data
        PODArray<RegionTerrainSectionData *> CompiledTerrainData;

        // block terrain information from the NEW source
        MemArray<int2> NewBlockTerrainSections;

        // entity information from the NEW source
        PODArray<const MapSourceEntityData *> NewEntityRefs;

        // entity information for either freshly compiled source, or current compiled data
        RegionEntityData *CompiledEntityData;

        // build new terrain data for this region
        bool RecompileTerrainData(ProgressCallbacks *pProgressCallbacks);

        // build new entity data for this region
        bool RecompileEntityData(ProgressCallbacks *pProgressCallbacks);

        // load any missing data
        bool LoadData();
    };

    // array of regions
    PODArray<Region *> m_regions;

    // helper to obtain a region pointer
    Region *CreateRegion(int32 regionX, int32 regionY, uint32 lodLevel);
    Region *GetRegion(int32 regionX, int32 regionY, uint32 lodLevel);

    // create any regions that exist due to entities, terrain or block terrain
    void CreateRegions(ProgressCallbacks *pProgressCallbacks);
    void CreateRegions_Entities(ProgressCallbacks *pProgressCallbacks);
    void CreateRegions_Terrain(ProgressCallbacks *pProgressCallbacks);

    // save region to the output archive
    bool SaveRegion(Region *pRegion);

    // load regions from current map
    bool SaveMapHeader();

    // other headers
    bool SaveRegionsHeader();
    bool SaveTerrainHeader();
    bool SaveClassTable();

    // global entities
    PODArray<const MapSourceEntityData *> m_newGlobalEntityRefs;
    RegionEntityData *m_pGlobalEntityData;
    bool LoadGlobalEntityData();
    bool CompileGlobalEntityData(ProgressCallbacks *pProgressCallbacks);
    bool SaveGlobalEntityData();

#if 0

    //////////////////////////////////////////////////////////////////////////
    // STATIC LIGHTING MESH DATA
    //////////////////////////////////////////////////////////////////////////

    // static mesh information, used mainly for static lighting
    enum LightingMeshType
    {
        LightingMeshType_StaticMesh,
        LightingMeshType_StaticBlockMesh,
        LightingMeshType_Count,
    };
    struct LightingMeshData
    {
        LightingMeshType MeshType;
        String MeshResourceName;

        AABox BoundingBox;
        Sphere BoundingSphere;

        struct Triangle
        {
            struct Vertex
            {
                float3 Position;
                float3 Normal;

                bool UseNormalMap;
                const Texture2D *pNormalMapTexture;
                float3 NormalMapCoordinates;
            };
        };
        MemArray<Triangle> Triangles;
    };

    // get lighting mesh information
    LightingMeshData *GetLightingMeshData(LightingMeshType type, const String &resourceName);
    bool GetLightingMeshBounds(LightingMeshType type, const String &resourceName, AABox *pBoundingBox, Sphere *pBoundingSphere);

    //////////////////////////////////////////////////////////////////////////
    // STATIC LIGHTING INSTANCE DATA
    //////////////////////////////////////////////////////////////////////////
    struct LightingMeshInstanceData
    {
        LightingMeshType MeshType;
        String ResourceName;
        Transform MeshTransform;
        AABox BoundingBox;
        Sphere BoundingSphere;
    };
    PODArray<LightingMeshInstanceData *> m_lightingMeshInstanceData;

    // generate lighting mesh instance data
    bool GenerateLightingMeshInstanceData();

    // get a list of all lighting meshes intersecting the specified bounding box
    void GetLightingMeshesIntersectingBoundingBox(const AABox &boundingBox, PODArray<LightingMeshInstanceData *> *pInstances);

#endif
};
