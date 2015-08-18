#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"

class ResourceCompilerInterface;
class ShaderComponentTypeInfo;
class VertexFactoryTypeInfo;
class MaterialShader;

enum SHADER_COMPILER_FLAGS
{
    SHADER_COMPILER_FLAG_ENABLE_DEBUG_INFO      = (1 << 0),
    SHADER_COMPILER_FLAG_DISABLE_OPTIMIZATIONS  = (1 << 1),
};

struct ShaderCompilerParameters
{
    RENDERER_PLATFORM Platform;
    RENDERER_FEATURE_LEVEL FeatureLevel;
    uint32 CompilerFlags;
    String StageFileNames[SHADER_PROGRAM_STAGE_COUNT];
    String StageEntryPoints[SHADER_PROGRAM_STAGE_COUNT];
    String VertexFactoryFileName;
    String MaterialShaderName;
    uint32 MaterialShaderFlags;
    bool EnableVerboseInfoLog;

    typedef KeyValuePair<String, String> PreprocessorMacro;
    Array<PreprocessorMacro> PreprocessorMacros;

    /*typedef KeyValuePair<String, SHADER_PARAMETER_TYPE> ParameterBinding;
    Array<ParameterBinding> BaseShaderParameters;
    Array<ParameterBinding> VertexFactoryParameters;
    Array<ParameterBinding> MaterialShaderUniformParameters;
    Array<ParameterBinding> MaterialShaderTextureParameters;*/

    void SetStageEntryPoint(SHADER_PROGRAM_STAGE stage, const char *filename, const char *entryPoint) { StageFileNames[stage] = filename; StageEntryPoints[stage] = entryPoint; }
    void SetStageEntryPoint(SHADER_PROGRAM_STAGE stage, const String &filename, const String &entryPoint) { StageFileNames[stage] = filename; StageEntryPoints[stage] = entryPoint; }
    void SetVertexFactoryFileName(const char *filename) { VertexFactoryFileName = filename; }
    void SetVertexFactoryFileName(const String &filename) { VertexFactoryFileName = filename; }
    void AddPreprocessorMacro(const char *define, const char *value) { PreprocessorMacros.Add(PreprocessorMacro(define, value)); }
    void AddPreprocessorMacro(const String &define, const String &value) { PreprocessorMacros.Add(PreprocessorMacro(define, value)); }
};

class ShaderCompilerFrontend
{
public:
    
    // hash code generator
    static void GenerateShaderHashCode(uint8 HashCode[16], 
                                       uint32 globalShaderFlags,
                                       const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, 
                                       const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, 
                                       const MaterialShader *pMaterialShader, uint32 materialShaderFlags);

    // shader compiler
    static bool CompileShader(ResourceCompilerInterface *pCompilerInterface,
                              uint32 globalShaderFlags, 
                              RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel,
                              const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags,
                              const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags,
                              const MaterialShader *pMaterialShader, uint32 materialShaderFlags,
                              bool enableDebugInfo, ByteStream *pOutByteCodeStream, ByteStream *pOutInfoLogStream);

    // current shader store hash
    static uint32 GetShaderStoreHash();

    // initializing compile support
    static void InitializeShaderCompilerSupport();

    // compile shaders for a material -- move to resource compiler
    //static bool CompileShadersForMaterial(const MaterialShader *pMaterialShader, uint32 *pNumShadersCompiled);

    // support for reloading base shaders, returns true if it changed
    static bool RecalculateShaderStoreHash();
};

