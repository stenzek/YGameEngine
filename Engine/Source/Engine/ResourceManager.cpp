#include "Engine/PrecompiledHeader.h"
#include "Engine/ResourceManager.h"
#include "Engine/Engine.h"
#include "Engine/Texture.h"
#include "Engine/MaterialShader.h"
#include "Engine/Material.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/BlockPalette.h"
#include "Engine/TerrainLayerList.h"
#include "Engine/BlockMesh.h"
#include "Engine/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalAnimation.h"
#include "Engine/ParticleSystem.h"
#include "Engine/EngineCVars.h"
#include "Renderer/Renderer.h"
#include "ResourceCompilerInterface/ResourceCompilerInterface.h"
#include "Core/DefinitionFile.h"
Log_SetChannel(Renderer);

static ResourceManager s_ResourceManager;
ResourceManager *g_pResourceManager = &s_ResourceManager;

ResourceManager::ResourceManager()
{
    m_pDefaultTexture2D = NULL;
    m_pDefaultTextureCube = NULL;
    m_pDefaultMaterial = NULL;
    m_pDefaultStaticMesh = NULL;
    m_pDefaultBlockMesh = NULL;
    m_pDefaultSkeletalMesh = NULL;

    m_pResourceModificationChangeNotifier = nullptr;
    m_pResourceCompilerInterface = nullptr;
    m_resourceCompilerInUse = false;
}

ResourceManager::~ResourceManager()
{
    DebugAssert(m_pDefaultTexture2D == NULL);
    DebugAssert(m_pDefaultTextureCube == NULL);
    DebugAssert(m_pDefaultMaterial == NULL);
    DebugAssert(m_pDefaultStaticMesh == NULL);
    DebugAssert(m_pDefaultBlockMesh == NULL);
    DebugAssert(m_pDefaultSkeletalMesh == NULL);

    DebugAssert(m_htMaterials.GetMemberCount() == 0);
    DebugAssert(m_htTextures.GetMemberCount() == 0);
    DebugAssert(m_htStaticMeshes.GetMemberCount() == 0);
    DebugAssert(m_htFonts.GetMemberCount() == 0);
    DebugAssert(m_htBlockPalette.GetMemberCount() == 0);
    DebugAssert(m_htTerrainLayerList.GetMemberCount() == 0);
    DebugAssert(m_htBlockMesh.GetMemberCount() == 0);
    DebugAssert(m_htSkeleton.GetMemberCount() == 0);
    DebugAssert(m_htSkeletalMesh.GetMemberCount() == 0);
    DebugAssert(m_htSkeletalAnimation.GetMemberCount() == 0);
    DebugAssert(m_htParticleSystem.GetMemberCount() == 0);

    delete m_pResourceModificationChangeNotifier;

    DebugAssert(m_pResourceCompilerInterface == nullptr);
}

const ResourceTypeInfo *ResourceManager::GetResourceTypeForFile(const char *FileName)
{
    PathString temp;
    return GetResourceTypeForStream(FileName, NULL, temp);
}

const ResourceTypeInfo *ResourceManager::GetResourceTypeForFile(const char *FileName, String &resourceName)
{
    return GetResourceTypeForStream(FileName, NULL, resourceName);
}

const ResourceTypeInfo *ResourceManager::GetResourceTypeForStream(const char *FileName, ByteStream *pStream)
{
    PathString temp;
    return GetResourceTypeForStream(FileName, pStream, temp);
}

const ResourceTypeInfo *ResourceManager::GetResourceTypeForStream(const char *FileName, ByteStream *pStream, String &resourceName)
{
    resourceName = FileName;

    // Material
    if (resourceName.EndsWith(".mtl", false))
    {
        resourceName.Erase(-4);
        return Material::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".mtl.xml", false))
    {
        resourceName.Erase(-8);
        return Material::StaticTypeInfo();
    }

    // MaterialShader
    if (resourceName.EndsWith(".msh", false))
    {
        resourceName.Erase(-4);
        return MaterialShader::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".msh.xml", false))
    {
        resourceName.Erase(-8);
        return MaterialShader::StaticTypeInfo();
    }

    // Textures
    if (resourceName.EndsWith(".tex", false))
    {
        resourceName.Erase(-4);

        TEXTURE_TYPE textureType;
        if (pStream == NULL)
        {
            pStream = g_pVirtualFileSystem->OpenFile(FileName, BYTESTREAM_OPEN_READ);
            if (pStream == NULL)
                return NULL;

            textureType = Texture::GetTextureTypeForStream(FileName, pStream);
            SAFE_RELEASE(pStream);
        }
        else
        {
            textureType = Texture::GetTextureTypeForStream(FileName, pStream);
        }

        return (textureType != TEXTURE_TYPE_COUNT) ? Texture2D::GetResourceTypeInfoForTextureType(textureType) : NULL;
    }
    else if (resourceName.EndsWith(".tex.dds", false))
    {
        resourceName.Erase(-8);
        return Texture::StaticTypeInfo();
    }

    // StaticMesh
    if (resourceName.EndsWith(".staticmesh", false))
    {
        resourceName.Erase(-11);
        return StaticMesh::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".staticmesh.xml", false))
    {
        resourceName.Erase(-15);
        return StaticMesh::StaticTypeInfo();
    }

    // Font
    if (resourceName.EndsWith(".font", false))
    {
        resourceName.Erase(-5);
        return Font::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".font.zip", false))
    {
        resourceName.Erase(-9);
        return Font::StaticTypeInfo();
    }

    // BlockMeshBlockList
    if (resourceName.EndsWith(".blp", false))
    {
        resourceName.Erase(-4);
        return BlockPalette::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".blp.xml", false))
    {
        resourceName.Erase(-8);
        return BlockPalette::StaticTypeInfo();
    }

    // BlockMeshBlockList
    if (resourceName.EndsWith(".layerlist", false))
    {
        resourceName.Erase(-10);
        return TerrainLayerList::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".layerlist.xml", false))
    {
        resourceName.Erase(-14);
        return TerrainLayerList::StaticTypeInfo();
    }

    // StaticBlockMesh
    if (resourceName.EndsWith(".blm", false))
    {
        resourceName.Erase(-4);
        return BlockMesh::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".blm.xml", false))
    {
        resourceName.Erase(-8);
        return BlockMesh::StaticTypeInfo();
    }

    // Skeleton
    if (resourceName.EndsWith(".skl", false))
    {
        resourceName.Erase(-4);
        return Skeleton::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".skl.xml", false))
    {
        resourceName.Erase(-8);
        return Skeleton::StaticTypeInfo();
    }

    // SkeletalMesh
    if (resourceName.EndsWith(".skm", false))
    {
        resourceName.Erase(-4);
        return SkeletalMesh::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".skm.xml", false))
    {
        resourceName.Erase(-8);
        return SkeletalMesh::StaticTypeInfo();
    }

    // SkeletalAnimation
    if (resourceName.EndsWith(".ska", false))
    {
        resourceName.Erase(-4);
        return SkeletalAnimation::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".ska.xml", false))
    {
        resourceName.Erase(-8);
        return SkeletalAnimation::StaticTypeInfo();
    }

    // ParticleSystem
    if (resourceName.EndsWith(".ParticleSystem", false))
    {
        resourceName.Erase(-15);
        return ParticleSystem::StaticTypeInfo();
    }
    else if (resourceName.EndsWith(".ParticleSystem.xml", false))
    {
        resourceName.Erase(-19);
        return ParticleSystem::StaticTypeInfo();
    }

    return nullptr;
}

