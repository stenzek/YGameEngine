#pragma once
#include "ContentConverter/BaseImporter.h"

namespace Assimp { class Importer; }
struct aiScene;
struct aiMaterial;
namespace AssimpHelpers { class ProgressCallbacksLogStream; }
class MaterialGenerator;

class AssimpMaterialImporter : public BaseImporter
{
public:
    struct Options
    {
        String SourcePath;
        bool ListOnly;
        bool AllMaterials;
        String SingleMaterialName;
        String OutputName;
        String OutputDirectory;
        String OutputPrefix;
    };

public:
    AssimpMaterialImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks);
    ~AssimpMaterialImporter();

    static void SetDefaultOptions(Options *pOptions);

    bool Execute();

    static bool ImportMaterial(const aiMaterial *pAIMaterial, const char *sourceFileName, const char *outputDirectory, const char *outputPrefix, const char *outputName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

private:
    bool LoadScene();
    void ListMaterials();
    bool ImportMaterials();

    const Options *m_pOptions;

    Assimp::Importer *m_pImporter;
    AssimpHelpers::ProgressCallbacksLogStream *m_pLogStream;
    const aiScene *m_pScene;
};

