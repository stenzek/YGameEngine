#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class StaticMeshGenerator;

class AssimpStaticMeshImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        String OutputResourceName;
        bool ImportMaterials;
        String MaterialDirectory;
        String MaterialPrefix;
        String DefaultMaterialName;
        uint32 BuildCollisionShapeType;
        COORDINATE_SYSTEM CoordinateSystem;
        bool FlipFaceOrder;
        float4x4 TransformMatrix;

        // todo: split into multiple meshes, center mesh, origin, etc.
    };

public:
    AssimpStaticMeshImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpStaticMeshImporter();

    const StaticMeshGenerator *GetOutputGenerator() const { return m_pOutputGenerator; }

    // removes ownership of the generator from the importer, ie will not be deleted
    StaticMeshGenerator *DetachOutputGenerator();

    static void SetDefaultOptions(Options *pOptions);
    static bool GetFileSubMeshNames(const char *filename, Array<String> &meshNames, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    bool Execute();

private:
    bool LoadScene();
    bool CreateGenerator();
    bool CreateMaterials();
    bool CreateMesh();
    bool ParseMesh(const aiMesh *pMesh, StaticMeshGenerator *pOutputGenerator, uint32 lodIndex, const char *meshName);
    bool PostProcess();
    bool WriteOutput();

    const Options *m_pOptions;
    
    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;

    String m_baseOutputPath;
    Array<String> m_materialMapping;
    float4x4 m_globalTransform;

    StaticMeshGenerator *m_pOutputGenerator;
};

