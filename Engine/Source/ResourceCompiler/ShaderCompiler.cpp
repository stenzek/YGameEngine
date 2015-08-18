#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderCompiler.h"
#include "ResourceCompiler/ShaderGraphCompiler.h"
#include "ResourceCompiler/ShaderGraphCompilerHLSL.h"
#include "ResourceCompiler/MaterialShaderGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/MaterialShader.h"
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Renderer/VertexFactoryTypeInfo.h"
Log_SetChannel(ShaderCompiler);

static const char SHADER_SOURCE_BASE_DIRECTORY[] = "shaders/base";
static const char MATERIAL_SHADER_TEMPLATE_HEADER_FILE[] = "shaders/base/MaterialTemplateHeader.hlsl";
static const char MATERIAL_SHADER_TEMPLATE_FOOTER_FILE[] = "shaders/base/MaterialTemplateFooter.hlsl";

ShaderCompiler::ShaderCompiler(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters)
    : m_pResourceCompilerCallbacks(pCallbacks),
      m_pMaterialShaderCode(nullptr),
      m_eRendererPlatform(pParameters->Platform),
      m_eRendererFeatureLevel(pParameters->FeatureLevel),
      m_iShaderCompilerFlags(pParameters->CompilerFlags),
      m_iMaterialShaderFlags(pParameters->MaterialShaderFlags),
      m_pDumpWriter(NULL)
{
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        m_StageFileNames[i] = pParameters->StageFileNames[i];
        m_StageEntryPoints[i] = pParameters->StageEntryPoints[i];
    }
    m_VertexFactoryFileName = pParameters->VertexFactoryFileName;
    m_MaterialShaderName = pParameters->MaterialShaderName;
    m_CompileMacros.Reserve(pParameters->PreprocessorMacros.GetSize());
    for (uint32 i = 0; i < pParameters->PreprocessorMacros.GetSize(); i++)
        m_CompileMacros.Add(pParameters->PreprocessorMacros[i]);
}

ShaderCompiler::~ShaderCompiler()
{
    if (m_pMaterialShaderCode != nullptr)
        m_pMaterialShaderCode->Release();
}

void ShaderCompiler::AddCompileMacro(const char *macroName, const char *macroValue)
{
    m_CompileMacros.Add(ShaderCompilerParameters::PreprocessorMacro(macroName, macroValue));
}

void ShaderCompiler::SetupShaderGraphCompiler(MaterialShaderGenerator *pMaterialShaderSource, ShaderGraphCompiler *pCompiler) const
{
    uint32 i;
    SmallString bindingName;
    DebugAssert(pMaterialShaderSource != NULL);

    // uniforms
    for (i = 0; i < pMaterialShaderSource->GetUniformParameterCount(); i++)
    {
        const MaterialShaderGenerator::UniformParameter *pUniformParameter = pMaterialShaderSource->GetUniformParameter(i);
        bindingName.Format("MTLUniformParameter_%s", pUniformParameter->Name.GetCharArray());
        pCompiler->AddExternalUniformParameter(pUniformParameter->Name, pUniformParameter->Type, bindingName);
    }

    // textures
    for (i = 0; i < pMaterialShaderSource->GetTextureParameterCount(); i++)
    {
        const MaterialShaderGenerator::TextureParameter *pTextureParameter = pMaterialShaderSource->GetTextureParameter(i);
        bindingName.Format("MTLTextureParameter_%s", pTextureParameter->Name.GetCharArray());
        pCompiler->AddExternalTextureParameter(pTextureParameter->Name, pTextureParameter->Type, bindingName);
    }

    // static switches -- since these are string allocated, they have to be pushed on the compiler parameters
    for (i = 0; i < pMaterialShaderSource->GetStaticSwitchParameterCount(); i++)
    {
        const MaterialShaderGenerator::StaticSwitchParameter *pStaticSwitchParameter = pMaterialShaderSource->GetStaticSwitchParameter(i);
        pCompiler->AddExternalStaticSwitchParameter(pStaticSwitchParameter->Name, (m_iMaterialShaderFlags & (1 << i)) != 0);
    }
}

