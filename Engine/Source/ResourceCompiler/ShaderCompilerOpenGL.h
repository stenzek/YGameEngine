#pragma once
#include "ResourceCompiler/ShaderCompiler.h"
#include "OpenGLRenderer/OpenGLShaderCacheEntry.h"
#include "glad/glad.h"
#include "HLSLTranslator.h"
#include "HLSLTranslator_GLSL.h"

class ShaderCompilerOpenGL : public ShaderCompiler
{
public:
    ShaderCompilerOpenGL(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters);
    virtual ~ShaderCompilerOpenGL();

protected:
    // compile
    virtual bool InternalCompile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream);

    // helper functions
    void AddDefinesForStage(SHADER_PROGRAM_STAGE stage, MemArray<HLSLTRANSLATOR_MACRO> &macroArray);
    bool CompileShaderStage(SHADER_PROGRAM_STAGE stage, uint32 outputGLSLVersion, bool outputGLSLES);
    bool ReflectShaderStage(SHADER_PROGRAM_STAGE stage);
    bool WriteOutput(ByteStream *pByteCodeStream);

private:
    HLSLTRANSLATOR_GLSL_OUTPUT *m_pOutputGLSL[SHADER_PROGRAM_STAGE_COUNT];

    MemArray<OpenGLShaderCacheEntryVertexAttribute> m_vertexAttributes;
    MemArray<OpenGLShaderCacheEntryUniformBlock> m_uniformBlocks;
    MemArray<OpenGLShaderCacheEntryParameter> m_parameters;
    MemArray<OpenGLShaderCacheEntryFragmentData> m_fragDatas;

    Array<String> m_vertexAttributeNames;
    Array<String> m_uniformBlockNames;
    Array<String> m_parameterNames;
    Array<String> m_parameterSamplerNames;
    Array<String> m_fragDataNames;
};

// enum GL_SHADER_COMPILE_FLAGS
// {
//     GL_SHADER_COMPILE_FLAG_COMPILE_VERTEX_SHADER                    = (1 << (SHADER_COMPILE_FLAG_LAST_BIT + 0)),
//     GL_SHADER_COMPILE_FLAG_COMPILE_TESSELATION_CONTROL_SHADER       = (1 << (SHADER_COMPILE_FLAG_LAST_BIT + 1)),
//     GL_SHADER_COMPILE_FLAG_COMPILE_TESSELATION_EVALUATION_SHADER    = (1 << (SHADER_COMPILE_FLAG_LAST_BIT + 2)),
//     GL_SHADER_COMPILE_FLAG_COMPILE_GEOMETRY_SHADER                  = (1 << (SHADER_COMPILE_FLAG_LAST_BIT + 3)),
//     GL_SHADER_COMPILE_FLAG_COMPILE_FRAGMENT_SHADER                  = (1 << (SHADER_COMPILE_FLAG_LAST_BIT + 4)),
// };
// 
// const BinaryBlob *GLRenderer_CompileShader(const ShaderCompileParameters *pCompileParameters);
