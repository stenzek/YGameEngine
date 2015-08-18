#pragma once
#include "ResourceCompiler/Common.h"
#include "ResourceCompiler/ResourceCompilerCallbacks.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/ShaderCompilerFrontend.h"

class MaterialShaderGenerator;
class ShaderGraphCompiler;

class ShaderCompiler
{
public:
    ShaderCompiler(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters);
    virtual ~ShaderCompiler();

    // accessors
    const RENDERER_PLATFORM GetRendererPlatform() const { return m_eRendererPlatform; }
    const RENDERER_FEATURE_LEVEL GetRendererFeatureLevel() const { return m_eRendererFeatureLevel; }
    const uint32 GetShaderCompilerFlags() const { return m_iShaderCompilerFlags; }
    
    // intemediary functions
    void AddCompileMacro(const char *macroName, const char *macroValue);

    // compile
    bool Compile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream);

    // add material shader defines to shader graph
    void SetupShaderGraphCompiler(MaterialShaderGenerator *pMaterialShaderSource, ShaderGraphCompiler *pCompiler) const;

    // Helper methods for reading various include files.
    bool ReadIncludeFile(bool systemInclude, const char *filename, void **ppOutFileContents, uint32 *pOutFileLength);
    void FreeIncludeFile(void *pFileContents);

    // Platform entry points
    static ShaderCompiler *CreateD3D11ShaderCompiler(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters);
    static ShaderCompiler *CreateOpenGLShaderCompiler(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters);

protected:
    // actual compile method
    virtual bool InternalCompile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream) = 0;

    // add material shader defines
    void AddMaterialShaderDefines();

    // in parameters
    ResourceCompilerCallbacks *m_pResourceCompilerCallbacks;
    BinaryBlob *m_pMaterialShaderCode;
    RENDERER_PLATFORM m_eRendererPlatform;
    RENDERER_FEATURE_LEVEL m_eRendererFeatureLevel;
    uint32 m_iShaderCompilerFlags;
    uint32 m_iMaterialShaderFlags;
    String m_StageFileNames[SHADER_PROGRAM_STAGE_COUNT];
    String m_StageEntryPoints[SHADER_PROGRAM_STAGE_COUNT];
    String m_VertexFactoryFileName;
    String m_MaterialShaderName;
    Array<ShaderCompilerParameters::PreprocessorMacro> m_CompileMacros;
    TextWriter *m_pDumpWriter;
};
