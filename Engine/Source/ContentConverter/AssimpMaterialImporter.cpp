#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpMaterialImporter.h"
#include "ContentConverter/AssimpCommon.h"
#include "ContentConverter/TextureImporter.h"
#include "ResourceCompiler/MaterialGenerator.h"

AssimpMaterialImporter::AssimpMaterialImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(NULL),
      m_pLogStream(NULL),
      m_pScene(NULL)
{

}

AssimpMaterialImporter::~AssimpMaterialImporter()
{
    delete m_pLogStream;
    delete m_pImporter;
}

void AssimpMaterialImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->ListOnly = false;
    pOptions->AllMaterials = false;
}

bool AssimpMaterialImporter::Execute()
{
    if (m_pOptions->SourcePath.GetLength() == 0 ||
        m_pOptions->OutputName.GetLength() == 0 ||
        m_pOptions->OutputDirectory.GetLength() == 0)
    {
        
    }
    return false;
}

bool AssimpMaterialImporter::LoadScene()
{
    // create importer
    m_pImporter = new Assimp::Importer();

    // hook up log interface
    m_pLogStream = new AssimpHelpers::ProgressCallbacksLogStream(m_pProgressCallbacks);

    // pass filename to assimp
    m_pScene = m_pImporter->ReadFile(m_pOptions->SourcePath, 0);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ReadFile failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // apply postprocessing
    m_pScene = m_pImporter->ApplyPostProcessing(AssimpHelpers::GetAssimpSkeletalMeshImportPostProcessingFlags());
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ApplyPostProcessing failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // display info
    m_pProgressCallbacks->DisplayFormattedInformation("Scene info: %u animations, %u camera, %u lights, %u materials, %u meshes, %u textures", m_pScene->mNumAnimations, m_pScene->mNumCameras, m_pScene->mNumLights, m_pScene->mNumMaterials, m_pScene->mNumMeshes, m_pScene->mNumTextures);

    // ok
    return true;
}

void AssimpMaterialImporter::ListMaterials()
{
    m_pProgressCallbacks->DisplayFormattedInformation("%u materials in scene: ", m_pScene->mNumMaterials);

    for (uint32 i = 0; i < m_pScene->mNumMaterials; i++)
    {
        aiString aiMaterialName;
        if (!aiGetMaterialString(m_pScene->mMaterials[i], AI_MATKEY_NAME, &aiMaterialName))
            aiMaterialName.Set(String::FromFormat("material%u", i));

        m_pProgressCallbacks->DisplayFormattedInformation("  %s", aiMaterialName.C_Str());
    }
}

bool AssimpMaterialImporter::ImportMaterials()
{
    return false;
}

static bool GetTextureFileName(String &fileName, const char *sourceFileName, const char *textureFileName, ProgressCallbacks *pProgressCallbacks)
{
    FileSystem::BuildPathRelativeToFile(fileName, sourceFileName, textureFileName, true, true);

    if (FileSystem::FileExists(fileName))
        return true;

    // try to correct it by stripping all the crud off and making it relative to the source
    const char *lastSlashPos = Max(Y_strrchr(textureFileName, '/'), Y_strrchr(textureFileName, '\\'));
    if (lastSlashPos == nullptr)
        return false;

    // construct relative filename
    FileSystem::BuildPathRelativeToFile(fileName, sourceFileName, lastSlashPos + 1, true, true);
    if (FileSystem::FileExists(fileName))
    {
        pProgressCallbacks->DisplayFormattedWarning("Successfully corrected filename '%s' to '%s'", textureFileName, fileName.GetCharArray());
        return true;
    }

    pProgressCallbacks->DisplayFormattedError("Failed to locate texture at '%s' relative to '%s'", textureFileName, sourceFileName);
    return false;
}

