#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderMap.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Engine/MaterialShader.h"
#include "Engine/EngineCVars.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
Log_SetChannel(ShaderMap);

int32 ShaderMap::Key::Compare(const Key *a, const Key *b)
{
    return Y_memcmp(a, b, sizeof(Key));
}

ShaderMap::ShaderMap()
{

}

ShaderMap::~ShaderMap()
{
    ReleaseGPUResources();
}

ShaderProgram *ShaderMap::GetShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const
{
    // binary search the list
    Key key(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, nullptr, 0, pMaterialShader, materialShaderFlags);
    const ProgramEntry *pProgramEntry = m_arrLoadedShaders.BinarySearchKey<Key>(key, [](const Key *key, const ProgramEntry *pe) { return Key::Compare(key, &pe->Key); });
    if (pProgramEntry != nullptr)
        return pProgramEntry->Value;
    
    // load it
    return LoadShaderPermutation(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, nullptr, 0, pMaterialShader, materialShaderFlags, pVertexAttributes, nVertexAttributes);
}

ShaderProgram *ShaderMap::GetShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const
{
    // binary search the list
    Key key(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags);
    const ProgramEntry *pProgramEntry = m_arrLoadedShaders.BinarySearchKey<Key>(key, [](const Key *key, const ProgramEntry *pe) { return Key::Compare(key, &pe->Key); });
    if (pProgramEntry != nullptr)
        return pProgramEntry->Value;
    
    // load it
    GPU_VERTEX_ELEMENT_DESC vertexAttributes[GPU_INPUT_LAYOUT_MAX_ELEMENTS];
    uint32 nVertexAttributes = (pVertexFactoryTypeInfo != nullptr) ? pVertexFactoryTypeInfo->GetVertexElementsDesc(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), vertexFactoryFlags, vertexAttributes) : 0;
    return LoadShaderPermutation(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags, vertexAttributes, nVertexAttributes);
}

void ShaderMap::ReleaseGPUResources()
{
    for (uint32 i = 0; i < m_arrLoadedShaders.GetSize(); i++)
        delete m_arrLoadedShaders[i].Value;

    m_arrLoadedShaders.Obliterate();
}

