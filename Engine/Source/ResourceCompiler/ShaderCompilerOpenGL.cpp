#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderCompilerOpenGL.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "OpenGLRenderer/OpenGLShaderCacheEntry.h"
#include "OpenGLRenderer/OpenGLDefines.h"
Log_SetChannel(ShaderCompilerOpenGL);

static const HLSLTRANSLATOR_SHADER_STAGE s_shaderStageToTranslatorStage[SHADER_PROGRAM_STAGE_COUNT] =
{
    HLSLTRANSLATOR_SHADER_STAGE_VERTEX,            // SHADER_PROGRAM_STAGE_VERTEX_SHADER
    NUM_HLSLTRANSLATOR_SHADER_STAGES,             // SHADER_PROGRAM_STAGE_HULL_SHADER
    NUM_HLSLTRANSLATOR_SHADER_STAGES,             // SHADER_PROGRAM_STAGE_DOMAIN_SHADER
    HLSLTRANSLATOR_SHADER_STAGE_GEOMETRY,          // SHADER_PROGRAM_STAGE_GEOMETRY_SHADER
    HLSLTRANSLATOR_SHADER_STAGE_PIXEL,             // SHADER_PROGRAM_STAGE_PIXEL_SHADER
    NUM_HLSLTRANSLATOR_SHADER_STAGES,             // SHADER_PROGRAM_STAGE_COMPUTE_SHADER
};

ShaderCompilerOpenGL::ShaderCompilerOpenGL(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters)
    : ShaderCompiler(pCallbacks, pParameters)
{
    Y_memzero(m_pOutputGLSL, sizeof(m_pOutputGLSL));
}

ShaderCompilerOpenGL::~ShaderCompilerOpenGL()
{
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_pOutputGLSL[i] != NULL)
            HLSLTranslator_FreeMemory(m_pOutputGLSL[i]);
    }
}

void ShaderCompilerOpenGL::AddDefinesForStage(SHADER_PROGRAM_STAGE stage, MemArray<HLSLTRANSLATOR_MACRO> &macroArray)
{
    // stage-specific defines
    switch (stage)
    {
    case SHADER_PROGRAM_STAGE_VERTEX_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("VERTEX_SHADER",                "1"));
        break;

    case SHADER_PROGRAM_STAGE_HULL_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("HULL_SHADER",                  "1"));
        break;

    case SHADER_PROGRAM_STAGE_DOMAIN_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("DOMAIN_SHADER",                "1"));
        break;

    case SHADER_PROGRAM_STAGE_GEOMETRY_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("GEOMETRY_SHADER",              "1"));
        break;

    case SHADER_PROGRAM_STAGE_PIXEL_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("PIXEL_SHADER",                 "1"));
        break;

    case SHADER_PROGRAM_STAGE_COMPUTE_SHADER:
        macroArray.Add(HLSLTRANSLATOR_MACRO("COMPUTE_SHADER",               "1"));
        break;
    }

    // add defines to preprocessor
    for (uint32 i = 0; i < m_CompileMacros.GetSize(); i++)
        macroArray.Add(HLSLTRANSLATOR_MACRO(m_CompileMacros[i].Key, m_CompileMacros[i].Value));
}

static bool IncludeOpenCallback(void *memoryContext, const char *includeFileName, bool systemInclude, char **outBuffer, unsigned int *outLength, void *context)
{
    ShaderCompilerOpenGL *pCompiler = reinterpret_cast<ShaderCompilerOpenGL *>(context);
    DebugAssert(pCompiler != nullptr);

    void *pCodePointer;
    uint32 codeSize;
    if (!pCompiler->ReadIncludeFile(systemInclude, includeFileName, &pCodePointer, &codeSize))
        return false;

    *outBuffer = reinterpret_cast<char *>(pCodePointer);
    *outLength = (size_t)codeSize;
    return true;
}

static void IncludeCloseCallback(void *memoryContext, char *buffer, void *context)
{
    ShaderCompilerOpenGL *pCompiler = reinterpret_cast<ShaderCompilerOpenGL *>(context);
    DebugAssert(pCompiler != nullptr);

    pCompiler->FreeIncludeFile(reinterpret_cast<void *>(buffer));
}

