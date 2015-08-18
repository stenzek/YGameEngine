#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiNode;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class SkeletalMeshGenerator;

class AssimpSkeletalMeshImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        String OutputResourceName;
        String SkeletonName;
        bool ImportMaterials;
        String MaterialDirectory;
        String MaterialPrefix;
        String DefaultMaterialName;
        COORDINATE_SYSTEM CoordinateSystem;
        bool FlipFaceOrder;
        String CollisionShapeType;
    };

public:
    AssimpSkeletalMeshImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpSkeletalMeshImporter();

    static void SetDefaultOptions(Options *pOptions);

    bool Execute();

private:
    bool LoadScene();
    bool CreateGenerator();
    bool CreateMaterials();
    bool CreateMesh();
    bool ParseNode(const aiNode *pNode);
    bool PostProcess();
    bool CreateCollisionShape();
    bool WriteOutput();

    const Options *m_pOptions;
    
    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;

    String m_baseOutputPath;
    PODArray<uint32> m_materialMapping;

    SkeletalMeshGenerator *m_pOutputGenerator;
};