void ResourceManager::ReleaseResources()
{
    Log_DevPrint("ResourceManager is releasing all managed resources...");

    // delete particle systems
    for (ParticleSystemTable::Iterator itr = m_htParticleSystem.Begin(); !itr.AtEnd();)
    {
        ParticleSystemTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htParticleSystem.Remove(pMember);
    }        

    // delete skeletal animation sets
    for (SkeletalAnimationTable::Iterator itr = m_htSkeletalAnimation.Begin(); !itr.AtEnd(); )
    {
        SkeletalAnimationTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htSkeletalAnimation.Remove(pMember);
    }

    // delete skeletal meshes
    for (SkeletalMeshTable::Iterator itr = m_htSkeletalMesh.Begin(); !itr.AtEnd();)
    {
        SkeletalMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htSkeletalMesh.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultSkeletalMesh);

    // delete skeletons
    for (SkeletonTable::Iterator itr = m_htSkeleton.Begin(); !itr.AtEnd();)
    {
        SkeletonTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htSkeleton.Remove(pMember);
    }

    // delete static block meshes
    for (BlockMeshTable::Iterator itr = m_htBlockMesh.Begin(); !itr.AtEnd(); )
    {
        BlockMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htBlockMesh.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultBlockMesh);

    // delete terrain layer lists
    for (TerrainLayerListTable::Iterator itr = m_htTerrainLayerList.Begin(); !itr.AtEnd(); )
    {
        TerrainLayerListTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htTerrainLayerList.Remove(pMember);
    }

    // delete block mesh block lists
    for (BlockPaletteTable::Iterator itr = m_htBlockPalette.Begin(); !itr.AtEnd(); )
    {
        BlockPaletteTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htBlockPalette.Remove(pMember);
    }

    // delete font data
    for (FontTable::Iterator itr = m_htFonts.Begin(); !itr.AtEnd(); )
    {
        FontTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htFonts.Remove(pMember);
    }

    // delete staticmesh
    for (StaticMeshTable::Iterator itr = m_htStaticMeshes.Begin(); !itr.AtEnd(); )
    {
        StaticMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htStaticMeshes.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultStaticMesh);

    // delete materials
    for (MaterialTable::Iterator itr = m_htMaterials.Begin(); !itr.AtEnd(); )
    {
        MaterialTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htMaterials.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultMaterial);

    // delete textures
    for (TextureTable::Iterator itr = m_htTextures.Begin(); !itr.AtEnd(); )
    {
        TextureTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htTextures.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultTexture2D);
    SAFE_RELEASE(m_pDefaultTexture2DArray);
    SAFE_RELEASE(m_pDefaultTextureCube);

    // delete material shaders
    for (MaterialShaderTable::Iterator itr = m_htMaterialShaders.Begin(); !itr.AtEnd(); )
    {
        MaterialShaderTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
            pMember->Value->Release();

        m_htMaterialShaders.Remove(pMember);
    }
    SAFE_RELEASE(m_pDefaultMaterialShader);
}

static bool GetResourceStatus(String &resourceName, const char *compiledExtension, const char *sourceExtension, bool &compileResource, bool &hasSourceVersion, bool &hasCompiledVersion)
{
    FILESYSTEM_STAT_DATA uncompiledStatData, compiledStatData;
    PathString currentFileName;

    // firstly check for the compiled version
    currentFileName.Format("%s%s", resourceName.GetCharArray(), compiledExtension);
    hasCompiledVersion = g_pVirtualFileSystem->StatFile(currentFileName, &compiledStatData);
    if (hasCompiledVersion)
    {
        // extract the resource name from it
        if (g_pVirtualFileSystem->GetFileName(currentFileName))
        {
            resourceName.Clear();
            resourceName.AppendSubString(currentFileName, 0, -(int32)Y_strlen(compiledExtension));
        }
    }

#if defined(WITH_RESOURCECOMPILER_EMBEDDED) || defined(WITH_RESOURCECOMPILER_SUBPROCESS)
    // now check for the uncompiled version
    if (CVars::rm_enable_resource_compilation.GetBool())
    {
        currentFileName.Format("%s%s", resourceName.GetCharArray(), sourceExtension);
        hasSourceVersion = g_pVirtualFileSystem->StatFile(currentFileName, &uncompiledStatData);
        if (hasSourceVersion)
        {
            // we only use the uncompiled version if it's modification time is greater
            if (hasCompiledVersion && uncompiledStatData.ModificationTime <= compiledStatData.ModificationTime)
            {
                // don't use the source version
                compileResource = false;
            }
            else
            {
                // extract the resource name from it
                if (g_pVirtualFileSystem->GetFileName(currentFileName))
                {
                    resourceName.Clear();
                    resourceName.AppendSubString(currentFileName, 0, -(int32)Y_strlen(sourceExtension));
                }

                // make the resource compile
                compileResource = true;
            }
        }
        else
        {
            // no source version, so can't compile, duh
            compileResource = false;
        }
    }
    else
#endif
    {
        // resource compilation not enabled
        hasSourceVersion = false;
        compileResource = false;
    }

    // if we have neither, return false
    return (hasCompiledVersion | hasSourceVersion);
}