bool ShaderCompiler::ReadIncludeFile(bool systemInclude, const char *filename, void **ppOutFileContents, uint32 *pOutFileLength)
{
    AutoReleasePtr<GrowableMemoryByteStream> pOutStream = ByteStream_CreateGrowableMemoryStream();
    BinaryBlob *pCodeBlob = nullptr;
    
    // Generated filenames
    if (Y_stricmp(filename, "VertexFactory.hlsl") == 0)
        pCodeBlob = (!m_VertexFactoryFileName.IsEmpty()) ? m_pResourceCompilerCallbacks->GetFileContents(m_VertexFactoryFileName) : nullptr;
    else if (Y_stricmp(filename, "Material.hlsl") == 0)
        pCodeBlob = (m_pMaterialShaderCode != nullptr) ? BinaryBlob::CreateFromPointer(m_pMaterialShaderCode->GetDataPointer(), m_pMaterialShaderCode->GetDataSize()) : nullptr;
    else
    {
        SmallString diskFilename;
        diskFilename.Format("%s/%s", SHADER_SOURCE_BASE_DIRECTORY, filename);
        pCodeBlob = m_pResourceCompilerCallbacks->GetFileContents(diskFilename);
    }

    if (pCodeBlob == nullptr)
    {
        Log_WarningPrintf("ShaderCompiler::ReadIncludeFile: Failed to include file '%s'", filename);
        return false;
    }

    pCodeBlob->DetachMemory(ppOutFileContents, pOutFileLength);
    pCodeBlob->Release();
    return true;
}

void ShaderCompiler::FreeIncludeFile(void *pFileContents)
{
    Y_free(pFileContents);
}

