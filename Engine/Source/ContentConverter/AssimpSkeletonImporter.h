#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiNode;
struct aiAnimation;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class SkeletonGenerator;

class AssimpSkeletonImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        String OutputResourceName;
        COORDINATE_SYSTEM CoordinateSystem;
        float4x4 TransformMatrix;
    };

public:
    AssimpSkeletonImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpSkeletonImporter();

    static void SetDefaultOptions(Options *pOptions);

    bool Execute();

private:
    bool LoadScene();
    bool CreateBones();
    bool ReadBoneInfoFromNode(const aiNode *pNode, int32 parentBoneIndex);
    bool WriteOutput();

    const Options *m_pOptions;
    
    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;

    float4x4 m_globalTransform;
    float4x4 m_inverseGlobalTransform;

    SkeletonGenerator *m_pOutputGenerator;
};