ShaderProgram *ShaderMap::LoadShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes) const
{
    // stuff to load
    RENDERER_PLATFORM rendererPlatform = g_pRenderer->GetPlatform();
    RENDERER_FEATURE_LEVEL rendererFeatureLevel = g_pRenderer->GetFeatureLevel();

    // resultant gpu program
    GPUShaderProgram *pGPUProgram = nullptr;

    // get hash code for shader
    uint8 shaderHashCode[16];
    SmallString hashCodeStr;
    PathString diskCacheFileName;
    ShaderCompilerFrontend::GenerateShaderHashCode(shaderHashCode, globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags);
    StringConverter::BytesToHexString(hashCodeStr, shaderHashCode, sizeof(shaderHashCode));
    
    // can we use the disk cache?
    if (CVars::r_use_shader_cache.GetBool())
    {
        // construct filename
        diskCacheFileName.Format("shadercache/%s_%s_%s/%s.bin", NameTable_GetNameString(NameTables::RendererPlatform, rendererPlatform),
                                                                NameTable_GetNameString(NameTables::RendererFeatureLevel, rendererFeatureLevel),
                                                                (CVars::r_use_debug_shaders.GetBool()) ? "DEBUG" : "RELEASE",
                                                                hashCodeStr.GetCharArray());

        // try opening it
        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(diskCacheFileName, BYTESTREAM_OPEN_READ);
        if (pStream != nullptr)
        {
            Log_DevPrintf("ShaderMap::LoadShaderPermutation: Shader program '%s' found in cache, attempting to use it.", hashCodeStr.GetCharArray());

            // read common header
            DF_SHADER_PROGRAM_COMMON_HEADER commonHeader;
            if (!pStream->Read2(&commonHeader, sizeof(commonHeader)) || commonHeader.Magic != DF_SHADER_PROGRAM_COMMON_HEADER_MAGIC)
            {
                Log_WarningPrintf("ShaderMap::LoadShaderPermutation: Bad common magic for shader program '%s'", hashCodeStr.GetCharArray());
                pStream->Release();
            }
            else
            {
#if defined(WITH_RESOURCECOMPILER_EMBEDDED) || defined(WITH_RESOURCECOMPILER_SUBPROCESS)
                // check code changes, only if resourcecompiler is present
                uint32 baseShaderParameterCRC = (pBaseShaderTypeInfo != nullptr) ? pBaseShaderTypeInfo->GetParameterCRC() : 0;
                uint32 vertexFactoryParameterCRC = (pVertexFactoryTypeInfo != nullptr) ? pVertexFactoryTypeInfo->GetParameterCRC() : 0;
                uint32 materialShaderCRC = (pMaterialShader != nullptr) ? pMaterialShader->GetSourceCRC() : 0;
                if (commonHeader.ShaderStoreCRC != ShaderCompilerFrontend::GetShaderStoreHash() ||
                    commonHeader.BaseShaderParameterCRC != baseShaderParameterCRC ||
                    commonHeader.VertexFactoryParameterCRC != vertexFactoryParameterCRC ||
                    commonHeader.MaterialShaderCRC != materialShaderCRC)
                {
                    Log_WarningPrintf("ShaderMap::LoadShaderPermutation: Shader program '%s' is out of date. Disregarding cache version.", hashCodeStr.GetCharArray());
                    pStream->Release();
                }
                else
                {
                    // create shader
                    pGPUProgram = g_pRenderer->CreateGraphicsProgram(pVertexAttributes, nVertexAttributes, pStream);
                    pStream->Release();
                }
#else
                // create shader, since we have no compiler support
                pGPUProgram = g_pRenderer->CreateGraphicsProgram(pVertexAttributes, nVertexAttributes, pStream);
                pStream->Release();
#endif
            }
        }
        else
        {
            // log a warning
            Log_WarningPrintf("ShaderMap::LoadShaderPermutation: Shader program '%s' not found in cache, attempting compilation.", hashCodeStr.GetCharArray());
        }
    }

#if defined(WITH_CONTENTCONVERTER_EMBEDDED) || defined(WITH_RESOURCECOMPILER_SUBPROCESS)
    // is a compile necessary?
    if (pGPUProgram == nullptr)
    {
        Log_InfoPrintf("ShaderMap::LoadShaderPermutation: Compiling program (%s, %s, %s, %X, %s, %X, %s, %X) -> %s...", 
                       NameTable_GetNameString(NameTables::RendererPlatform, rendererPlatform),
                       NameTable_GetNameString(NameTables::RendererFeatureLevel, rendererFeatureLevel),
                       (pBaseShaderTypeInfo != nullptr) ? pBaseShaderTypeInfo->GetTypeName() : "NULL",
                       baseShaderFlags,
                       (pVertexFactoryTypeInfo != nullptr) ? pVertexFactoryTypeInfo->GetTypeName() : "NULL",
                       vertexFactoryFlags,
                       (pMaterialShader != nullptr) ? pMaterialShader->GetName().GetCharArray() : "NULL",
                       materialShaderFlags,
                       hashCodeStr.GetCharArray());

        // create streams
        AutoReleasePtr<GrowableMemoryByteStream> pByteCodeStream = ByteStream_CreateGrowableMemoryStream();
        AutoReleasePtr<ByteStream> pInfoLogStream = nullptr;

        // setup dump stream
        if (CVars::r_dump_shaders.GetBool())
        {
            PathString dumpFileName;
            StringConverter::BytesToHexString(hashCodeStr, shaderHashCode, sizeof(shaderHashCode));
            dumpFileName.Format("shaderdump/%s_%s_%s/%s.txt", NameTable_GetNameString(NameTables::RendererPlatform, g_pRenderer->GetPlatform()), 
                                                              NameTable_GetNameString(NameTables::RendererFeatureLevel, g_pRenderer->GetFeatureLevel()),
                                                              (CVars::r_use_debug_shaders.GetBool()) ? "DEBUG" : "RELEASE",
                                                              hashCodeStr.GetCharArray());
            
            pInfoLogStream = g_pVirtualFileSystem->OpenFile(dumpFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE);
        }

        // get resource compiler interface
        ResourceCompilerInterface *pCompilerInterface = g_pResourceManager->GetResourceCompilerInterface();
        if (pCompilerInterface != nullptr)
        {
            // forward through to compiler
            if (ShaderCompilerFrontend::CompileShader(pCompilerInterface, globalShaderFlags, g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(),
                                                      pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags,
                                                      CVars::r_use_debug_shaders.GetBool(), pByteCodeStream, pInfoLogStream))
            { 
                // write to disk cache
                if (CVars::r_use_shader_cache.GetBool() && CVars::r_allow_shader_cache_writes.GetBool())
                {
                    ByteStream *pStream = g_pVirtualFileSystem->OpenFile(diskCacheFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
                    if (pStream != nullptr)
                    {
                        // generate the common header
                        DF_SHADER_PROGRAM_COMMON_HEADER commonHeader;
                        commonHeader.Magic = DF_SHADER_PROGRAM_COMMON_HEADER_MAGIC;
                        commonHeader.ShaderStoreCRC = ShaderCompilerFrontend::GetShaderStoreHash();
                        commonHeader.BaseShaderParameterCRC = (pBaseShaderTypeInfo != nullptr) ? pBaseShaderTypeInfo->GetParameterCRC() : 0;
                        commonHeader.VertexFactoryParameterCRC = (pVertexFactoryTypeInfo != nullptr) ? pVertexFactoryTypeInfo->GetParameterCRC() : 0;
                        commonHeader.MaterialShaderCRC = (pMaterialShader != nullptr) ? pMaterialShader->GetSourceCRC() : 0;

                        // write common header and bytecode
                        pByteCodeStream->SeekAbsolute(0);
                        if (pStream->Write2(&commonHeader, sizeof(commonHeader)) && ByteStream_AppendStream(pByteCodeStream, pStream))
                            pStream->Commit();
                        else
                            pStream->Discard();

                        pStream->Release();
                    }
                }

                // create gpu object
                pByteCodeStream->SeekAbsolute(0);
                pGPUProgram = g_pRenderer->CreateGraphicsProgram(pVertexAttributes, nVertexAttributes, pByteCodeStream);
            }
            else
            {
                Log_ErrorPrintf("ShaderMap::LoadShaderPermutation: Compiling program %s failed.", hashCodeStr.GetCharArray());
            }

            // release compiler interface
            g_pResourceManager->ReleaseResourceCompilerInterface(pCompilerInterface);
        }
    }
#endif

    // set debug name
#ifdef Y_BUILD_CONFIG_DEBUG
    if (pGPUProgram != nullptr)
    {
        pGPUProgram->SetDebugName(SmallString::FromFormat("%s<0x%x>:%s<0x%x>:%s<0x%x>", 
                                                          (pBaseShaderTypeInfo != nullptr) ? pBaseShaderTypeInfo->GetTypeName() : "null", vertexFactoryFlags,
                                                          (pVertexFactoryTypeInfo != nullptr) ? pVertexFactoryTypeInfo->GetTypeName() : "null", vertexFactoryFlags,
                                                          (pMaterialShader != nullptr) ? pMaterialShader->GetName().GetCharArray() : "null", materialShaderFlags));
    }
#endif

    // got a gpu program?
    ShaderProgram *pProgram = (pGPUProgram != nullptr) ? new ShaderProgram(pGPUProgram, globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags) : nullptr;
    
    // add to list, and re-sort it
    m_arrLoadedShaders.Add(ProgramEntry(Key(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags), pProgram));
    m_arrLoadedShaders.SortCB([](const ProgramEntry &a, const ProgramEntry &b) { return Key::Compare(&a.Key, &b.Key); });
    return pProgram;
}