bool ShaderCompiler::Compile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream)
{
    SmallString fileName;

    // if a material shader was specified, load it's source, and compile the shader graph (if there is one)
    if (!m_MaterialShaderName.IsEmpty())
    {
        // load source
        fileName.Format("%s.msh.xml", m_MaterialShaderName.GetCharArray());
        BinaryBlob *pBlob = m_pResourceCompilerCallbacks->GetFileContents(fileName);
        if (pBlob == nullptr)
        {
            Log_ErrorPrintf("ShaderCompiler::Compile: Failed to find material shader source. (%s)", fileName.GetCharArray());
            return false;
        }

        // create generator
        ByteStream *pMaterialSourceStream = pBlob->CreateReadOnlyStream();
        MaterialShaderGenerator *pMaterialSource = new MaterialShaderGenerator();
        if (!pMaterialSource->LoadFromXML(m_pResourceCompilerCallbacks, fileName, pMaterialSourceStream))
        {
            Log_ErrorPrintf("ShaderCompiler::Compile: Failed to load material shader source. (%s)", fileName.GetCharArray());
            return false;
        }

        // release source now
        pMaterialSourceStream->Release();
        pBlob->Release();

        // create the temporary output stream
        ByteStream *pCodeStream = ByteStream_CreateGrowableMemoryStream();
        
        // append the material shader header
        pBlob = m_pResourceCompilerCallbacks->GetFileContents(MATERIAL_SHADER_TEMPLATE_HEADER_FILE);
        if (pBlob == nullptr || !pCodeStream->Write2(pBlob->GetDataPointer(), pBlob->GetDataSize()))
        {
            Log_ErrorPrintf("ShaderCompiler::Compile: Failed to read material shader common header.");
            if (pBlob != nullptr)
                pBlob->Release();

            pCodeStream->Release();
            delete pMaterialSource;
            return false;
        }

        
        // prefer code over graphs
        int32 currentFeatureLevel = (int32)m_eRendererFeatureLevel;
        for (; currentFeatureLevel >= 0; currentFeatureLevel--)
        {
            if (pMaterialSource->GetShaderCodeAvailability((RENDERER_FEATURE_LEVEL)currentFeatureLevel))
            {
                const String *pCode = pMaterialSource->GetShaderCode((RENDERER_FEATURE_LEVEL)currentFeatureLevel);
                pCodeStream->Write2(pCode->GetCharArray(), pCode->GetLength());
                break;
            }
            else if (pMaterialSource->GetShaderGraphAvailability((RENDERER_FEATURE_LEVEL)currentFeatureLevel))
            {
                const ShaderGraph *pGraph = pMaterialSource->GetShaderGraph((RENDERER_FEATURE_LEVEL)currentFeatureLevel);

                ShaderGraphCompilerHLSL *pGraphCompiler = new ShaderGraphCompilerHLSL(pGraph);
                SetupShaderGraphCompiler(pMaterialSource, pGraphCompiler);

                if (!pGraphCompiler->Compile(pCodeStream))
                {
                    Log_ErrorPrintf("ShaderCompiler::Compile: Failed to compile shader graph at feature level %s for material shader %s.", 
                        NameTable_GetNameString(NameTables::RendererFeatureLevel, currentFeatureLevel), m_MaterialShaderName.GetCharArray());

                    delete pGraphCompiler;
                    pCodeStream->Release();
                    delete pMaterialSource;
                    return false;
                }

                delete pGraphCompiler;
                break;
            }
        }

        // material can be unloaded now
        delete pMaterialSource;

        // if we didn't get any code, that is fatal
        if (currentFeatureLevel < 0)
        {
            Log_ErrorPrintf("ShaderCompiler::Compile: Failed to get shader code for material shader %s.", m_MaterialShaderName.GetCharArray());
            pCodeStream->Release();
            return false;
        } 

        // set material feature level
        switch (currentFeatureLevel)
        {
        case RENDERER_FEATURE_LEVEL_ES2:    m_CompileMacros.Add(ShaderCompilerParameters::PreprocessorMacro(StaticString("MATERIAL_FEATURE_LEVEL"), StaticString("0")));    break;
        case RENDERER_FEATURE_LEVEL_ES3:    m_CompileMacros.Add(ShaderCompilerParameters::PreprocessorMacro(StaticString("MATERIAL_FEATURE_LEVEL"), StaticString("1")));    break;
        case RENDERER_FEATURE_LEVEL_SM4:    m_CompileMacros.Add(ShaderCompilerParameters::PreprocessorMacro(StaticString("MATERIAL_FEATURE_LEVEL"), StaticString("2")));    break;
        case RENDERER_FEATURE_LEVEL_SM5:    m_CompileMacros.Add(ShaderCompilerParameters::PreprocessorMacro(StaticString("MATERIAL_FEATURE_LEVEL"), StaticString("3")));    break;
        }

        // append the material shader footer
        pBlob = m_pResourceCompilerCallbacks->GetFileContents(MATERIAL_SHADER_TEMPLATE_FOOTER_FILE);
        if (pBlob == nullptr || !pCodeStream->Write2(pBlob->GetDataPointer(), pBlob->GetDataSize()))
        {
            Log_ErrorPrintf("ShaderCompiler::Compile: Failed to read material shader common footer.");
            if (pBlob != nullptr)
                pBlob->Release();

            pCodeStream->Release();
            return false;
        }

        // turn the stream into a blob
        m_pMaterialShaderCode = BinaryBlob::CreateFromStream(pCodeStream);
        pCodeStream->Release();
    }
    
    // add writer
    if (pInfoLogStream != NULL)
        m_pDumpWriter = new TextWriter(pInfoLogStream);

    // invoke compile
    bool returnValue = InternalCompile(pByteCodeStream, pInfoLogStream);

    // close writer
    delete m_pDumpWriter;
    m_pDumpWriter = NULL;

    // clean up
    return returnValue;
}

bool ResourceCompiler::CompileShader(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters, ByteStream *pByteCodeStream, ByteStream *pInfoLogStream)
{
    ShaderCompiler *pShaderCompiler;
    switch (pParameters->Platform)
    {
#ifdef Y_PLATFORM_WINDOWS
    case RENDERER_PLATFORM_D3D11:
        pShaderCompiler = ShaderCompiler::CreateD3D11ShaderCompiler(pCallbacks, pParameters);
        break;
#endif

    case RENDERER_PLATFORM_OPENGL:
    case RENDERER_PLATFORM_OPENGLES2:
        pShaderCompiler = ShaderCompiler::CreateOpenGLShaderCompiler(pCallbacks, pParameters);
        break;

    default:
        Log_ErrorPrintf("ResourceCompiler::CompileShader: Unknown shader platform: %s", NameTable_GetNameString(NameTables::RendererPlatform, pParameters->Platform));
        return false;
    }

    if (!pShaderCompiler->Compile(pByteCodeStream, pInfoLogStream))
    {
        delete pShaderCompiler;
        return false;
    }

    delete pShaderCompiler;
    return true;
}
