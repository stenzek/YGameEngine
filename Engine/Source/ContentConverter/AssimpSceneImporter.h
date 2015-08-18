#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class StaticMeshGenerator;

class AssimpSceneImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        String MeshDirectory;
        String MeshPrefix;
        String OutputMapName;
        bool ImportMaterials;
        String MaterialDirectory;
        String MaterialPrefix;
        String DefaultMaterialName;
        uint32 BuildCollisionShapeType;
        float4x4 TransformMatrix;
        COORDINATE_SYSTEM CoordinateSystem;
        uint32 CenterMeshes;

        // todo: split into multiple meshes, center mesh, origin, etc.
    };

public:
    AssimpSceneImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpSceneImporter();

    static void SetDefaultOptions(Options *pOptions);

    bool Execute();

private:
    bool LoadScene();
    bool CreateMaterials();
    bool ParseScene();
    bool ParseNode(const aiNode *pNode, const void *parentTransform);
    bool ParseMesh(const aiMesh *pMesh, StaticMeshGenerator *pOutputGenerator, uint32 lodIndex, const char *meshName);
    bool FinalizeMesh(StaticMeshGenerator *pOutputGenerator, const char *meshName);
    bool WriteMeshes();
    bool WriteMap();

    const Options *m_pOptions;
    
    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;

    // global transform (coordinate system)
    float4x4 m_globalTransform;

    Array<String> m_materialMapping;

    struct OutputMesh
    {
        // pointer is a copy of the object in m_pScene, so no delete needed!
        const unsigned int *pSourceMeshes;
        uint32 nSourceMeshes;

        String MeshName;
        StaticMeshGenerator *pGenerator;
        float3 Offset;
    };

    Array<OutputMesh> m_meshes;

    typedef HashTable<const aiMesh *, uint32> MeshGeneratorHashMap;
    MeshGeneratorHashMap m_aiMeshToOutputMesh;

    struct OutputMeshInstance
    {
        uint32 MeshIndex;
        float3 Position;
        Quaternion Rotation;
        float3 Scale;
    };

    MemArray<OutputMeshInstance> m_meshInstances;
};

