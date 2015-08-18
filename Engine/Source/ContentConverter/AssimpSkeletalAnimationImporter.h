#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiNode;
struct aiAnimation;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class Skeleton;
class SkeletonGenerator;
class SkeletalAnimationGenerator;

class AssimpSkeletalAnimationImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        bool ListOnly;
        bool CreateSkeleton;
        String SkeletonName;
        String OutputResourceName;
        String OutputResourceDirectory;
        String OutputResourcePrefix;
        float DefaultAnimationTicksPerSecond;
        float OverrideTicksPerSecond;
        bool AllAnimations;
        String SingleAnimationName;
        bool ClipAnimation;
        uint32 ClipRangeStart;
        uint32 ClipRangeEnd;
        bool OptimizeAnimation;
    };

public:
    AssimpSkeletalAnimationImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpSkeletalAnimationImporter();

    static void SetDefaultOptions(Options *pOptions);

    bool Execute();

private:
    bool CreateSkeleton();
    bool LoadScene();
    void ListAnimations();
    bool CreateAnimations();
    bool ParseAnimation(const aiAnimation *pAnimation, const char *outputResourceName, bool clipAnimation = false, uint32 clipKeyFrameStart = 0, uint32 clipKeyFrameEnd = 0);
    bool WriteOutput(const char *resourceName, const SkeletalAnimationGenerator *pOutputAnimation);

    const Options *m_pOptions;
    
    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;
};

