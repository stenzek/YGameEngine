#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Renderer/VertexFactoryTypeInfo.h"
#include "Renderer/ShaderComponent.h"
#include "Engine/MaterialShader.h"
#include "ResourceCompilerInterface/ResourceCompilerInterface.h"
#include "YBaseLib/CRC32.h"
#include "YBaseLib/MD5Digest.h"
Log_SetChannel(ShaderCompilerFrontend);

// crc32 of all base shader sources
static uint32 s_shaderStoreHash = 0;

void ShaderCompilerFrontend::GenerateShaderHashCode(uint8 HashCode[16], 
                                                    uint32 globalShaderFlags,
                                                    const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, 
                                                    const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, 
                                                    const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    static const byte nullNamePlusFlags[5] = { 0, 0, 0, 0, 0 };
    MD5Digest md5;

    // global shader flags
    md5.Update(&globalShaderFlags, sizeof(uint32));

    // hash base shader name and flags
    if (pBaseShaderTypeInfo != NULL)
    {
        md5.Update(pBaseShaderTypeInfo->GetTypeName(), Y_strlen(pBaseShaderTypeInfo->GetTypeName()));
        md5.Update(&baseShaderFlags, sizeof(uint32));
    }
    else
    {
        md5.Update(nullNamePlusFlags, sizeof(nullNamePlusFlags));
    }

    // hash vertex factory name and flags
    if (pVertexFactoryTypeInfo != NULL)
    {
        md5.Update(pVertexFactoryTypeInfo->GetTypeName(), Y_strlen(pVertexFactoryTypeInfo->GetTypeName()));
        md5.Update(&vertexFactoryFlags, sizeof(uint32));
    }
    else
    {
        md5.Update(nullNamePlusFlags, sizeof(nullNamePlusFlags));
    }

    // hash material shader name and flags
    if (pMaterialShader != NULL)
    {
        md5.Update(pMaterialShader->GetName().GetCharArray(), pMaterialShader->GetName().GetLength());
        md5.Update(&materialShaderFlags, sizeof(uint32));
    }
    else
    {
        md5.Update(nullNamePlusFlags, sizeof(nullNamePlusFlags));
    }

    // get digest
    md5.Final(HashCode);
}

static void AddGlobalShaderDefines(uint32 globalShaderFlags, ShaderCompilerParameters *pParameters)
{
    if (globalShaderFlags & SHADER_GLOBAL_FLAG_MATERIAL_TINT)
        pParameters->AddPreprocessorMacro("MATERIAL_TINT", "1");

    if (globalShaderFlags & SHADER_GLOBAL_FLAG_SHADER_QUALITY_HIGH)
    {
        pParameters->AddPreprocessorMacro("QUALITY_HIGH", "1");
        pParameters->AddPreprocessorMacro("QUALITY_LEVEL", "3");
    }
    else if (globalShaderFlags & SHADER_GLOBAL_FLAG_SHADER_QUALITY_MEDIUM)
    {
        pParameters->AddPreprocessorMacro("QUALITY_MEDIUM", "1");
        pParameters->AddPreprocessorMacro("QUALITY_LEVEL", "2");
    }
    else if (globalShaderFlags & SHADER_GLOBAL_FLAG_SHADER_QUALITY_LOW)
    {
        pParameters->AddPreprocessorMacro("QUALITY_LOW", "1");
        pParameters->AddPreprocessorMacro("QUALITY_LEVEL", "1");
    }

    if (globalShaderFlags & SHADER_GLOBAL_FLAG_USE_VERTEX_ID_QUADS)
        pParameters->AddPreprocessorMacro("USE_VERTEX_ID_QUADS", "1");
}