Material *ResourceManager::LoadMaterial(const char *Name)
{
    PathString fileName;
    Material *pMaterial = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".mtl", ".mtl.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.mtl", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pMaterial = new Material();
                if (!pMaterial->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadMaterial: Failed to load Material '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pMaterial->Release();
                    pMaterial = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadMaterial: Failed to load Material '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile material
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileMaterial(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // release compiler here since we're no longer using it and something else could trigger compiling
                    ReleaseResourceCompilerInterface(pCompilerInterface);

                    // write it to disk
                    fileName.Format("%s.mtl", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadMaterial: Failed to write Material '%s' to disk.", resourceName.GetCharArray());

                    // load the material from memory (saves a round-trip to the disk)
                    pMaterial = new Material();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pMaterial->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadMaterial: Failed to load just-compiled Material '%s'.", resourceName.GetCharArray());
                        pMaterial->Release();
                        pMaterial = nullptr;
                    }
                }
                else
                {
                    // compile material failed
                    Log_ErrorPrintf("ResourceManager::LoadMaterial: Failed to compile Material '%s'.", resourceName.GetCharArray());
                    ReleaseResourceCompilerInterface(pCompilerInterface);
                }
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadMaterial: Could not compile Material '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadMaterial: Material '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pMaterial != nullptr)
        Log_ProfilePrintf("PROFILE: Material load of '%s' took %.3f msec", pMaterial->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pMaterial;
}

const Material *ResourceManager::UncachedGetMaterial(const char *Name)
{
    MaterialTable::Member *pMember = m_htMaterials.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadMaterial(Name);
}

const Material *ResourceManager::GetMaterial(const char *Name)
{
    m_resourceLock.LockShared();

    MaterialTable::Member *pMember = m_htMaterials.Find(Name);
    if (pMember == nullptr)
    {
        m_resourceLock.UnlockShared();

        Material *pMaterial = LoadMaterial(Name);

        m_resourceLock.LockExclusive();
        pMember = m_htMaterials.Find(Name);
        if (pMember == nullptr)
            pMember = m_htMaterials.Insert((pMaterial != nullptr) ? pMaterial->GetName().GetCharArray() : Name, pMaterial);
        else if (pMaterial != nullptr)
            pMaterial->Release();
        m_resourceLock.UnlockExclusive();
    }
    else
    {
        m_resourceLock.UnlockShared();
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const Material *ResourceManager::GetDefaultMaterial()
{
    if (m_pDefaultMaterial == nullptr && (m_pDefaultMaterial = GetMaterial(g_pEngine->GetDefaultMaterialName())) == nullptr)
        Panic("GetDefaultMaterial() called, and the default material failed to load.");

    m_pDefaultMaterial->AddRef();
    return m_pDefaultMaterial;
}

const Material *ResourceManager::SafeGetMaterial(const char *Name)
{
    if (Name != nullptr)
    {
        const Material *pMaterial = GetMaterial(Name);
        if (pMaterial != nullptr)
            return pMaterial;
    }

    return GetDefaultMaterial();
}

MaterialShader *ResourceManager::LoadMaterialShader(const char *Name)
{
    PathString fileName;
    MaterialShader *pMaterialShader = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".msh", ".msh.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.msh", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pMaterialShader = new MaterialShader();
                if (!pMaterialShader->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadMaterialShader: Failed to load MaterialShader '%s', read failed.", resourceName.GetCharArray());
                    pMaterialShader->Release();
                    pMaterialShader = nullptr;
                    compileResource = hasSourceVersion;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadMaterialShader: Failed to load MaterialShader '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile material
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileMaterialShader(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.msh", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadMaterialShader: Failed to write MaterialShader '%s' to disk.", resourceName.GetCharArray());

                    // load the material from memory (saves a round-trip to the disk)
                    pMaterialShader = new MaterialShader();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pMaterialShader->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadMaterialShader: Failed to load just-compiled MaterialShader '%s'.", resourceName.GetCharArray());
                        pMaterialShader->Release();
                        pMaterialShader = nullptr;
                    }
                }
                else
                {
                    // compile material failed
                    Log_ErrorPrintf("ResourceManager::LoadMaterialShader: Failed to compile MaterialShader '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadMaterialShader: Could not compile MaterialShader '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadMaterialShader: MaterialShader '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pMaterialShader != nullptr)
        Log_ProfilePrintf("PROFILE: Material shader load of '%s' took %.3f msec", pMaterialShader->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pMaterialShader;
}

const MaterialShader *ResourceManager::GetMaterialShader(const char *Name)
{
    m_resourceLock.LockShared();

    MaterialShaderTable::Member *pMember = m_htMaterialShaders.Find(Name);
    if (pMember == nullptr)
    {
        m_resourceLock.UnlockShared();

        MaterialShader *pMaterialShader = LoadMaterialShader(Name);

        m_resourceLock.LockExclusive();
        pMember = m_htMaterialShaders.Find(Name);
        if (pMember == nullptr)
            pMember = m_htMaterialShaders.Insert((pMaterialShader != nullptr) ? pMaterialShader->GetName().GetCharArray() : Name, pMaterialShader);
        else if (pMaterialShader != nullptr)
            pMaterialShader->Release();
        m_resourceLock.UnlockExclusive();
    }
    else
    {
        m_resourceLock.UnlockShared();
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const MaterialShader *ResourceManager::UncachedGetMaterialShader(const char *Name)
{
    MaterialShaderTable::Member *pMember = m_htMaterialShaders.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadMaterialShader(Name);
}

const MaterialShader *ResourceManager::GetDefaultMaterialShader()
{
    if (m_pDefaultMaterialShader == nullptr && (m_pDefaultMaterialShader = GetMaterialShader(g_pEngine->GetDefaultMaterialShaderName())) == nullptr)
        Panic("GetDefaultShader() called, and the default material shader failed to load.");

    m_pDefaultMaterialShader->AddRef();
    return m_pDefaultMaterialShader;
}

const MaterialShader *ResourceManager::SafeGetMaterialShader(const char *Name)
{
    if (Name != nullptr)
    {
        const MaterialShader *pMaterialShader = GetMaterialShader(Name);
        if (pMaterialShader != nullptr)
            return pMaterialShader;
    }

    return GetDefaultMaterialShader();
}

Texture *ResourceManager::LoadTexture(const char *Name)
{
    PathString fileName;
    Texture *pTexture = nullptr;

    // get extension for current texture platform
    TEXTURE_PLATFORM texturePlatform = (g_pRenderer != nullptr) ? g_pRenderer->GetTexturePlatform() : TEXTURE_PLATFORM_DXTC;
    TinyString texturePlatformExtension;
    texturePlatformExtension.Format(".tex_%s", NameTable_GetNameString(NameTables::TexturePlatformFileExtension, texturePlatform));

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, texturePlatformExtension, ".tex.zip", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                TEXTURE_TYPE textureType = Texture::GetTextureTypeForStream(fileName, pStream);
                pTexture = Texture::CreateTextureObjectForType(textureType);
                if (pTexture == nullptr || !pTexture->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadTexture: Failed to load Texture '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    SAFE_RELEASE(pTexture);
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadTexture: Failed to load Texture '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile Texture
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileTexture(texturePlatform, resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::GetTexture: Failed to write Texture '%s' to disk.", resourceName.GetCharArray());

                    // load the Texture from memory (saves a round-trip to the disk)
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    TEXTURE_TYPE textureType = Texture::GetTextureTypeForStream(fileName, pStream);
                    pTexture = Texture::CreateTextureObjectForType(textureType);
                    if (pTexture == nullptr || !pTexture->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadTexture: Failed to load just-compiled Texture '%s'.", resourceName.GetCharArray());
                        SAFE_RELEASE(pTexture);
                    }
                }
                else
                {
                    // compile Texture failed
                    Log_ErrorPrintf("ResourceManager::LoadTexture: Failed to compile Texture '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadTexture: Could not compile Texture '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadTexture: Texture '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pTexture != nullptr)
    {
        Log_ProfilePrintf("PROFILE: Texture load of \"%s\" (%s, %s) took %.3f msec",
                          pTexture->GetName().GetCharArray(), pTexture->GetResourceTypeInfo()->GetTypeName(),
                          PixelFormat_GetPixelFormatInfo(pTexture->GetPixelFormat())->Name, loadTimer.GetTimeMilliseconds());
    }
#endif

    return pTexture;
}

const Texture *ResourceManager::GetTexture(const char *Name)
{
    m_resourceLock.LockShared();

    TextureTable::Member *pMember = m_htTextures.Find(Name);
    if (pMember == nullptr)
    {
        m_resourceLock.UnlockShared();

        Texture *pTexture = LoadTexture(Name);

        m_resourceLock.LockExclusive();
        pMember = m_htTextures.Find(Name);
        if (pMember == nullptr)
            pMember = m_htTextures.Insert((pTexture != nullptr) ? pTexture->GetName().GetCharArray() : Name, pTexture);
        else if (pTexture != nullptr)
            pTexture->Release();
        m_resourceLock.UnlockExclusive();
    }
    else
    {
        m_resourceLock.UnlockShared();
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const Texture2D *ResourceManager::GetTexture2D(const char *Name)
{
    const Texture *pTexture = GetTexture(Name);
    if (pTexture == nullptr)
        return nullptr;

    const Texture2D *pTexture2D = pTexture->SafeCast<Texture2D>();
    if (pTexture2D == NULL)
    {
        Log_ErrorPrintf("RESOURCE TYPE MISMATCH: Attempting to access resource '%s' as a Texture2D, when it is a %s", pTexture->GetName().GetCharArray(), pTexture->GetResourceTypeInfo()->GetTypeName());
        pTexture->Release();
        return nullptr;
    }
    
    return pTexture2D;
}

const Texture2DArray *ResourceManager::GetTexture2DArray(const char *Name)
{
    const Texture *pTexture = GetTexture(Name);
    if (pTexture == nullptr)
        return nullptr;

    const Texture2DArray *pTexture2DArray = pTexture->SafeCast<Texture2DArray>();
    if (pTexture2DArray == nullptr)
    {
        Log_ErrorPrintf("RESOURCE TYPE MISMATCH: Attempting to access resource '%s' as a Texture2DArray, when it is a %s", pTexture->GetName().GetCharArray(), pTexture->GetResourceTypeInfo()->GetTypeName());
        pTexture->Release();
        return NULL;
    }

    return pTexture2DArray;
}

const TextureCube *ResourceManager::GetTextureCube(const char *Name)
{
    const Texture *pTexture = GetTexture(Name);
    if (pTexture == nullptr)
        return nullptr;

    const TextureCube *pTextureCube = pTexture->SafeCast<TextureCube>();
    if (pTextureCube == nullptr)
    {
        Log_ErrorPrintf("RESOURCE TYPE MISMATCH: Attempting to access resource '%s' as a TextureCube, when it is a %s", pTexture->GetName().GetCharArray(), pTexture->GetResourceTypeInfo()->GetTypeName());
        pTexture->Release();
        return NULL;
    }

    return pTextureCube;
}

const Texture *ResourceManager::UncachedGetTexture(const char *Name)
{
    TextureTable::Member *pMember = m_htTextures.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadTexture(Name);
}

const Texture *ResourceManager::GetDefaultTexture(TEXTURE_TYPE TextureType)
{
    switch (TextureType)
    {
    case TEXTURE_TYPE_2D:
        return GetDefaultTexture2D();
        
    case TEXTURE_TYPE_2D_ARRAY:
        return GetDefaultTexture2DArray();

    case TEXTURE_TYPE_CUBE:
        return GetDefaultTextureCube();
    }

    UnreachableCode();
    return nullptr;
}

const Texture2D *ResourceManager::GetDefaultTexture2D()
{
    if (m_pDefaultTexture2D == nullptr && (m_pDefaultTexture2D = GetTexture2D(g_pEngine->GetDefaultTexture2DName())) == nullptr)
        Panic("GetDefaultTexture2D() called, and the default texture failed to load.");

    m_pDefaultTexture2D->AddRef();
    return m_pDefaultTexture2D;
}

const Texture2DArray *ResourceManager::GetDefaultTexture2DArray()
{
    if (m_pDefaultTexture2DArray == nullptr && (m_pDefaultTexture2DArray = GetTexture2DArray(g_pEngine->GetDefaultTexture2DArrayName())) == nullptr)
        Panic("GetDefaultTexture2DArray() called, and the default texture failed to load.");

    m_pDefaultTexture2DArray->AddRef();
    return m_pDefaultTexture2DArray;
}

const TextureCube *ResourceManager::GetDefaultTextureCube()
{
    if (m_pDefaultTextureCube == nullptr && (m_pDefaultTextureCube = GetTextureCube(g_pEngine->GetDefaultTextureCubeName())) == nullptr)
        Panic("GetDefaultTextureCube() called, and the default texture failed to load.");

    m_pDefaultTextureCube->AddRef();
    return m_pDefaultTextureCube;
}


const Texture2D *ResourceManager::SafeGetTexture2D(const char *Name)
{
    if (Name != nullptr)
    {
        const Texture2D *pTexture2D = GetTexture2D(Name);
        if (pTexture2D != nullptr)
            return pTexture2D;
    }

    return GetDefaultTexture2D();
}

const Texture2DArray *ResourceManager::SafeGetTexture2DArray(const char *Name)
{
    if (Name != nullptr)
    {
        const Texture2DArray *pTexture2DArray = GetTexture2DArray(Name);
        if (pTexture2DArray != nullptr)
            return pTexture2DArray;
    }

    return GetDefaultTexture2DArray();
}

const TextureCube *ResourceManager::SafeGetTextureCube(const char *Name)
{
    if (Name != nullptr)
    {
        const TextureCube *pTextureCube = GetTextureCube(Name);
        if (pTextureCube != nullptr)
            return pTextureCube;
    }

    return GetDefaultTextureCube();
}

StaticMesh *ResourceManager::LoadStaticMesh(const char *Name)
{
    PathString fileName;
    StaticMesh *pStaticMesh = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".staticmesh", ".staticmesh.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.staticmesh", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pStaticMesh = new StaticMesh();
                if (!pStaticMesh->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadStaticMesh: Failed to load StaticMesh '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pStaticMesh->Release();
                    pStaticMesh = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadStaticMesh: Failed to load StaticMesh '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile StaticMesh
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileStaticMesh(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // release compiler here since we're no longer using it and something else could trigger compiling
                    ReleaseResourceCompilerInterface(pCompilerInterface);

                    // write it to disk
                    fileName.Format("%s.staticmesh", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadStaticMesh: Failed to write StaticMesh '%s' to disk.", resourceName.GetCharArray());

                    // load the StaticMesh from memory (saves a round-trip to the disk)
                    pStaticMesh = new StaticMesh();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pStaticMesh->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadStaticMesh: Failed to load just-compiled StaticMesh '%s'.", resourceName.GetCharArray());
                        pStaticMesh->Release();
                        pStaticMesh = nullptr;
                    }
                }
                else
                {
                    // compile StaticMesh failed
                    Log_ErrorPrintf("ResourceManager::LoadStaticMesh: Failed to compile StaticMesh '%s'.", resourceName.GetCharArray());
                    ReleaseResourceCompilerInterface(pCompilerInterface);
                }
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadStaticMesh: Could not compile StaticMesh '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadStaticMesh: StaticMesh '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pStaticMesh != nullptr)
        Log_ProfilePrintf("PROFILE: StaticMesh load of '%s' took %.3f msec", pStaticMesh->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pStaticMesh;
}

const StaticMesh *ResourceManager::GetStaticMesh(const char *Name)
{
    StaticMeshTable::Member *pMember = m_htStaticMeshes.Find(Name);
    if (pMember == nullptr)
    {
        StaticMesh *pStaticMesh = LoadStaticMesh(Name);
        pMember = m_htStaticMeshes.Insert((pStaticMesh != nullptr) ? pStaticMesh->GetName().GetCharArray() : Name, pStaticMesh);
    }
    
    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const StaticMesh *ResourceManager::UncachedGetStaticMesh(const char *Name)
{
    StaticMeshTable::Member *pMember = m_htStaticMeshes.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadStaticMesh(Name);
}

const StaticMesh *ResourceManager::GetDefaultStaticMesh()
{
    if (m_pDefaultStaticMesh == nullptr && (m_pDefaultStaticMesh = GetStaticMesh(g_pEngine->GetDefaultStaticMeshName())) == nullptr)
        Panic("GetDefaultStaticMesh() called, and the default texture failed to load.");

    m_pDefaultStaticMesh->AddRef();
    return m_pDefaultStaticMesh;
}

const StaticMesh *ResourceManager::SafeGetStaticMesh(const char *Name)
{
    if (Name != nullptr)
    {
        const StaticMesh *pStaticMesh = GetStaticMesh(Name);
        if (pStaticMesh != nullptr)
            return pStaticMesh;
    }

    return GetDefaultStaticMesh();
}

Font *ResourceManager::LoadFont(const char *Name)
{
    PathString fileName;
    Font *pFont = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // get extension for current texture platform
    TEXTURE_PLATFORM texturePlatform = (g_pRenderer != nullptr) ? g_pRenderer->GetTexturePlatform() : TEXTURE_PLATFORM_DXTC;
    TinyString texturePlatformExtension;
    texturePlatformExtension.Format(".tex_%s", NameTable_GetNameString(NameTables::TexturePlatformFileExtension, texturePlatform));

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, texturePlatformExtension, ".font.zip", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pFont = new Font();
                if (!pFont->LoadFromStream(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadFont: Failed to load Font '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pFont->Release();
                    pFont = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadFont: Failed to load Font '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile Font
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileFont(texturePlatform, resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadFont: Failed to write Font '%s' to disk.", resourceName.GetCharArray());

                    // load the Font from memory (saves a round-trip to the disk)
                    pFont = new Font();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pFont->LoadFromStream(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadFont: Failed to load just-compiled Font '%s'.", resourceName.GetCharArray());
                        pFont->Release();
                        pFont = nullptr;
                    }
                }
                else
                {
                    // compile Font failed
                    Log_ErrorPrintf("ResourceManager::LoadFont: Failed to compile Font '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadFont: Could not compile Font '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadFont: Font '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pFont != nullptr)
        Log_ProfilePrintf("PROFILE: Font load of '%s' took %.3f msec", pFont->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pFont;
}

const Font *ResourceManager::GetFont(const char *name)
{
    FontTable::Member *pMember = m_htFonts.Find(name);
    if (pMember == nullptr)
    {
        Font *pFont = LoadFont(name);
        pMember = m_htFonts.Insert((pFont != nullptr) ? pFont->GetName().GetCharArray() : name, pFont);
    }
     
    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const Font *ResourceManager::UncachedGetFont(const char *Name)
{
    FontTable::Member *pMember = m_htFonts.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadFont(Name);
}

BlockPalette *ResourceManager::LoadBlockPalette(const char *Name)
{
    PathString fileName;
    BlockPalette *pBlockPalette = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // get extension for current texture platform
    TEXTURE_PLATFORM texturePlatform = (g_pRenderer != nullptr) ? g_pRenderer->GetTexturePlatform() : TEXTURE_PLATFORM_DXTC;
    TinyString texturePlatformExtension;
    texturePlatformExtension.Format(".blp_%s", NameTable_GetNameString(NameTables::TexturePlatformFileExtension, texturePlatform));

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, texturePlatformExtension, ".blp.zip", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pBlockPalette = new BlockPalette();
                if (!pBlockPalette->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadBlockPalette: Failed to load BlockPalette '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pBlockPalette->Release();
                    pBlockPalette = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadBlockPalette: Failed to load BlockPalette '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile BlockPalette
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileBlockPalette(texturePlatform, resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // release compiler
                    ReleaseResourceCompilerInterface(pCompilerInterface);

                    // write it to disk
                    fileName.Format("%s%s", resourceName.GetCharArray(), texturePlatformExtension.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadBlockPalette: Failed to write BlockPalette '%s' to disk.", resourceName.GetCharArray());

                    // load the BlockPalette from memory (saves a round-trip to the disk)
                    pBlockPalette = new BlockPalette();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pBlockPalette->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadBlockPalette: Failed to load just-compiled BlockPalette '%s'.", resourceName.GetCharArray());
                        pBlockPalette->Release();
                        pBlockPalette = nullptr;
                    }
                }
                else
                {
                    // compile BlockPalette failed
                    Log_ErrorPrintf("ResourceManager::LoadBlockPalette: Failed to compile BlockPalette '%s'.", resourceName.GetCharArray());
                    ReleaseResourceCompilerInterface(pCompilerInterface);
                }
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadBlockPalette: Could not compile BlockPalette '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadBlockPalette: BlockPalette '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pBlockPalette != nullptr)
        Log_ProfilePrintf("PROFILE: BlockPalette load of '%s' took %.3f msec", pBlockPalette->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pBlockPalette;
}

const BlockPalette *ResourceManager::GetBlockPalette(const char *Name)
{
    BlockPaletteTable::Member *pMember = m_htBlockPalette.Find(Name);
    if (pMember == nullptr)
    {
        BlockPalette *pPalette = LoadBlockPalette(Name);
        pMember = m_htBlockPalette.Insert((pPalette != nullptr) ? pPalette->GetName().GetCharArray() : Name, pPalette);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const BlockPalette *ResourceManager::UncachedGetBlockPalette(const char *Name)
{
    BlockPaletteTable::Member *pMember = m_htBlockPalette.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadBlockPalette(Name);
}

TerrainLayerList *ResourceManager::LoadTerrainLayerList(const char *Name)
{
    PathString fileName;
    TerrainLayerList *pTerrainLayerList = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".layerlist", ".layerlist.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.layerlist", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pTerrainLayerList = new TerrainLayerList();
                if (!pTerrainLayerList->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadTerrainLayerList: Failed to load TerrainLayerList '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pTerrainLayerList->Release();
                    pTerrainLayerList = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadTerrainLayerList: Failed to load TerrainLayerList '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile TerrainLayerList
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileTerrainLayerList(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.layerlist", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadTerrainLayerList: Failed to write TerrainLayerList '%s' to disk.", resourceName.GetCharArray());

                    // load the TerrainLayerList from memory (saves a round-trip to the disk)
                    pTerrainLayerList = new TerrainLayerList();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pTerrainLayerList->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadTerrainLayerList: Failed to load just-compiled TerrainLayerList '%s'.", resourceName.GetCharArray());
                        pTerrainLayerList->Release();
                        pTerrainLayerList = nullptr;
                    }
                }
                else
                {
                    // compile TerrainLayerList failed
                    Log_ErrorPrintf("ResourceManager::LoadTerrainLayerList: Failed to compile TerrainLayerList '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadTerrainLayerList: Could not compile TerrainLayerList '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadTerrainLayerList: TerrainLayerList '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pTerrainLayerList != nullptr)
        Log_ProfilePrintf("PROFILE: TerrainLayerList load of '%s' took %.3f msec", pTerrainLayerList->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pTerrainLayerList;
}

const TerrainLayerList *ResourceManager::GetTerrainLayerList(const char *Name)
{
    TerrainLayerListTable::Member *pMember = m_htTerrainLayerList.Find(Name);
    if (pMember == nullptr)
    {
        TerrainLayerList *pLayerList = LoadTerrainLayerList(Name);
        pMember = m_htTerrainLayerList.Insert((pLayerList != nullptr) ? pLayerList->GetName().GetCharArray() : Name, pLayerList);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const TerrainLayerList *ResourceManager::UncachedGetTerrainLayerList(const char *Name)
{
    TerrainLayerListTable::Member *pMember = m_htTerrainLayerList.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadTerrainLayerList(Name);
}

BlockMesh *ResourceManager::LoadBlockMesh(const char *Name)
{
    PathString fileName;
    BlockMesh *pBlockMesh = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".blm", ".blm.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.blm", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pBlockMesh = new BlockMesh();
                if (!pBlockMesh->Load(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadBlockMesh: Failed to load BlockMesh '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pBlockMesh->Release();
                    pBlockMesh = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadBlockMesh: Failed to load BlockMesh '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile BlockMesh
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileBlockMesh(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.mtl", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadBlockMesh: Failed to write BlockMesh '%s' to disk.", resourceName.GetCharArray());

                    // load the BlockMesh from memory (saves a round-trip to the disk)
                    pBlockMesh = new BlockMesh();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pBlockMesh->Load(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadBlockMesh: Failed to load just-compiled BlockMesh '%s'.", resourceName.GetCharArray());
                        pBlockMesh->Release();
                        pBlockMesh = nullptr;
                    }
                }
                else
                {
                    // compile BlockMesh failed
                    Log_ErrorPrintf("ResourceManager::LoadBlockMesh: Failed to compile BlockMesh '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadBlockMesh: Could not compile BlockMesh '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadBlockMesh: BlockMesh '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pBlockMesh != nullptr)
        Log_ProfilePrintf("PROFILE: BlockMesh load of '%s' took %.3f msec", pBlockMesh->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pBlockMesh;
}

const BlockMesh *ResourceManager::GetBlockMesh(const char *Name)
{
    BlockMeshTable::Member *pMember = m_htBlockMesh.Find(Name);
    if (pMember == nullptr)
    {
        BlockMesh *pBlockMesh = LoadBlockMesh(Name);
        pMember = m_htBlockMesh.Insert((pBlockMesh != nullptr) ? pBlockMesh->GetName().GetCharArray() : Name, pBlockMesh);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const BlockMesh *ResourceManager::UncachedGetBlockMesh(const char *Name)
{
    BlockMeshTable::Member *pMember = m_htBlockMesh.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadBlockMesh(Name);
}

const BlockMesh *ResourceManager::GetDefaultBlockMesh()
{
    if (m_pDefaultBlockMesh == nullptr && (m_pDefaultBlockMesh = GetBlockMesh(g_pEngine->GetDefaultBlockMeshName())) == nullptr)
        Panic("GetDefaultBlockMesh() called, and the default mesh failed to load.");

    m_pDefaultBlockMesh->AddRef();
    return m_pDefaultBlockMesh;
}

const BlockMesh *ResourceManager::SafeGetBlockMesh(const char *Name)
{
    if (Name != nullptr)
    {
        const BlockMesh *pStaticBlockMesh = GetBlockMesh(Name);
        if (pStaticBlockMesh != nullptr)
            return pStaticBlockMesh;
    }

    return GetDefaultBlockMesh();
}

Skeleton *ResourceManager::LoadSkeleton(const char *Name)
{
    PathString fileName;
    Skeleton *pSkeleton = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".skl", ".skl.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.skl", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pSkeleton = new Skeleton();
                if (!pSkeleton->LoadFromStream(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadSkeleton: Failed to load Skeleton '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pSkeleton->Release();
                    pSkeleton = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadSkeleton: Failed to load Skeleton '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile Skeleton
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileSkeleton(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.skl", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadSkeleton: Failed to write Skeleton '%s' to disk.", resourceName.GetCharArray());

                    // load the Skeleton from memory (saves a round-trip to the disk)
                    pSkeleton = new Skeleton();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pSkeleton->LoadFromStream(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadSkeleton: Failed to load just-compiled Skeleton '%s'.", resourceName.GetCharArray());
                        pSkeleton->Release();
                        pSkeleton = nullptr;
                    }
                }
                else
                {
                    // compile Skeleton failed
                    Log_ErrorPrintf("ResourceManager::LoadSkeleton: Failed to compile Skeleton '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadSkeleton: Could not compile Skeleton '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadSkeleton: Skeleton '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pSkeleton != nullptr)
        Log_ProfilePrintf("PROFILE: Skeleton load of '%s' took %.3f msec", pSkeleton->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pSkeleton;
}

const Skeleton *ResourceManager::GetSkeleton(const char *name)
{
    SkeletonTable::Member *pMember = m_htSkeleton.Find(name);
    if (pMember == nullptr)
    {
        Skeleton *pSkeleton = LoadSkeleton(name);
        pMember = m_htSkeleton.Insert((pSkeleton != nullptr) ? pSkeleton->GetName().GetCharArray() : name, pSkeleton);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const Skeleton *ResourceManager::UncachedGetSkeleton(const char *Name)
{
    SkeletonTable::Member *pMember = m_htSkeleton.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadSkeleton(Name);
}

SkeletalMesh *ResourceManager::LoadSkeletalMesh(const char *Name)
{
    PathString fileName;
    SkeletalMesh *pSkeletalMesh = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".skm", ".skm.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.skm", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pSkeletalMesh = new SkeletalMesh();
                if (!pSkeletalMesh->LoadFromStream(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadSkeletalMesh: Failed to load SkeletalMesh '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pSkeletalMesh->Release();
                    pSkeletalMesh = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadSkeletalMesh: Failed to load SkeletalMesh '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile SkeletalMesh
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileSkeletalMesh(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.skm", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadSkeletalMesh: Failed to write SkeletalMesh '%s' to disk.", resourceName.GetCharArray());

                    // load the SkeletalMesh from memory (saves a round-trip to the disk)
                    pSkeletalMesh = new SkeletalMesh();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pSkeletalMesh->LoadFromStream(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadSkeletalMesh: Failed to load just-compiled SkeletalMesh '%s'.", resourceName.GetCharArray());
                        pSkeletalMesh->Release();
                        pSkeletalMesh = nullptr;
                    }
                }
                else
                {
                    // compile SkeletalMesh failed
                    Log_ErrorPrintf("ResourceManager::LoadSkeletalMesh: Failed to compile SkeletalMesh '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadSkeletalMesh: Could not compile SkeletalMesh '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadSkeletalMesh: SkeletalMesh '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pSkeletalMesh != nullptr)
        Log_ProfilePrintf("PROFILE: SkeletalMesh load of '%s' took %.3f msec", pSkeletalMesh->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pSkeletalMesh;
}

const SkeletalMesh *ResourceManager::GetSkeletalMesh(const char *name)
{
    SkeletalMeshTable::Member *pMember = m_htSkeletalMesh.Find(name);
    if (pMember == nullptr)
    {
        SkeletalMesh *pSkeletalMesh = LoadSkeletalMesh(name);
        pMember = m_htSkeletalMesh.Insert((pSkeletalMesh != nullptr) ? pSkeletalMesh->GetName().GetCharArray() : name, pSkeletalMesh);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const SkeletalMesh *ResourceManager::UncachedGetSkeletalMesh(const char *Name)
{
    SkeletalMeshTable::Member *pMember = m_htSkeletalMesh.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadSkeletalMesh(Name);
}

const SkeletalMesh *ResourceManager::GetDefaultSkeletalMesh()
{
    if (m_pDefaultSkeletalMesh == nullptr && (m_pDefaultSkeletalMesh = GetSkeletalMesh(g_pEngine->GetDefaultBlockMeshName())) == nullptr)
        Panic("GetDefaultSkeletalMesh() called, and the default mesh failed to load.");

    m_pDefaultSkeletalMesh->AddRef();
    return m_pDefaultSkeletalMesh;
}

const SkeletalMesh *ResourceManager::SafeGetSkeletalMesh(const char *Name)
{
    if (Name != nullptr)
    {
        const SkeletalMesh *pSkeletalMesh = GetSkeletalMesh(Name);
        if (pSkeletalMesh != nullptr)
            return pSkeletalMesh;
    }

    return GetDefaultSkeletalMesh();
}

SkeletalAnimation *ResourceManager::LoadSkeletalAnimation(const char *Name)
{
    PathString fileName;
    SkeletalAnimation *pSkeletalAnimation = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".ska", ".ska.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.ska", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pSkeletalAnimation = new SkeletalAnimation();
                if (!pSkeletalAnimation->LoadFromStream(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadSkeletalAnimation: Failed to load SkeletalAnimation '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pSkeletalAnimation->Release();
                    pSkeletalAnimation = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadSkeletalAnimation: Failed to load SkeletalAnimation '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile SkeletalAnimation
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileSkeletalAnimation(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.ska", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadSkeletalAnimation: Failed to write SkeletalAnimation '%s' to disk.", resourceName.GetCharArray());

                    // load the SkeletalAnimation from memory (saves a round-trip to the disk)
                    pSkeletalAnimation = new SkeletalAnimation();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pSkeletalAnimation->LoadFromStream(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadSkeletalAnimation: Failed to load just-compiled SkeletalAnimation '%s'.", resourceName.GetCharArray());
                        pSkeletalAnimation->Release();
                        pSkeletalAnimation = nullptr;
                    }
                }
                else
                {
                    // compile SkeletalAnimation failed
                    Log_ErrorPrintf("ResourceManager::LoadSkeletalAnimation: Failed to compile SkeletalAnimation '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadSkeletalAnimation: Could not compile SkeletalAnimation '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadSkeletalAnimation: SkeletalAnimation '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pSkeletalAnimation != nullptr)
        Log_ProfilePrintf("PROFILE: SkeletalAnimation load of '%s' took %.3f msec", pSkeletalAnimation->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pSkeletalAnimation;
}

const SkeletalAnimation *ResourceManager::GetSkeletalAnimation(const char *name)
{
    SkeletalAnimationTable::Member *pMember = m_htSkeletalAnimation.Find(name);
    if (pMember == nullptr)
    {
        SkeletalAnimation *pSkeletalAnimation = LoadSkeletalAnimation(name);
        pMember = m_htSkeletalAnimation.Insert((pSkeletalAnimation != nullptr) ? pSkeletalAnimation->GetName().GetCharArray() : name, pSkeletalAnimation);
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const SkeletalAnimation *ResourceManager::UncachedGetSkeletalAnimation(const char *Name)
{
    SkeletalAnimationTable::Member *pMember = m_htSkeletalAnimation.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadSkeletalAnimation(Name);
}

ParticleSystem *ResourceManager::LoadParticleSystem(const char *Name)
{
    PathString fileName;
    ParticleSystem *pParticleSystem = nullptr;

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    Timer loadTimer;
#endif

    // build name (for now)
    PathString resourceName;
    resourceName.AppendString(Name);

    // see what we have
    bool compileResource, hasSourceVersion, hasCompiledVersion;
    if (GetResourceStatus(resourceName, ".ParticleSystem", ".ParticleSystem.xml", compileResource, hasSourceVersion, hasCompiledVersion))
    {
        // loading the compiled version?
        if (hasCompiledVersion)
        {
            AutoReleasePtr<ByteStream> pStream;
            fileName.Format("%s.ParticleSystem", resourceName.GetCharArray());
            pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // load it
                pParticleSystem = new ParticleSystem();
                if (!pParticleSystem->LoadFromStream(resourceName, pStream))
                {
                    // log the error, then try to recompile it
                    Log_ErrorPrintf("ResourceManager::LoadParticleSystem: Failed to load ParticleSystem '%s', read failed.", resourceName.GetCharArray());
                    compileResource = hasSourceVersion;
                    pParticleSystem->Release();
                    pParticleSystem = nullptr;
                }
            }
            else
            {
                // open failed
                Log_ErrorPrintf("ResourceManager::LoadParticleSystem: Failed to load ParticleSystem '%s', open failed.", resourceName.GetCharArray());
            }
        }

        // loading the uncompiled version?
        if (compileResource)
        {
            // open resource compiler
            ResourceCompilerInterface *pCompilerInterface = GetResourceCompilerInterface();
            if (pCompilerInterface != nullptr)
            {
                // compile ParticleSystem
                AutoReleasePtr<BinaryBlob> pCompiledBlob = pCompilerInterface->CompileParticleSystem(resourceName);
                if (pCompiledBlob != nullptr)
                {
                    // write it to disk
                    fileName.Format("%s.ParticleSystem", resourceName.GetCharArray());
                    if (!g_pVirtualFileSystem->PutFileContents(fileName, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize(), true, true))
                        Log_WarningPrintf("ResourceManager::LoadParticleSystem: Failed to write ParticleSystem '%s' to disk.", resourceName.GetCharArray());

                    // load the ParticleSystem from memory (saves a round-trip to the disk)
                    pParticleSystem = new ParticleSystem();
                    AutoReleasePtr<ByteStream> pStream = pCompiledBlob->CreateReadOnlyStream();
                    if (!pParticleSystem->LoadFromStream(resourceName, pStream))
                    {
                        Log_ErrorPrintf("ResourceManager::LoadParticleSystem: Failed to load just-compiled ParticleSystem '%s'.", resourceName.GetCharArray());
                        pParticleSystem->Release();
                        pParticleSystem = nullptr;
                    }
                }
                else
                {
                    // compile ParticleSystem failed
                    Log_ErrorPrintf("ResourceManager::LoadParticleSystem: Failed to compile ParticleSystem '%s'.", resourceName.GetCharArray());
                }

                // release compiler
                ReleaseResourceCompilerInterface(pCompilerInterface);
            }
            else
            {
                // open resource compiler failed
                Log_ErrorPrintf("ResourceManager::LoadParticleSystem: Could not compile ParticleSystem '%s', no compiler available.", resourceName.GetCharArray());
            }
        }
    }
    else
    {
        Log_WarningPrintf("ResourceManager::LoadParticleSystem: ParticleSystem '%s' is unavailable or does not exit.", resourceName.GetCharArray());
    }

#if PROFILE_RESOURCEMANAGER_LOAD_TIMES
    if (pParticleSystem != nullptr)
        Log_ProfilePrintf("PROFILE: ParticleSystem load of '%s' took %.3f msec", pParticleSystem->GetName().GetCharArray(), loadTimer.GetTimeMilliseconds());
#endif

    return pParticleSystem;
}

const ParticleSystem *ResourceManager::GetParticleSystem(const char *name)
{
    m_resourceLock.LockShared();

    ParticleSystemTable::Member *pMember = m_htParticleSystem.Find(name);
    if (pMember == nullptr)
    {
        m_resourceLock.UnlockShared();

        ParticleSystem *pParticleSystem = LoadParticleSystem(name);

        m_resourceLock.LockExclusive();
        pMember = m_htParticleSystem.Find(name);
        if (pMember == nullptr)
            pMember = m_htParticleSystem.Insert((pParticleSystem != nullptr) ? pParticleSystem->GetName().GetCharArray() : name, pParticleSystem);
        else if (pParticleSystem != nullptr)
            pParticleSystem->Release();
        m_resourceLock.UnlockExclusive();
    }
    else
    {
        m_resourceLock.UnlockShared();
    }

    if (pMember->Value != nullptr)
        pMember->Value->AddRef();

    return pMember->Value;
}

const ParticleSystem *ResourceManager::UncachedGetParticleSystem(const char *Name)
{
    ParticleSystemTable::Member *pMember = m_htParticleSystem.Find(Name);
    if (pMember != nullptr)
    {
        if (pMember->Value != nullptr)
            pMember->Value->AddRef();

        return pMember->Value;
    }

    return LoadParticleSystem(Name);
}

void ResourceManager::CreateDeviceResources()
{
    // recreate lost resources
    Log_InfoPrint("ResourceManager::CreateDeviceResources() Recreating lost resources...");

    // textures
    for (TextureTable::Iterator itr = m_htTextures.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateDeviceResources();
    }

    // shader
    for (MaterialShaderTable::Iterator itr = m_htMaterialShaders.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateDeviceResources();
    }

    // material
    for (MaterialTable::Iterator itr = m_htMaterials.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateDeviceResources();
    }

    // staticmesh
    for (StaticMeshTable::Iterator itr = m_htStaticMeshes.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateGPUResources();
    }

    // blocklists
    for (BlockPaletteTable::Iterator itr = m_htBlockPalette.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateGPUResources();
    }

    // static block meshes
    for (BlockMeshTable::Iterator itr = m_htBlockMesh.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateGPUResources();
    }

    // skeletal meshes
    for (SkeletalMeshTable::Iterator itr = m_htSkeletalMesh.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->CreateGPUResources();
    }
}

void ResourceManager::ReleaseDeviceResources()
{
    Log_InfoPrint("ResourceManager::ReleaseDeviceResources()");

    // skeletal meshes
    for (SkeletalMeshTable::Iterator itr = m_htSkeletalMesh.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseGPUResources();
    }

    // static block meshes
    for (BlockMeshTable::Iterator itr = m_htBlockMesh.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseGPUResources();
    }

    // terrain layer lists
    for (TerrainLayerListTable::Iterator itr = m_htTerrainLayerList.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseGPUResources();
    }

    // blocklists
    for (BlockPaletteTable::Iterator itr = m_htBlockPalette.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseGPUResources();
    }

    // staticmesh
    for (StaticMeshTable::Iterator itr = m_htStaticMeshes.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseGPUResources();
    }

    // material
    for (MaterialTable::Iterator itr = m_htMaterials.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseDeviceResources();
    }

    // shader
    for (MaterialShaderTable::Iterator itr = m_htMaterialShaders.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseDeviceResources();
    }

    // textures
    for (TextureTable::Iterator itr = m_htTextures.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value != NULL)
            itr->Value->ReleaseDeviceResources();
    }
}

// the way this is currently done is a bit of a hack, basically we add a reference, then release it immediately,
// giving the current reference count on return, if this is one, the only copy left is inside the resource manager,
// so we drop it. if the pointer is null, we just remove it anyway.
void ResourceManager::CompactResources()
{
    uint32 nRefCount;
    Log_DevPrint("ResourceManager is compacting managed resources...");

    // delete skeletal animation sets
    for (ParticleSystemTable::Iterator itr = m_htParticleSystem.Begin(); !itr.AtEnd();)
    {
        ParticleSystemTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping ParticleSystem '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htParticleSystem.Remove(pMember);
    }

    // delete skeletal animation sets
    for (SkeletalAnimationTable::Iterator itr = m_htSkeletalAnimation.Begin(); !itr.AtEnd();)
    {
        SkeletalAnimationTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping SkeletalAnimationSet '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htSkeletalAnimation.Remove(pMember);
    }

    // delete skeletal meshes
    for (SkeletalMeshTable::Iterator itr = m_htSkeletalMesh.Begin(); !itr.AtEnd();)
    {
        SkeletalMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping SkeletalMesh '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htSkeletalMesh.Remove(pMember);
    }

    // delete skeletons
    for (SkeletonTable::Iterator itr = m_htSkeleton.Begin(); !itr.AtEnd();)
    {
        SkeletonTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping Skeleton '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htSkeleton.Remove(pMember);
    }

    // delete block meshes
    for (BlockMeshTable::Iterator itr = m_htBlockMesh.Begin(); !itr.AtEnd(); )
    {
        BlockMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping BlockMesh '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htBlockMesh.Remove(pMember);
    }

    // delete terrain layer lists
    for (TerrainLayerListTable::Iterator itr = m_htTerrainLayerList.Begin(); !itr.AtEnd();)
    {
        TerrainLayerListTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping TerrainLayerList '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htTerrainLayerList.Remove(pMember);
    }

    // delete block mesh block lists
    for (BlockPaletteTable::Iterator itr = m_htBlockPalette.Begin(); !itr.AtEnd(); )
    {
        BlockPaletteTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping BlockMeshBlockList '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htBlockPalette.Remove(pMember);
    }

    // delete font data
    for (FontTable::Iterator itr = m_htFonts.Begin(); !itr.AtEnd(); )
    {
        FontTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping FontData '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htFonts.Remove(pMember);
    }

    // delete staticmesh
    for (StaticMeshTable::Iterator itr = m_htStaticMeshes.Begin(); !itr.AtEnd(); )
    {
        StaticMeshTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping StaticMesh '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htStaticMeshes.Remove(pMember);
    }

    // delete materials
    for (MaterialTable::Iterator itr = m_htMaterials.Begin(); !itr.AtEnd(); )
    {
        MaterialTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping Material '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htMaterials.Remove(pMember);
    }

    // delete material shaders
    for (MaterialShaderTable::Iterator itr = m_htMaterialShaders.Begin(); !itr.AtEnd(); )
    {
        MaterialShaderTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping MaterialShader '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htMaterialShaders.Remove(pMember);
    }

    // delete textures
    for (TextureTable::Iterator itr = m_htTextures.Begin(); !itr.AtEnd(); )
    {
        TextureTable::Member *pMember = &(*itr++);
        if (pMember->Value != NULL)
        {
            pMember->Value->AddRef();
            if (pMember->Value->Release() > 1)
                continue;

            // drop last reference
            Log_DevPrintf("  dropping Texture '%s'...", pMember->Value->GetName().GetCharArray());
            nRefCount = pMember->Value->Release();
            DebugAssert(nRefCount == 0);
        }

        m_htTextures.Remove(pMember);
    }
}

const Resource *ResourceManager::GetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *Name)
{
    if (pResourceTypeInfo->IsDerived(Texture::StaticTypeInfo()))
        return GetTexture(Name);
    else if (pResourceTypeInfo == Material::StaticTypeInfo())
        return GetMaterial(Name);
    else if (pResourceTypeInfo == MaterialShader::StaticTypeInfo())
        return GetMaterialShader(Name);
    else if (pResourceTypeInfo == StaticMesh::StaticTypeInfo())
        return GetStaticMesh(Name);
    else if (pResourceTypeInfo == BlockMesh::StaticTypeInfo())
        return GetBlockMesh(Name);
    else if (pResourceTypeInfo == Skeleton::StaticTypeInfo())
        return GetSkeleton(Name);
    else if (pResourceTypeInfo == SkeletalMesh::StaticTypeInfo())
        return GetSkeletalMesh(Name);
    else if (pResourceTypeInfo == SkeletalAnimation::StaticTypeInfo())
        return GetSkeletalAnimation(Name);
    
    return NULL;
}

const Resource *ResourceManager::SafeGetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *Name)
{
    if (pResourceTypeInfo->IsDerived(Texture::StaticTypeInfo()))
    {
        if (pResourceTypeInfo == Texture2D::StaticTypeInfo())
            return SafeGetTexture2D(Name);
        else if (pResourceTypeInfo == TextureCube::StaticTypeInfo())
            return SafeGetTextureCube(Name);
    }
    else if (pResourceTypeInfo == Material::StaticTypeInfo())
        return SafeGetMaterial(Name);
    else if (pResourceTypeInfo == MaterialShader::StaticTypeInfo())
        return SafeGetMaterialShader(Name);
    else if (pResourceTypeInfo == StaticMesh::StaticTypeInfo())
        return SafeGetStaticMesh(Name);
    else if (pResourceTypeInfo == BlockMesh::StaticTypeInfo())
        return SafeGetBlockMesh(Name);
    else if (pResourceTypeInfo == SkeletalMesh::StaticTypeInfo())
        return SafeGetSkeletalMesh(Name);

    return NULL;
}

const Resource *ResourceManager::UncachedGetResource(const ResourceTypeInfo *pResourceTypeInfo, const char *Name)
{
    if (pResourceTypeInfo->IsDerived(Texture::StaticTypeInfo()))
        return UncachedGetTexture(Name);
    else if (pResourceTypeInfo == Material::StaticTypeInfo())
        return UncachedGetMaterial(Name);
    else if (pResourceTypeInfo == MaterialShader::StaticTypeInfo())
        return UncachedGetMaterialShader(Name);
    else if (pResourceTypeInfo == StaticMesh::StaticTypeInfo())
        return UncachedGetStaticMesh(Name);
    else if (pResourceTypeInfo == BlockMesh::StaticTypeInfo())
        return UncachedGetBlockMesh(Name);
    else if (pResourceTypeInfo == Skeleton::StaticTypeInfo())
        return UncachedGetSkeleton(Name);
    else if (pResourceTypeInfo == SkeletalMesh::StaticTypeInfo())
        return UncachedGetSkeletalMesh(Name);
    else if (pResourceTypeInfo == SkeletalAnimation::StaticTypeInfo())
        return UncachedGetSkeletalAnimation(Name);

    return NULL;
}

void ResourceManager::SetResourceModificationDetectionEnabled(bool enabled)
{
    if (!enabled)
    {
        if (m_pResourceModificationChangeNotifier != nullptr)
        {
            delete m_pResourceModificationChangeNotifier;
            m_pResourceModificationChangeNotifier = nullptr;

            Log_InfoPrint("ResourceManager: Resource modification detection disabled.");
        }
    }
    else
    {
        m_pResourceModificationChangeNotifier = g_pVirtualFileSystem->CreateChangeNotifier();

        if (m_pResourceModificationChangeNotifier != nullptr)
            Log_InfoPrint("ResourceManager: Resource modification detection enabled successfully.");
        else
            Log_WarningPrint("ResourceManager: Failed to enable resource modification detection.");
    }
}

void ResourceManager::CheckForModifiedResources()
{
    if (m_pResourceModificationChangeNotifier == nullptr)
        return;

    m_pResourceModificationChangeNotifier->EnumerateChanges([](const FileSystem::ChangeNotifier::ChangeInfo *pChangeInfo)
    {
        const char *changeTypeStr = "unknown";
        if (pChangeInfo->Event == FileSystem::ChangeNotifier::ChangeEvent_FileAdded)
            changeTypeStr = "ChangeEvent_FileAdded";
        else if (pChangeInfo->Event == FileSystem::ChangeNotifier::ChangeEvent_FileRemoved)
            changeTypeStr = "ChangeEvent_FileRemoved";
        else if (pChangeInfo->Event == FileSystem::ChangeNotifier::ChangeEvent_FileModified)
            changeTypeStr = "ChangeEvent_FileModified";
        else if (pChangeInfo->Event == FileSystem::ChangeNotifier::ChangeEvent_RenamedOldName)
            changeTypeStr = "ChangeEvent_RenamedOldName";
        else if (pChangeInfo->Event == FileSystem::ChangeNotifier::ChangeEvent_RenamedNewName)
            changeTypeStr = "ChangeEvent_RenamedNewName";

        Log_DevPrintf("ResourceManager::CheckForModifiedResources: Change %s on file '%s'", changeTypeStr, pChangeInfo->Path);
    });
}

ResourceCompilerInterface *ResourceManager::GetResourceCompilerInterface()
{
#if defined(WITH_RESOURCECOMPILER_SUBPROCESS)
    // hold the lock
    m_resourceCompilerLock.Lock();

    // primary in use?
    if (m_resourceCompilerInUse)
    {
        // resource compiler is in use, so spawn another one
        return ResourceCompilerInterface::CreateRemoteInterface();
    }

    // if there's an existing interface, use it
    if (m_pResourceCompilerInterface != nullptr)
    {
        m_resourceCompilerInUse = true;
        return m_pResourceCompilerInterface;
    }

    // spawn nw
    m_pResourceCompilerInterface = ResourceCompilerInterface::CreateRemoteInterface();
    if (m_pResourceCompilerInterface == nullptr)
    {
        Log_ErrorPrintf("ResourceManager::GetResourceCompilerInterface: Failed to create interface.");
        m_resourceCompilerLock.Unlock();
        return nullptr;
    }

    m_resourceCompilerInUse = true;
    m_resourceCompilerSpawnTime.Reset();
    return m_pResourceCompilerInterface;
#else
    Log_ErrorPrintf("ResourceManager::GetResourceCompilerInterface: This engine was not built with ResourceCompiler support.");
    return nullptr;
#endif          // WITH_RESOURCECOMPILER_SUBPROCESS
}

void ResourceManager::ReleaseResourceCompilerInterface(ResourceCompilerInterface *pInterface)
{
#if defined(WITH_RESOURCECOMPILER_SUBPROCESS)
    if (pInterface == m_pResourceCompilerInterface)
    {
        DebugAssert(m_resourceCompilerInUse);
        m_resourceCompilerInUse = false;

        // if there is a zero delay, just close it immediately
        if (CVars::rm_remote_resource_compiler_close_delay.GetUInt() == 0)
        {
            m_pResourceCompilerInterface->Release();
            m_pResourceCompilerInterface = nullptr;
        }
    }
    else
    {
        // was one of the extra spawned ones, so just despawn it
        pInterface->Release();
    }

    // release lock
    m_resourceCompilerLock.Unlock();
#endif      // WITH_RESOURCECOMPILER_SUBPROCESS
}

void ResourceManager::Update()
{
    // perform maintenance
    uint32 timeDiff = (uint32)Math::Truncate((float)m_lastMaintenanceTime.GetTimeSeconds());
    if (timeDiff >= CVars::rm_maintenance_interval.GetUInt())
    {
        // update changed resources
        CheckForModifiedResources();

#if defined(WITH_RESOURCECOMPILER_SUBPROCESS)
        // release resource compiler if it hasn't been used in x time
        // potential race here, it just means we'll miss cleaning it up for a loop, which
        // is unlikely to have the time elapsed anyway, and it saves locking every frame
        if (m_pResourceCompilerInterface != nullptr && m_resourceCompilerLock.TryLock())
        {
            uint32 timeElapsed = (uint32)Math::Truncate((float)m_resourceCompilerSpawnTime.GetTimeSeconds());
            if (!m_resourceCompilerInUse && timeElapsed >= CVars::rm_remote_resource_compiler_close_delay.GetUInt())
            {
                // release him
                m_pResourceCompilerInterface->Release();
                m_pResourceCompilerInterface = nullptr;
            }
            m_resourceCompilerLock.Unlock();
        }
#endif          // WITH_RESOURCECOMPILER_SUBPROCESS
    }
}