static bool ImportTexture(const char *sourceFileName, const char *textureFileName, const char *maskFileName, const char *outputName, aiTextureMapMode *textureMapModes, TEXTURE_USAGE usage, ProgressCallbacks *pProgressCallbacks)
{
    // get full texture file name
    PathString fileName;

    // check if the name already exists
    fileName.Format("%s.tex.zip", outputName);
    if (g_pVirtualFileSystem->FileExists(fileName))
    {
        pProgressCallbacks->DisplayFormattedInformation("Skipping import of texture '%s', it already exists ('%s')", textureFileName, outputName);
        return true;
    }

    // get filename
    pProgressCallbacks->DisplayFormattedInformation("Trying to import texture at '%s'...", textureFileName);

    // exists?
    if (!GetTextureFileName(fileName, sourceFileName, textureFileName, pProgressCallbacks))
        return false;

    // create texture importer
    TextureImporterOptions options;
    TextureImporter::SetDefaultOptions(&options);
    options.InputFileNames.Add(fileName);
    options.OutputName = outputName;
    options.Usage = usage;
    options.Compile = false;
    options.Optimize = false;
    options.SourcePremultipliedAlpha = false;

    // handle masks
    if (maskFileName != nullptr)
    {
        pProgressCallbacks->DisplayFormattedInformation("With mask texture at '%s'...", maskFileName);

        // exists?
        if (!GetTextureFileName(fileName, sourceFileName, maskFileName, pProgressCallbacks))
            return false;

        options.InputMaskFileNames.Add(fileName);
    }

    // u mode
    if (textureMapModes[0] == aiTextureMapMode_Wrap)
        options.AddressU = TEXTURE_ADDRESS_MODE_WRAP;
    else if (textureMapModes[0] == aiTextureMapMode_Mirror)
        options.AddressU = TEXTURE_ADDRESS_MODE_MIRROR;
    else if (textureMapModes[0] == aiTextureMapMode_Clamp)
        options.AddressU = TEXTURE_ADDRESS_MODE_CLAMP;
    else
        options.AddressU = TEXTURE_ADDRESS_MODE_WRAP;

    // v mode
    if (textureMapModes[1] == aiTextureMapMode_Wrap)
        options.AddressV = TEXTURE_ADDRESS_MODE_WRAP;
    else if (textureMapModes[1] == aiTextureMapMode_Mirror)
        options.AddressV = TEXTURE_ADDRESS_MODE_MIRROR;
    else if (textureMapModes[1] == aiTextureMapMode_Clamp)
        options.AddressV = TEXTURE_ADDRESS_MODE_CLAMP;
    else
        options.AddressV = TEXTURE_ADDRESS_MODE_WRAP;

    // run the importer
    TextureImporter importer(pProgressCallbacks);
    if (!importer.Execute(&options))
    {
        pProgressCallbacks->DisplayFormattedError("Failed to import texture '%s'...", fileName.GetCharArray());
        return false;
    }

    // done
    return true;
}