static void AddMaterialShaderDefines(const MaterialShader *pMaterialShader, uint32 materialShaderFlags, ShaderCompilerParameters *pParameters)
{
    // blendmode
    switch (pMaterialShader->GetBlendMode())
    {
    case MATERIAL_BLENDING_MODE_NONE:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_NONE", "1");
        break;

    case MATERIAL_BLENDING_MODE_ADDITIVE:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_ADDITIVE", "1");
        break;

    case MATERIAL_BLENDING_MODE_STRAIGHT:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_STRAIGHT", "1");
        break;

    case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_PREMULTIPLIED", "1");
        break;

    case MATERIAL_BLENDING_MODE_MASKED:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_MASKED", "1");
        break;

    case MATERIAL_BLENDING_MODE_SOFTMASKED:
        pParameters->AddPreprocessorMacro("MATERIAL_BLENDING_MODE_SOFTMASKED", "1");
        break;

    default:
        UnreachableCode();
        break;
    }

    // lightingtype
    switch (pMaterialShader->GetLightingType())
    {
    case MATERIAL_LIGHTING_TYPE_EMISSIVE:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_TYPE_EMISSIVE", "1");
        break;

    case MATERIAL_LIGHTING_TYPE_REFLECTIVE:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_TYPE_REFLECTIVE", "1");
        break;

    case MATERIAL_LIGHTING_TYPE_REFLECTIVE_EMISSIVE:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_TYPE_REFLECTIVE_EMISSIVE", "1");
        break;

    case MATERIAL_LIGHTING_TYPE_REFLECTIVE_TWO_SIDED:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_TYPE_REFLECTIVE_TWO_SIDED", "1");
        break;

    default:
        UnreachableCode();
        break;
    }

    // lightingmodel
    switch (pMaterialShader->GetLightingModel())
    {
    case MATERIAL_LIGHTING_MODEL_PHONG:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_MODEL_PHONG", "1");
        break;

    case MATERIAL_LIGHTING_MODEL_BLINN_PHONG:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_MODEL_BLINN_PHONG", "1");
        break;

    case MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED", "1");
        break;
        
    case MATERIAL_LIGHTING_MODEL_CUSTOM:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_MODEL_CUSTOM", "1");
        break;

    default:
        UnreachableCode();
        break;
    }

    // lighting normal space
    switch (pMaterialShader->GetLightingNormalSpace())
    {
    case MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE", "1");
        break;

    case MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE:
        pParameters->AddPreprocessorMacro("MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE", "1");
        break;
    }

    // render mode
    switch (pMaterialShader->GetRenderMode())
    {
    case MATERIAL_RENDER_MODE_NORMAL:
        pParameters->AddPreprocessorMacro("MATERIAL_RENDER_MODE_NORMAL", "1");
        break;

    case MATERIAL_RENDER_MODE_WIREFRAME:
        pParameters->AddPreprocessorMacro("MATERIAL_RENDER_MODE_WIREFRAME", "1");
        break;

    case MATERIAL_RENDER_MODE_POST_PROCESS:
        pParameters->AddPreprocessorMacro("MATERIAL_RENDER_MODE_POST_PROCESS", "1");
        break;
    }

    // two-sided flag
    if (pMaterialShader->IsTwoSided())
        pParameters->AddPreprocessorMacro("MATERIAL_RENDER_TWO_SIDED", "1");

    // static switches -- since these are string allocated, they have to be pushed on the compiler parameters
    for (uint32 i = 0; i < pMaterialShader->GetStaticSwitchParameterCount(); i++)
    {
        const MaterialShader::StaticSwitchParameter *pStaticSwitchParameter = pMaterialShader->GetStaticSwitchParameter(i);
        if (materialShaderFlags & pStaticSwitchParameter->Mask)
        {
            SmallString staticSwitchMacro;
            staticSwitchMacro.Format("MTLStaticSwitch_%s", pStaticSwitchParameter->Name.GetCharArray());
            TinyString staticSwitchValue;
            staticSwitchValue.AppendString("1");
            pParameters->AddPreprocessorMacro(staticSwitchMacro, staticSwitchValue);
        }
    }
}