bool ShaderCompilerOpenGL::CompileShaderStage(SHADER_PROGRAM_STAGE stage, uint32 outputGLSLVersion, bool outputGLSLES)
{   
    Timer timer;

    // get source filename
    DebugAssert(s_shaderStageToTranslatorStage[stage] != NUM_HLSLTRANSLATOR_SHADER_STAGES);
    if (m_StageFileNames[stage].IsEmpty() || m_StageEntryPoints[stage].IsEmpty())
        return false;

    // open the source file
    AutoReleasePtr<BinaryBlob> pSourceBlob = m_pResourceCompilerCallbacks->GetFileContents(m_StageFileNames[stage]);
    if (pSourceBlob == nullptr)
    {
        Log_ErrorPrintf("ShaderCompilerD3D11::CompileShaderStage: Could not open shader source file '%s'.", m_StageFileNames[stage].GetCharArray());
        return false;
    }

    // get define list
    MemArray<HLSLTRANSLATOR_MACRO> macroArray;
    AddDefinesForStage(stage, macroArray);

    // preprocess the shader
    if (m_pDumpWriter != nullptr)
    {
        HLSLTRANSLATOR_OUTPUT *pPreprocessorOutput;
        if (!HLSLTranslator_PreprocessHLSL(m_StageFileNames[stage], reinterpret_cast<const char *>(pSourceBlob->GetDataPointer()), pSourceBlob->GetDataSize(), 
                                           IncludeOpenCallback, IncludeCloseCallback, this, macroArray.GetBasePointer(), macroArray.GetSize(), &pPreprocessorOutput))
        {
            Log_ErrorPrintf("Failed to preprocess source file \"%s\" for stage %s.", m_StageFileNames[stage].GetCharArray(), NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
            m_pDumpWriter->WriteFormattedLine("Failed to preprocess source file \"%s\" for stage %s.", m_StageFileNames[stage].GetCharArray(), NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
            if (pPreprocessorOutput->InfoLogLength > 0)
            {
                Log_ErrorPrint(pPreprocessorOutput->InfoLog);
                m_pDumpWriter->Write(pPreprocessorOutput->InfoLog, pPreprocessorOutput->InfoLogLength);
            }

            HLSLTranslator_FreeMemory(pPreprocessorOutput);
            return false;
        }

        m_pDumpWriter->WriteFormattedLine("Preprocessed HLSL for %s:", NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
        m_pDumpWriter->WriteLine("=====================================================================");
        m_pDumpWriter->WriteWithLineNumbers(pPreprocessorOutput->OutputSource, pPreprocessorOutput->OutputSourceLength);
        m_pDumpWriter->WriteLine("=====================================================================");
        m_pDumpWriter->WriteLine("");

        HLSLTranslator_FreeMemory(pPreprocessorOutput);
    }

    // generate compile flags
    uint32 compileFlags = HLSLTRANSLATOR_COMPILE_FLAG_GENERATE_REFLECTION;
    if (!(m_iShaderCompilerFlags & SHADER_COMPILER_FLAG_DISABLE_OPTIMIZATIONS))
        compileFlags |= HLSLTRANSLATOR_COMPILE_FLAG_OPTIMIZATION_LEVEL_3 | HLSLTRANSLATOR_COMPILE_FLAG_OPTIMIZATION_LEVEL_2 | HLSLTRANSLATOR_COMPILE_FLAG_OPTIMIZATION_LEVEL_1;

    // translate code
    HLSLTRANSLATOR_GLSL_OUTPUT *pTranslatedGLSL;
    if (!HLSLTranslator_ConvertHLSLToGLSL(m_StageFileNames[stage], reinterpret_cast<const char *>(pSourceBlob->GetDataPointer()), pSourceBlob->GetDataSize(), 
                                          IncludeOpenCallback, IncludeCloseCallback, this, m_StageEntryPoints[stage], s_shaderStageToTranslatorStage[stage],
                                          compileFlags, outputGLSLVersion, outputGLSLES, macroArray.GetBasePointer(), macroArray.GetSize(), &pTranslatedGLSL))
    { 
        Log_ErrorPrintf("Failed to translate source file \"%s\" for stage %s.", m_StageFileNames[stage].GetCharArray(), NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
        if (m_pDumpWriter != nullptr)
            m_pDumpWriter->WriteFormattedLine("Failed to translate source file \"%s\" for stage %s.", m_StageFileNames[stage].GetCharArray(), NameTable_GetNameString(NameTables::ShaderProgramStage, stage));

        if (pTranslatedGLSL->InfoLogLength > 0)
        {
            Log_ErrorPrint(pTranslatedGLSL->InfoLog);
            
            if (m_pDumpWriter != nullptr)
                m_pDumpWriter->Write(pTranslatedGLSL->InfoLog, pTranslatedGLSL->InfoLogLength);
        }
        
        HLSLTranslator_FreeMemory(pTranslatedGLSL);
        return false;
    }
    Log_DevPrintf("    Translating HLSL->GLSL[%s] took %.3f msec", NameTable_GetNameString(NameTables::ShaderProgramStage, stage), timer.GetTimeMilliseconds());
    timer.Reset();

    // write translated code
    if (m_pDumpWriter != nullptr)
    {
        m_pDumpWriter->WriteFormattedLine("Translated GLSL for %s:", NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
        m_pDumpWriter->WriteLine("=====================================================================");
        m_pDumpWriter->WriteWithLineNumbers(pTranslatedGLSL->OutputSource, pTranslatedGLSL->OutputSourceLength);
        m_pDumpWriter->WriteLine("=====================================================================");
        m_pDumpWriter->WriteLine("");

        m_pDumpWriter->WriteFormattedLine("Reflection for %s:", NameTable_GetNameString(NameTables::ShaderProgramStage, stage));
        m_pDumpWriter->WriteLine("=====================================================================");
        {
            const HLSLTRANSLATOR_GLSL_SHADER_REFLECTION *pReflection = pTranslatedGLSL->Reflection;

            m_pDumpWriter->WriteFormattedLine("Inputs (%u): ", pReflection->InputCount);
            for (uint32 i = 0; i < pReflection->InputCount; i++)
            {
                const HLSLTRANSLATOR_GLSL_SHADER_INPUT *pInput = &pReflection->Inputs[i];
                m_pDumpWriter->WriteFormattedLine("  %u: name:'%s', semanticname:'%s', semanticindex:%u, type:0x%X, location:%d", i, pInput->VariableName, (pInput->SemanticName != nullptr) ? pInput->SemanticName : "", pInput->SemanticIndex, pInput->Type, pInput->ExplicitLocation);
            }
            m_pDumpWriter->WriteLine("");

            m_pDumpWriter->WriteFormattedLine("Outputs (%u): ", pReflection->OutputCount);
            for (uint32 i = 0; i < pReflection->OutputCount; i++)
            {
                const HLSLTRANSLATOR_GLSL_SHADER_OUTPUT *pOutput = &pReflection->Outputs[i];
                m_pDumpWriter->WriteFormattedLine("  %u: name:'%s', semanticname:'%s', semanticindex:%u, type:0x%X, location:%d", i, pOutput->VariableName, (pOutput->SemanticName != nullptr) ? pOutput->SemanticName : "", pOutput->SemanticIndex, pOutput->Type, pOutput->ExplicitLocation);
            }
            m_pDumpWriter->WriteLine("");

            m_pDumpWriter->WriteFormattedLine("Uniform Blocks (%u): ", pReflection->UniformBlockCount);
            for (uint32 i = 0; i < pReflection->UniformBlockCount; i++)
            {
                const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM_BLOCK *pBlock = &pReflection->UniformBlocks[i];
                m_pDumpWriter->WriteFormattedLine("  %u: blockname:'%s', instancename:'%s', size:%u, location:%d, fields: %u:", i, pBlock->BlockName, (pBlock->InstanceName != nullptr) ? pBlock->InstanceName : "", pBlock->TotalSize, pBlock->ExplicitLocation, pBlock->FieldCount);
                for (uint32 j = 0; j < pBlock->FieldCount; j++)
                {
                    const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM_BLOCK_FIELD *pField = &pBlock->Fields[j];
                    m_pDumpWriter->WriteFormattedLine("    %u: name:'%s', type:0x%X, arraysize:%d, offset:%u, size:%u, stride:%u, rowmajor:%u", j, pField->VariableName, pField->Type, pField->ArraySize, pField->OffsetInBuffer, pField->SizeInBytes, pField->ArrayStride, (uint32)pField->RowMajor);
                }
            }
            m_pDumpWriter->WriteLine("");

            m_pDumpWriter->WriteFormattedLine("Uniforms (%u): ", pReflection->UniformCount);
            for (uint32 i = 0; i < pReflection->UniformCount; i++)
            {
                const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM *pField = &pReflection->Uniforms[i];
                m_pDumpWriter->WriteFormattedLine("  %u: name:'%s', samplername:'%s', type:0x%X, arraysize:%d, location:%d", i, pField->VariableName, (pField->SamplerVariableName != nullptr) ? pField->SamplerVariableName : "", pField->Type, pField->ArraySize, pField->ExplicitLocation);
            }
            m_pDumpWriter->WriteLine("");
        }
        m_pDumpWriter->WriteLine("=====================================================================");
    }

    m_pOutputGLSL[stage] = pTranslatedGLSL;
    return true;
}

static bool MapOpenGLUniformTypeToShaderParameterType(GLenum glType, SHADER_PARAMETER_TYPE *pParameterType, OPENGL_SHADER_BIND_TARGET *pBindTarget)
{
    struct Mapping
    {
        GLenum GLType;
        SHADER_PARAMETER_TYPE ShaderParameterType;
        OPENGL_SHADER_BIND_TARGET BindTarget;
    };

    static const Mapping mapping[] =
    {
        // GLType                           ShaderParameterType
        { GL_FLOAT,                         SHADER_PARAMETER_TYPE_FLOAT,            OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_VEC2,                    SHADER_PARAMETER_TYPE_FLOAT2,           OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_VEC3,                    SHADER_PARAMETER_TYPE_FLOAT3,           OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_VEC4,                    SHADER_PARAMETER_TYPE_FLOAT4,           OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_INT,                           SHADER_PARAMETER_TYPE_INT,              OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_INT_VEC2,                      SHADER_PARAMETER_TYPE_INT2,             OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_INT_VEC3,                      SHADER_PARAMETER_TYPE_INT3,             OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_INT_VEC4,                      SHADER_PARAMETER_TYPE_INT4,             OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_UNSIGNED_INT,                  SHADER_PARAMETER_TYPE_UINT,             OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_UNSIGNED_INT_VEC2,             SHADER_PARAMETER_TYPE_UINT2,            OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_UNSIGNED_INT_VEC3,             SHADER_PARAMETER_TYPE_UINT3,            OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_UNSIGNED_INT_VEC4,             SHADER_PARAMETER_TYPE_UINT4,            OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_BOOL,                          SHADER_PARAMETER_TYPE_BOOL,             OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_MAT2,                    SHADER_PARAMETER_TYPE_FLOAT2X2,         OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_MAT3,                    SHADER_PARAMETER_TYPE_FLOAT3X3,         OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_FLOAT_MAT4x3,                  SHADER_PARAMETER_TYPE_FLOAT3X4,         OPENGL_SHADER_BIND_TARGET_UNIFORM       },      // <-- because matrices are transposed as they go into gl, float3x4 becomes float4x3
        { GL_FLOAT_MAT4,                    SHADER_PARAMETER_TYPE_FLOAT4X4,         OPENGL_SHADER_BIND_TARGET_UNIFORM       },
        { GL_SAMPLER_1D,                    SHADER_PARAMETER_TYPE_TEXTURE1D,        OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_1D_ARRAY,              SHADER_PARAMETER_TYPE_TEXTURE1DARRAY,   OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D,                    SHADER_PARAMETER_TYPE_TEXTURE2D,        OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D_ARRAY,              SHADER_PARAMETER_TYPE_TEXTURE2DARRAY,   OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D_MULTISAMPLE,        SHADER_PARAMETER_TYPE_TEXTURE2DMS,      OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D_MULTISAMPLE_ARRAY,  SHADER_PARAMETER_TYPE_TEXTURE2DMSARRAY, OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_3D,                    SHADER_PARAMETER_TYPE_TEXTURE3D,        OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_CUBE,                  SHADER_PARAMETER_TYPE_TEXTURECUBE,      OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_CUBE_MAP_ARRAY,        SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY, OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_BUFFER,                SHADER_PARAMETER_TYPE_TEXTUREBUFFER,    OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_1D_SHADOW,             SHADER_PARAMETER_TYPE_TEXTURE1D,        OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_1D_ARRAY_SHADOW,       SHADER_PARAMETER_TYPE_TEXTURE1DARRAY,   OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D_SHADOW,             SHADER_PARAMETER_TYPE_TEXTURE2D,        OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_2D_ARRAY_SHADOW,       SHADER_PARAMETER_TYPE_TEXTURE2DARRAY,   OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_CUBE_SHADOW,           SHADER_PARAMETER_TYPE_TEXTURECUBE,      OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
        { GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW, SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY, OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT  },
    };

    for (uint32 i = 0; i < countof(mapping); i++)
    {
        if (mapping[i].GLType == glType)
        {
            *pParameterType = mapping[i].ShaderParameterType;
            *pBindTarget = mapping[i].BindTarget;
            return true;
        }
    }

    return false;
}

bool ShaderCompilerOpenGL::ReflectShaderStage(SHADER_PROGRAM_STAGE stage)
{
    const HLSLTRANSLATOR_GLSL_OUTPUT *pOutput = m_pOutputGLSL[stage];
    const HLSLTRANSLATOR_GLSL_SHADER_REFLECTION *pReflection = pOutput->Reflection;
    DebugAssert(pReflection != nullptr);

    if (stage == SHADER_PROGRAM_STAGE_VERTEX_SHADER)
    {
        // pull in vertex attributes and map the glsl names to attribute locations
        for (uint32 inInputIndex = 0; inInputIndex < pReflection->InputCount; inInputIndex++)
        {
            const HLSLTRANSLATOR_GLSL_SHADER_INPUT *programAttribute = &pReflection->Inputs[inInputIndex];

            // parse the name. it should start with in_
            const char *currentNamePtr = programAttribute->VariableName;
            if (Y_strnicmp(programAttribute->VariableName, "in_", 3) == 0)
                currentNamePtr += 3;
            else if (Y_strnicmp(programAttribute->VariableName, "attribute_", 10) == 0)
                currentNamePtr += 10;
            else
            {
                Log_ErrorPrintf("OpenGLShaderCompiler::ReflectShaderStage: Invalid attribute variable name: %s", programAttribute->VariableName);
                return false;
            }

            // figure out which semantic it maps to
            uint32 semantic;
            for (semantic = 0; semantic < GPU_VERTEX_ELEMENT_SEMANTIC_COUNT; semantic++)
            {
                const char *semanticNameString = NameTable_GetNameString(NameTables::GPUVertexElementSemantic, semantic);
                uint32 semanticNameLength = Y_strlen(semanticNameString);
                if (Y_strncmp(currentNamePtr, semanticNameString, semanticNameLength) == 0)
                {
                    // match, so strip the name out
                    currentNamePtr += semanticNameLength;
                    break;
                }
            }

            // found match?
            if (semantic == GPU_VERTEX_ELEMENT_SEMANTIC_COUNT)
            {
                Log_ErrorPrintf("OpenGLShaderCompiler::ReflectShaderStage: Invalid attribute semantic: %s", currentNamePtr);
                return false;
            }

            // find the semantic index, if it is the end-of-string, assume zero
            uint32 semanticIndex = 0;
            if (*currentNamePtr != '\0')
                semanticIndex = StringConverter::StringToUInt32(currentNamePtr);

            // temp
            Log_DevPrintf("attrib '%s' -> semantic %s index %u", programAttribute->VariableName, NameTable_GetNameString(NameTables::GPUVertexElementSemantic, semantic), semanticIndex);

            // add an entry
            OpenGLShaderCacheEntryVertexAttribute outAttrib;
            outAttrib.NameLength = Y_strlen(programAttribute->VariableName);
            outAttrib.SemanticName = semantic;
            outAttrib.SemanticIndex = semanticIndex;
            m_vertexAttributes.Add(outAttrib);
            m_vertexAttributeNames.Add(programAttribute->VariableName);
        }
    }
    
    // map uniform buffers on GL3+
    if (m_eRendererFeatureLevel > RENDERER_FEATURE_LEVEL_ES2)
    {
        for (uint32 inUniformBufferIndex = 0; inUniformBufferIndex < pReflection->UniformBlockCount; inUniformBufferIndex++)
        {
            const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM_BLOCK *programUniformBlock = &pReflection->UniformBlocks[inUniformBufferIndex];

            // TODO: Check uniform block signatures match
            uint32 constantBufferIndex;
            for (constantBufferIndex = 0; constantBufferIndex < m_uniformBlocks.GetSize(); constantBufferIndex++)
            {
                if (m_uniformBlockNames[constantBufferIndex].Compare(programUniformBlock->BlockName))
                    break;
            }
            if (constantBufferIndex != m_uniformBlocks.GetSize())
                continue;

            // Determine if the buffer should be allocated locally
            bool isLocal = (Y_strncmp(programUniformBlock->BlockName, "__LocalConstantBuffer_", 22) == 0);
            uint32 parameterIndex = m_parameters.GetSize();

            // Allocate a parameter for this buffer
            OpenGLShaderCacheEntryParameter outParameter;
            outParameter.NameLength = Y_strlen(programUniformBlock->BlockName);
            outParameter.SamplerNameLength = 0;
            outParameter.Type = SHADER_PARAMETER_TYPE_CONSTANT_BUFFER;
            outParameter.UniformBlockIndex = -1;
            outParameter.UniformBlockOffset = 0;
            outParameter.ArraySize = 0;
            outParameter.ArrayStride = 0;
            outParameter.BindTarget = OPENGL_SHADER_BIND_TARGET_UNIFORM_BUFFER;
            m_parameters.Add(outParameter);
            m_parameterNames.Add(programUniformBlock->BlockName);
            m_parameterSamplerNames.Add(EmptyString);

            // Allocate a constant buffer entry
            OpenGLShaderCacheEntryUniformBlock outUniformBuffer;
            outUniformBuffer.NameLength = Y_strlen(programUniformBlock->BlockName);
            outUniformBuffer.Size = programUniformBlock->TotalSize;
            outUniformBuffer.ParameterIndex = parameterIndex;
            outUniformBuffer.IsLocal = isLocal;
            m_uniformBlocks.Add(outUniformBuffer);
            m_uniformBlockNames.Add(programUniformBlock->BlockName);

            // If it is a local buffer, allocate parameters for each of its fields
            if (isLocal)
            {
                for (uint32 fieldIndex = 0; fieldIndex < programUniformBlock->FieldCount; fieldIndex++)
                {
                    const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM_BLOCK_FIELD *programField = &programUniformBlock->Fields[fieldIndex];

                    // convert to a parameter type
                    SHADER_PARAMETER_TYPE parameterType;
                    OPENGL_SHADER_BIND_TARGET bindTarget;
                    if (!MapOpenGLUniformTypeToShaderParameterType(programField->Type, &parameterType, &bindTarget))
                    {
                        Log_ErrorPrintf("OpenGLShaderCompiler::ReflectProgram: Failed to map GL type %u (%s uniform block %s) to a parameter type.", programField->Type, programField->VariableName, programUniformBlock->BlockName);
                        return false;
                    }

                    // create parameter
                    OpenGLShaderCacheEntryParameter fieldParameter;
                    fieldParameter.NameLength = Y_strlen(programField->VariableName);
                    fieldParameter.Type = parameterType;
                    fieldParameter.UniformBlockIndex = constantBufferIndex;
                    fieldParameter.UniformBlockOffset = programField->OffsetInBuffer;
                    fieldParameter.ArraySize = (programField->ArraySize < 0) ? 0 : (uint32)programField->ArraySize;
                    fieldParameter.ArrayStride = programField->ArrayStride;

#if 0
                    // follow alignment rules and align the field to the next vec4
                    uint32 valueSize = ShaderParameterValueTypeSize(parameterType);
                    uint32 valueSizeRemainder = (valueSize % 16);
                    if (valueSizeRemainder != 0)
                        fieldParameter.ArrayStride = valueSize + (16 - valueSizeRemainder);
                    else
                        fieldParameter.ArrayStride = valueSize;
#endif

                    // should be binding to uniform only
                    DebugAssert(bindTarget == OPENGL_SHADER_BIND_TARGET_UNIFORM);

                    // remaining fields
                    fieldParameter.BindTarget = OPENGL_SHADER_BIND_TARGET_COUNT;
                    m_parameters.Add(outParameter);
                    m_parameterNames.Add(programField->VariableName);
                    m_parameterSamplerNames.Add(EmptyString);
                }
            }
        }
    }

    // map uniforms across
    for (uint32 inUniformIndex = 0; inUniformIndex < pReflection->UniformCount; inUniformIndex++)
    {
        const HLSLTRANSLATOR_GLSL_SHADER_UNIFORM *programUniform = &pReflection->Uniforms[inUniformIndex];

        // check for already existing
        // @TODO ensure types etc match
        uint32 parameterIndex;
        for (parameterIndex = 0; parameterIndex < m_parameters.GetSize(); parameterIndex++)
        {
            if (m_parameterNames[parameterIndex].Compare(programUniform->VariableName))
                break;
        }
        if (parameterIndex != m_parameters.GetSize())
            continue;

        // convert to a parameter type
        SHADER_PARAMETER_TYPE parameterType;
        OPENGL_SHADER_BIND_TARGET bindTarget;
        if (!MapOpenGLUniformTypeToShaderParameterType(programUniform->Type, &parameterType, &bindTarget))
        {
            Log_ErrorPrintf("OpenGLShaderCompiler::ReflectProgram: Failed to map GL type %u (uniform %s) to a parameter type.", programUniform->Type, programUniform->VariableName);
            return false;
        }

        // create parameter
        OpenGLShaderCacheEntryParameter outParameter;
        outParameter.NameLength = Y_strlen(programUniform->VariableName);
        outParameter.SamplerNameLength = (programUniform->SamplerVariableName != nullptr) ? Y_strlen(programUniform->SamplerVariableName) : 0;
        outParameter.Type = parameterType;
        outParameter.UniformBlockIndex = -1;
        outParameter.UniformBlockOffset = 0;
        outParameter.ArraySize = (programUniform->ArraySize < 0) ? 0 : (uint32)programUniform->ArraySize;

        // follow alignment rules and align the field to the next vec4
        uint32 valueSize = ShaderParameterValueTypeSize(parameterType);
        uint32 valueSizeRemainder = (valueSize % 16);
        if (valueSizeRemainder != 0)
            outParameter.ArrayStride = valueSize + (16 - valueSizeRemainder);
        else
            outParameter.ArrayStride = valueSize;

        // remaining fields
        outParameter.BindTarget = bindTarget;
        m_parameters.Add(outParameter);
        m_parameterNames.Add(programUniform->VariableName);
        if (programUniform->SamplerVariableName != nullptr)
            m_parameterSamplerNames.Add(programUniform->SamplerVariableName);
        else
            m_parameterSamplerNames.Add(EmptyString);
    }
    
    // map outputs
    if (stage == SHADER_PROGRAM_STAGE_PIXEL_SHADER && m_eRendererFeatureLevel > RENDERER_FEATURE_LEVEL_ES2)
    {
        for (uint32 inOutputIndex = 0; inOutputIndex < pReflection->OutputCount; inOutputIndex++)
        {
            const HLSLTRANSLATOR_GLSL_SHADER_OUTPUT *programOutput = &pReflection->Outputs[inOutputIndex];

            // parse the name. it should start with out_COLOR
            const char *currentNamePtr = programOutput->VariableName;
            if (Y_strnicmp(programOutput->VariableName, "out_COLOR", 9) == 0)
                currentNamePtr += 9;
            else if (Y_strnicmp(programOutput->VariableName, "out_Target", 10) == 0)
                currentNamePtr += 10;
            else
            {
                Log_ErrorPrintf("OpenGLShaderCompiler::ReflectProgram: Invalid output variable name: %s", programOutput->VariableName);
                return false;
            }

            // find the semantic index, if it is the end-of-string, assume zero
            uint32 renderTargetIndex = 0;
            if (*currentNamePtr != '\0')
                renderTargetIndex = StringConverter::StringToUInt32(currentNamePtr);

            // temp
            Log_DevPrintf("output '%s' -> render target %u", programOutput->VariableName, renderTargetIndex);

            // add an entry
            OpenGLShaderCacheEntryFragmentData outFragData;
            outFragData.NameLength = Y_strlen(programOutput->VariableName);
            outFragData.RenderTargetIndex = renderTargetIndex;
            m_fragDatas.Add(outFragData);
            m_fragDataNames.Add(programOutput->VariableName);
        }
    }

    return true;
}

bool ShaderCompilerOpenGL::InternalCompile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream)
{
    Timer compileTimer, stageCompileTimer;
    float compileTimeCompile, compileTimeReflect;
    bool result = false;
    
    // Create code arrays.
    stageCompileTimer.Reset();
    compileTimeCompile = 0.0f;
    compileTimeReflect = 0.0f;

    // glsl versions for feature levels
    static const uint32 featureLevelGLSLVersions[RENDERER_FEATURE_LEVEL_COUNT] =
    {
        330,        // RENDERER_FEATURE_LEVEL_ES2
        330,        // RENDERER_FEATURE_LEVEL_ES3
        330,        // RENDERER_FEATURE_LEVEL_SM4
        330//450    // RENDERER_FEATURE_LEVEL_SM5
    };
    static const uint32 featureLevelGLSLVersionsES[RENDERER_FEATURE_LEVEL_COUNT] =
    {
        100,        // RENDERER_FEATURE_LEVEL_ES2
        300,        // RENDERER_FEATURE_LEVEL_ES3
        300,        // RENDERER_FEATURE_LEVEL_SM4
        300//450    // RENDERER_FEATURE_LEVEL_SM5
    };

    // create compiler
    bool compileGLSLES = (m_eRendererPlatform == RENDERER_PLATFORM_OPENGLES2);
    uint32 compileGLSLVersion = (!compileGLSLES) ? featureLevelGLSLVersions[m_eRendererFeatureLevel] : featureLevelGLSLVersionsES[m_eRendererFeatureLevel];

    // compile stages
    for (uint32 stageIndex = 0; stageIndex < SHADER_PROGRAM_STAGE_COUNT; stageIndex++)
    {
        if (m_StageEntryPoints[stageIndex].GetLength() > 0)
        {
            if (!CompileShaderStage((SHADER_PROGRAM_STAGE)stageIndex, compileGLSLVersion, compileGLSLES))
                goto CLEANUP;

            float stageCompileTime = (float)stageCompileTimer.GetTimeMilliseconds();
            Log_DevPrintf("  Compiled stage %s in %.3f msec", NameTable_GetNameString(NameTables::ShaderProgramStage, stageIndex), stageCompileTime);
            compileTimeCompile += stageCompileTime;
            stageCompileTimer.Reset();
        }
    }

    // reflect stages
    for (uint32 stageIndex = 0; stageIndex < SHADER_PROGRAM_STAGE_COUNT; stageIndex++)
    {
        if (m_pOutputGLSL[stageIndex] != nullptr)
        {
            if (!ReflectShaderStage((SHADER_PROGRAM_STAGE)stageIndex))
                goto CLEANUP;

            float stageReflectTime = (float)stageCompileTimer.GetTimeMilliseconds();
            Log_DevPrintf("  Reflected stage %s in %.3f msec", NameTable_GetNameString(NameTables::ShaderProgramStage, stageIndex), stageReflectTime);
            compileTimeReflect += stageReflectTime;
            stageCompileTimer.Reset();
        }
    }
    
#if 1
    // Dump out variables
    for (uint32 atttributeIndex = 0; atttributeIndex < m_vertexAttributes.GetSize(); atttributeIndex++)
        Log_DevPrintf("Shader Vertex Attribute [%u] : %s", atttributeIndex, m_vertexAttributeNames[atttributeIndex].GetCharArray());
    for (uint32 constantBufferIndex = 0; constantBufferIndex < m_uniformBlocks.GetSize(); constantBufferIndex++)
        Log_DevPrintf("Shader Constant Buffer [%u] : %s, %u bytes, local: %s", constantBufferIndex, m_uniformBlockNames[constantBufferIndex].GetCharArray(), m_uniformBlocks[constantBufferIndex].Size, (m_uniformBlocks[constantBufferIndex].IsLocal) ? "yes" : "no");
    for (uint32 parameterIndex = 0; parameterIndex < m_parameters.GetSize(); parameterIndex++)
        Log_DevPrintf("Shader Parameter [%u] : %s, type %s", parameterIndex, m_parameterNames[parameterIndex].GetCharArray(), NameTable_GetNameString(NameTables::ShaderParameterType, m_parameters[parameterIndex].Type));
    for (uint32 fragDataIndex = 0; fragDataIndex < m_fragDatas.GetSize(); fragDataIndex++)
        Log_DevPrintf("Shader Fragment Output [%u] : %s, render target %u", fragDataIndex, m_fragDataNames[fragDataIndex].GetCharArray(), m_fragDatas[fragDataIndex].RenderTargetIndex);
#endif

    // write output
    if (!WriteOutput(pByteCodeStream))
        goto CLEANUP;

    // Compile succeded.
    result = true;
    Log_DevPrintf("Shader successfully compiled. Took %.3f msec (Compile: %.3f msec, Reflect: %.3f msec, Write: %.3f msec)",
        (float)compileTimer.GetTimeMilliseconds(), compileTimeCompile, compileTimeReflect, stageCompileTimer.GetTimeMilliseconds());

CLEANUP:
    // cleanup stages
    for (uint32 stageIndex = 0; stageIndex < SHADER_PROGRAM_STAGE_COUNT; stageIndex++)
    {
        if (m_pOutputGLSL[stageIndex] != nullptr)
        {
            HLSLTranslator_FreeMemory(m_pOutputGLSL[stageIndex]);
            m_pOutputGLSL[stageIndex] = nullptr;
        }
    }

    return result;
}

bool ShaderCompilerOpenGL::WriteOutput(ByteStream *pByteCodeStream)
{
    // binary writer
    BinaryWriter binaryWriter(pByteCodeStream);

    // make debug name
    SmallString debugName;
    //debugName.Format("%s", m_p)

    // construct header
    OpenGLShaderCacheEntryHeader header;
    header.Signature = OPENGL_SHADER_CACHE_ENTRY_HEADER;
    header.Platform = m_eRendererPlatform;
    header.FeatureLevel = m_eRendererFeatureLevel;
    header.VertexAttributeCount = m_vertexAttributes.GetSize();
    header.UniformBlockCount = m_uniformBlocks.GetSize();
    header.ParameterCount = m_parameters.GetSize();
    header.FragmentDataCount = m_fragDatas.GetSize();
    header.DebugNameLength = debugName.GetLength();
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
        header.StageSize[i] = (m_pOutputGLSL[i] != nullptr) ? m_pOutputGLSL[i]->OutputSourceLength : 0;
    
    // write header
    binaryWriter.WriteBytes(&header, sizeof(header));

    // write stage code
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_pOutputGLSL[i] != nullptr)
            binaryWriter.WriteBytes(m_pOutputGLSL[i]->OutputSource, m_pOutputGLSL[i]->OutputSourceLength);
    }

    // write attributes
    for (uint32 attributeIndex = 0; attributeIndex < m_vertexAttributes.GetSize(); attributeIndex++)
    {
        binaryWriter.WriteBytes(&m_vertexAttributes[attributeIndex], sizeof(OpenGLShaderCacheEntryVertexAttribute));
        binaryWriter.WriteFixedString(m_vertexAttributeNames[attributeIndex], m_vertexAttributes[attributeIndex].NameLength);
    }

    // write uniform blocks
    for (uint32 uniformBlockIndex = 0; uniformBlockIndex < m_uniformBlocks.GetSize(); uniformBlockIndex++)
    {
        binaryWriter.WriteBytes(&m_uniformBlocks[uniformBlockIndex], sizeof(OpenGLShaderCacheEntryUniformBlock));
        binaryWriter.WriteFixedString(m_uniformBlockNames[uniformBlockIndex], m_uniformBlocks[uniformBlockIndex].NameLength);
    }

    // write parameters
    for (uint32 parameterIndex = 0; parameterIndex < m_parameters.GetSize(); parameterIndex++)
    {
        binaryWriter.WriteBytes(&m_parameters[parameterIndex], sizeof(OpenGLShaderCacheEntryParameter));
        binaryWriter.WriteFixedString(m_parameterNames[parameterIndex], m_parameters[parameterIndex].NameLength);
        if (m_parameters[parameterIndex].SamplerNameLength > 0)
            binaryWriter.WriteFixedString(m_parameterSamplerNames[parameterIndex], m_parameters[parameterIndex].SamplerNameLength);
    }

    // write outputs
    for (uint32 fragDataIndex = 0; fragDataIndex < m_fragDatas.GetSize(); fragDataIndex++)
    {
        binaryWriter.WriteBytes(&m_fragDatas[fragDataIndex], sizeof(OpenGLShaderCacheEntryFragmentData));
        binaryWriter.WriteFixedString(m_fragDataNames[fragDataIndex], m_fragDatas[fragDataIndex].NameLength);
    }

    // write debug name
    if (header.DebugNameLength > 0)
        binaryWriter.WriteFixedString(debugName, header.DebugNameLength);

    // done
    Log_DevPrintf("OpenGLShaderCompiler::WriteOutput: %u attributes, %u constant buffers, %u parameters, %u outputs mapped, total size: %u bytes", m_vertexAttributes.GetSize(), m_uniformBlocks.GetSize(), m_parameters.GetSize(), m_fragDatas.GetSize(), (uint32)pByteCodeStream->GetSize());
    return !binaryWriter.InErrorState();
}

ShaderCompiler *ShaderCompiler::CreateOpenGLShaderCompiler(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters)
{
    return new ShaderCompilerOpenGL(pCallbacks, pParameters);
}