bool AssimpMaterialImporter::ImportMaterial(const aiMaterial *pAIMaterial, const char *sourceFileName, const char *outputDirectory, const char *outputPrefix, const char *outputName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    aiColor4D propColor;
    aiString propString;
    aiString maskString;
    int propInt;
    aiTextureMapMode textureMapMode[2];
    PathString fileName;
    SmallString resourceName;
    unsigned int textureFlags;

    // pull some initial properties
    bool isTransparent = (aiGetMaterialInteger(pAIMaterial, AI_MATKEY_BLEND_FUNC, &propInt) == aiReturn_SUCCESS);
    bool hasNormalMap = (aiGetMaterialTextureCount(pAIMaterial, aiTextureType_NORMALS) > 0);
    bool hasBumpMap = (aiGetMaterialTextureCount(pAIMaterial, aiTextureType_HEIGHT) > 0);

    if (!isTransparent)
    {
        // try searching for an opacity map if not transparent
        textureFlags = 0;
        if ((aiGetMaterialTexture(pAIMaterial, aiTextureType_DIFFUSE, 0, &propString, nullptr, nullptr, nullptr, nullptr, nullptr, &textureFlags) == aiReturn_SUCCESS && textureFlags & aiTextureFlags_UseAlpha) ||
            aiGetMaterialTexture(pAIMaterial, aiTextureType_OPACITY, 0, &propString, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == aiReturn_SUCCESS)
        {
            // no blending is defined, but the material is transparent. assume default of straight blending
            isTransparent = true;
        }
    }

    // determine material to use
    MaterialGenerator materialGenerator;
    if (isTransparent)
        materialGenerator.Create((hasNormalMap || hasBumpMap) ? "shaders/engine/phong_lit_normalmap_transparent" : "shaders/engine/phong_lit_transparent");
    else
        materialGenerator.Create((hasNormalMap || hasBumpMap) ? "shaders/engine/phong_lit_normalmap" : "shaders/engine/phong_lit");

    // base color
    {
        if (aiGetMaterialColor(pAIMaterial, AI_MATKEY_COLOR_DIFFUSE, &propColor) != aiReturn_SUCCESS)
        {
            propColor.r = 1.0f;
            propColor.g = 1.0f;
            propColor.b = 1.0f;
            propColor.a = 1.0f;
        }

        float3 diffuseColor(propColor.r, propColor.g, propColor.b);
        materialGenerator.SetShaderUniformParameterByName("BaseColor", SHADER_PARAMETER_TYPE_FLOAT3, &diffuseColor);
    }

    // default texture map mode
    textureMapMode[0] = aiTextureMapMode_Wrap;
    textureMapMode[1] = aiTextureMapMode_Wrap;

    // base texture
    if (aiGetMaterialTexture(pAIMaterial, aiTextureType_DIFFUSE, 0, &propString, nullptr, nullptr, nullptr, nullptr, textureMapMode, &textureFlags) == aiReturn_SUCCESS)
    {
        // remove the extension
        resourceName = propString.C_Str();
        int32 extensionPos = resourceName.RFind('.');
        if (extensionPos > 0)
            resourceName.Erase(extensionPos);

        // sanitize the texture name
        FileSystem::SanitizeFileName(resourceName);
        fileName.Format("%s/%s%s", outputDirectory, outputPrefix, resourceName.GetCharArray());

        // has an alpha mask? add it at the same time
        if (isTransparent)
        {
            // if use alpha flag is set, use that as the alpha channel. otherwise, try for the opacity texture
            if (textureFlags & aiTextureFlags_UseAlpha)
                maskString = propString;
            else
                aiGetMaterialTexture(pAIMaterial, aiTextureType_OPACITY, 0, &maskString, nullptr, nullptr, nullptr, nullptr, nullptr, &textureFlags);
        }

        // try importing it
        if (ImportTexture(sourceFileName, propString.C_Str(), (maskString.length != 0) ? maskString.C_Str() : nullptr, fileName, textureMapMode, TEXTURE_USAGE_COLOR_MAP, pProgressCallbacks))
        {
            materialGenerator.SetShaderStaticSwitchParameterByName("UseBaseTexture", true);
            materialGenerator.SetShaderTextureParameterStringByName("BaseTexture", fileName);
        }
    }

    if (hasNormalMap && aiGetMaterialTexture(pAIMaterial, aiTextureType_NORMALS, 0, &propString, nullptr, nullptr, nullptr, nullptr, textureMapMode, &textureFlags) == aiReturn_SUCCESS)
    {
        // remove the extension
        resourceName = propString.C_Str();
        int32 extensionPos = resourceName.RFind('.');
        if (extensionPos > 0)
            resourceName.Erase(extensionPos);

        // sanitize the texture name
        FileSystem::SanitizeFileName(resourceName);
        fileName.Format("%s/%s%s", outputDirectory, outputPrefix, resourceName.GetCharArray());

        // try importing it
        if (ImportTexture(sourceFileName, propString.C_Str(), nullptr, fileName, textureMapMode, TEXTURE_USAGE_NORMAL_MAP, pProgressCallbacks))
            materialGenerator.SetShaderTextureParameterStringByName("NormalMap", fileName);

    }
    else if (hasBumpMap && aiGetMaterialTexture(pAIMaterial, aiTextureType_HEIGHT, 0, &propString, nullptr, nullptr, nullptr, nullptr, textureMapMode, &textureFlags) == aiReturn_SUCCESS)
    {
        // remove the extension
        resourceName = propString.C_Str();
        int32 extensionPos = resourceName.RFind('.');
        if (extensionPos > 0)
            resourceName.Erase(extensionPos);

        // sanitize the texture name
        FileSystem::SanitizeFileName(resourceName);
        fileName.Format("%s/%s%s", outputDirectory, outputPrefix, resourceName.GetCharArray());

        // try importing it
        // for now, treat bump map == height map
        if (ImportTexture(sourceFileName, propString.C_Str(), nullptr, fileName, textureMapMode, TEXTURE_USAGE_NORMAL_MAP, pProgressCallbacks))
            materialGenerator.SetShaderTextureParameterStringByName("NormalMap", fileName);
    }

    // construct output filename
    fileName.Format("%s.mtl.xml", outputName);
    AutoReleasePtr<ByteStream> pOutputStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pOutputStream == nullptr || !materialGenerator.SaveToXML(pOutputStream))
    {
        pProgressCallbacks->DisplayFormattedError("Failed to write material file '%s'", fileName.GetCharArray());
        if (pOutputStream != nullptr)
            pOutputStream->Discard();
        return false;
    }

    pOutputStream->Commit();
    return true;
}