static void AddCommonDefines(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel,
                             const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags,
                             const MaterialShader *pMaterialShader, uint32 materialShaderFlags,
                             ShaderCompilerParameters *pParameters)
{  
    switch (platform)
    {
    case RENDERER_PLATFORM_D3D11:
        pParameters->AddPreprocessorMacro("PLATFORM_D3D11", "1");
        break;

    case RENDERER_PLATFORM_D3D12:
        pParameters->AddPreprocessorMacro("PLATFORM_D3D12", "1");
        break;

    case RENDERER_PLATFORM_OPENGL:
        pParameters->AddPreprocessorMacro("PLATFORM_OPENGL", "1");
        break;

    case RENDERER_PLATFORM_OPENGLES2:
        pParameters->AddPreprocessorMacro("PLATFORM_OPENGLES2", "1");
        break;
    }

    // platform-specific defines
    switch (featureLevel)
    {
    case RENDERER_FEATURE_LEVEL_ES2:
        pParameters->AddPreprocessorMacro("FEATURE_LEVEL", "0");
        break;

    case RENDERER_FEATURE_LEVEL_ES3:
        pParameters->AddPreprocessorMacro("FEATURE_LEVEL", "1");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_ES3", "1");
        break;

    case RENDERER_FEATURE_LEVEL_SM4:
        pParameters->AddPreprocessorMacro("FEATURE_LEVEL", "2");
        pParameters->AddPreprocessorMacro("SHADER_MODEL_VERSION", "4");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_ES3", "1");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_SM4", "1");
        break;

    case RENDERER_FEATURE_LEVEL_SM5:
        pParameters->AddPreprocessorMacro("FEATURE_LEVEL", "3");
        pParameters->AddPreprocessorMacro("SHADER_MODEL_VERSION", "5");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_ES3", "1");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_SM4", "1");
        pParameters->AddPreprocessorMacro("HAS_FEATURE_LEVEL_SM5", "1");
        break;
    }

    // which stages are present
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_VERTEX_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_VERTEX_SHADER", "1");
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_HULL_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_HULL_SHADER", "1");
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_DOMAIN_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_DOMAIN_SHADER", "1");
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_PIXEL_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_PIXEL_SHADER", "1");
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_GEOMETRY_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_GEOMETRY_SHADER", "1");
    if (!pParameters->StageEntryPoints[SHADER_PROGRAM_STAGE_COMPUTE_SHADER].IsEmpty())
        pParameters->AddPreprocessorMacro("HAS_COMPUTE_SHADER", "1");

    // shader-specific defines
    if (pVertexFactoryTypeInfo != nullptr)
        pParameters->AddPreprocessorMacro("WITH_VERTEX_FACTORY", "1");
    if (pMaterialShader != nullptr)
        pParameters->AddPreprocessorMacro("WITH_MATERIAL",        "1");
}

bool ShaderCompilerFrontend::CompileShader(ResourceCompilerInterface *pCompilerInterface, 
                                           uint32 globalShaderFlags,
                                           RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel,
                                           const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags,
                                           const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags,
                                           const MaterialShader *pMaterialShader, uint32 materialShaderFlags,
                                           bool enableDebugInfo, ByteStream *pOutByteCodeStream, ByteStream *pOutInfoLogStream)
{
    // work out compiler flags
    uint32 compilerFlags = 0;
    if (enableDebugInfo)
        compilerFlags |= SHADER_COMPILER_FLAG_ENABLE_DEBUG_INFO | SHADER_COMPILER_FLAG_DISABLE_OPTIMIZATIONS;

    // create parameter block
    ShaderCompilerParameters compilerParameters;
    compilerParameters.Platform = platform;
    compilerParameters.FeatureLevel = featureLevel;
    compilerParameters.CompilerFlags = compilerFlags;
    compilerParameters.EnableVerboseInfoLog = (pOutInfoLogStream != nullptr);
    compilerParameters.MaterialShaderName = (pMaterialShader != nullptr) ? pMaterialShader->GetName() : EmptyString;
    compilerParameters.MaterialShaderFlags = materialShaderFlags;

    // add global defines
    AddGlobalShaderDefines(globalShaderFlags, &compilerParameters);

    // fill from types
    if (!pBaseShaderTypeInfo->FillShaderCompilerParameters(globalShaderFlags, baseShaderFlags, vertexFactoryFlags, &compilerParameters) ||
        (pVertexFactoryTypeInfo != nullptr && !pVertexFactoryTypeInfo->FillShaderCompilerParameters(globalShaderFlags, baseShaderFlags, vertexFactoryFlags, &compilerParameters)))
    {
        return false;
    }

    // material defines
    if (pMaterialShader != nullptr)
        AddMaterialShaderDefines(pMaterialShader, materialShaderFlags, &compilerParameters);

    // common defines
    AddCommonDefines(platform, featureLevel, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags, &compilerParameters);

    // create info log if requested
    if (pOutInfoLogStream != nullptr)
    {
        TextWriter textWriter(pOutInfoLogStream);
        textWriter.WriteLine("Shader Compiler");
        textWriter.WriteFormattedLine("Platform: %s", NameTable_GetNameString(NameTables::RendererPlatform, platform));
        textWriter.WriteFormattedLine("Feature Level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevel));
        textWriter.WriteFormattedLine("Base Shader: %s %08X", (pBaseShaderTypeInfo != NULL) ? pBaseShaderTypeInfo->GetTypeName() : "NULL", baseShaderFlags);
        textWriter.WriteFormattedLine("Vertex Factory: %s %08X", (pVertexFactoryTypeInfo != NULL) ? pVertexFactoryTypeInfo->GetTypeName() : "NULL", vertexFactoryFlags);
        textWriter.WriteFormattedLine("Material Shader: %s %08X", (pMaterialShader != NULL) ? pMaterialShader->GetName().GetCharArray() : "NULL", materialShaderFlags);
        textWriter.WriteLine("");
    }

    // pass off to resource compiler
    return pCompilerInterface->CompileShader(&compilerParameters, pOutByteCodeStream, pOutInfoLogStream);
}

uint32 ShaderCompilerFrontend::GetShaderStoreHash()
{
    return s_shaderStoreHash;
}

static void CalculateShaderStoreHash()
{
    CRC32 crc;

    FileSystem::FindResultsArray fileList;
    if (g_pVirtualFileSystem->FindFiles("shaders/base", "*.hlsl", FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_RECURSIVE, &fileList))
    {
        for (uint32 i = 0; i < fileList.GetSize(); i++)
        {
            BinaryBlob *pBlob = g_pVirtualFileSystem->GetFileContents(fileList[i].FileName);
            if (pBlob != nullptr)
            {
                crc.Update(pBlob->GetDataPointer(), pBlob->GetDataSize());
                pBlob->Release();
            }
        }
    }

    s_shaderStoreHash = crc.GetCRC();
}

void ShaderCompilerFrontend::InitializeShaderCompilerSupport()
{
    // not threadsafe.. whatever.
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    Log_DevPrintf("ShaderCompilerFrontend::InitializeShaderCompilerSupport()");

    // calculate the crc of all base shader files
    CalculateShaderStoreHash();
    Log_DevPrintf("  Shader store CRC = 0x%08X", s_shaderStoreHash);

    // calculate the parameters crc for each shader type
    uint32 nShaderComponentTypes = 0;
    for (uint32 typeIndex = 0; typeIndex < ShaderComponentTypeInfo::GetRegistry().GetNumTypes(); typeIndex++)
    {
        const ObjectTypeInfo *pObjectTypeInfo = ShaderComponentTypeInfo::GetRegistry().GetTypeInfoByIndex(typeIndex);
        if (pObjectTypeInfo == nullptr || !pObjectTypeInfo->IsDerived(OBJECT_TYPEINFO(ShaderComponent)))
            continue;

        // cast (messy..)
        ShaderComponentTypeInfo *pShaderComponentTypeInfo = const_cast<ShaderComponentTypeInfo *>(static_cast<const ShaderComponentTypeInfo *>(pObjectTypeInfo));
        const SHADER_COMPONENT_PARAMETER_BINDING *pBindings = pShaderComponentTypeInfo->GetParameterBindings();

        // calculate crc
        CRC32 crc;
        for (uint32 i = 0; i < pShaderComponentTypeInfo->GetParameterBindingCount(); i++)
        {
            uint32 typeAsInt = (uint32)pBindings[i].ExpectedType;
            crc.Update(pBindings[i].ParameterName, Y_strlen(pBindings[i].ParameterName));
            crc.Update(&typeAsInt, sizeof(typeAsInt));
        }
        pShaderComponentTypeInfo->SetParameterCRC(crc.GetCRC());
        nShaderComponentTypes++;
    }
    Log_DevPrintf("  %u shader component types registered.", nShaderComponentTypes);
}

bool ShaderCompilerFrontend::RecalculateShaderStoreHash()
{
    uint32 old = s_shaderStoreHash;
    CalculateShaderStoreHash();
    return (old != s_shaderStoreHash);
}
