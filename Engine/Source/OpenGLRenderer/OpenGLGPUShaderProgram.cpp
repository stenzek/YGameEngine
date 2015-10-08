#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUShaderProgram.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "OpenGLRenderer/OpenGLRenderBackend.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(OpenGLRenderBackend);

#define LAZY_RESOURCE_CLEANUP_AFTER_SWITCH

OpenGLGPUShaderProgram::OpenGLGPUShaderProgram()
    : m_pBoundContext(nullptr)
{
    Y_memzero(m_iGLShaderId, sizeof(m_iGLShaderId));
    m_iGLProgramId = 0;
}

OpenGLGPUShaderProgram::~OpenGLGPUShaderProgram()
{
    DebugAssert(m_pBoundContext == NULL);

    if (m_iGLProgramId > 0)
        glDeleteProgram(m_iGLProgramId);

    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_iGLShaderId[i] > 0)
            glDeleteShader(m_iGLShaderId[i]);
    }

    for (uint32 i = 0; i < m_uniformBuffers.GetSize(); i++)
    {
        ShaderUniformBuffer &constantBuffer = m_uniformBuffers[i];
        if (constantBuffer.pLocalGPUBuffer != nullptr)
            constantBuffer.pLocalGPUBuffer->Release();
    }
}

void OpenGLGPUShaderProgram::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    // fixme
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void OpenGLGPUShaderProgram::SetDebugName(const char *name)
{
    // set shaders
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_iGLShaderId[i] != 0)
            OpenGLHelpers::SetObjectDebugName(GL_SHADER, m_iGLShaderId[i], name);
    }

    // set program
    OpenGLHelpers::SetObjectDebugName(GL_PROGRAM, m_iGLProgramId, name);
}

bool OpenGLGPUShaderProgram::LoadBlob(OpenGLGPUDevice *pRenderer, const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    // binary reader
    BinaryReader binaryReader(pByteCodeStream);

    // read/validate header
    OpenGLShaderCacheEntryHeader header;
    if (!binaryReader.SafeReadBytes(&header, sizeof(header)) || header.Signature != OPENGL_SHADER_CACHE_ENTRY_HEADER || header.Platform != RENDERER_PLATFORM_OPENGL)
    {
        Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: Shader cache entry header corrupted.");
        return false;
    }

    // begin checked section
    GL_CHECKED_SECTION_BEGIN();

    // time stuff
    Timer operationTimer;

    // generate shader ids
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (header.StageSize[i] == 0)
            continue;

        // read into memory
        GLint stageSourceLength = (GLint)header.StageSize[i];
        GLchar *pStageSource = new GLchar[stageSourceLength];
        if (!binaryReader.SafeReadBytes(pStageSource, stageSourceLength))
        {
            delete[] pStageSource;
            return false;
        }

        // lookup table of i -> GL shader stage
        static const GLenum glShaderStageMapping[SHADER_PROGRAM_STAGE_COUNT] = {
            GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER
        };

        // create the shader object
        if ((m_iGLShaderId[i] = glCreateShader(glShaderStageMapping[i])) == 0)
        {
            GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: glCreateShader failed: ");
            return false;
        }
        
        // pass it the shader source, and compile it
        glShaderSource(m_iGLShaderId[i], 1, (const GLchar **)&pStageSource, &stageSourceLength);
        glCompileShader(m_iGLShaderId[i]);
        delete[] pStageSource;

        GLint shaderStatus = GL_FALSE;
        GLint shaderInfoLogLength = 0;
        glGetShaderiv(m_iGLShaderId[i], GL_COMPILE_STATUS, &shaderStatus);
        glGetShaderiv(m_iGLShaderId[i], GL_INFO_LOG_LENGTH, &shaderInfoLogLength);
        
        if (shaderStatus == GL_FALSE)
        {
            Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: %s failed compilation:\n", NameTable_GetNameString(NameTables::ShaderProgramStage, i));

            if (shaderInfoLogLength > 1)
            {
                GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
                GLint shaderInfoLogLengthWritten;
                glGetShaderInfoLog(m_iGLShaderId[i], shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
                shaderInfoLog[shaderInfoLogLength - 1] = 0;
                Log_ErrorPrint(shaderInfoLog);
                delete[] shaderInfoLog;
            }

            return false;
        }

        if (shaderInfoLogLength > 1)
        {
            Log_WarningPrintf("OpenGLGPUShaderProgram::Create: %s compiled, with warnings:\n", NameTable_GetNameString(NameTables::ShaderProgramStage, i));

            GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
            GLint shaderInfoLogLengthWritten;
            glGetShaderInfoLog(m_iGLShaderId[i], shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
            shaderInfoLog[shaderInfoLogLength - 1] = 0;
            Log_WarningPrint(shaderInfoLog);
            delete[] shaderInfoLog;
        }

        Log_PerfPrintf("OpenGLGPUShaderProgram::Create: %s took %.4f msec to compile.", NameTable_GetNameString(NameTables::ShaderProgramStage, i), operationTimer.GetTimeMilliseconds());
        operationTimer.Reset();
    }

    // generate program id
    if ((m_iGLProgramId = glCreateProgram()) == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: glCreateProgram failed: ");
        return false;
    }

    // attach shaders to program
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_iGLShaderId[i] != 0)
            glAttachShader(m_iGLProgramId, m_iGLShaderId[i]);
    }

    // check for any errors
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: One or more glAttachShader calls failed: ");
        return false;
    }

    // log
    Log_PerfPrintf("OpenGLGPUShaderProgram::Create: Took %.4f msec to attach.", operationTimer.GetTimeMilliseconds());
    operationTimer.Reset();

    // bind attributes
    if (header.VertexAttributeCount > 0)
    {
        for (uint32 attributeIndex = 0; attributeIndex < header.VertexAttributeCount; attributeIndex++)
        {
            OpenGLShaderCacheEntryVertexAttribute attribute;
            SmallString attributeVariableName;
            if (!binaryReader.SafeReadBytes(&attribute, sizeof(attribute)) || !binaryReader.SafeReadFixedString(attribute.NameLength, &attributeVariableName))
                return false;

            // find the corresponding vertex element in spec
            uint32 ilIndex;
            for (ilIndex = 0; ilIndex < nVertexElements; ilIndex++)
            {
                if (pVertexElements[ilIndex].Semantic == (GPU_VERTEX_ELEMENT_SEMANTIC)attribute.SemanticName &&
                    pVertexElements[ilIndex].SemanticIndex == attribute.SemanticIndex)
                {
                    // bind to the corresponding attribute location
                    glBindAttribLocation(m_iGLProgramId, ilIndex, attributeVariableName);
                    break;
                }
            }

            // found?
            if (ilIndex == nVertexElements)
            {
                Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: Failed to find binding for required shader input attribute '%s'", attributeVariableName.GetCharArray());
                return false;
            }
        }

        // check for errors
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: One or more glBindAttribLocation calls failed: ");
            return false;
        }

        // to avoid bouncing around vertex formats too often we're going to just store the whole thing, this could be moved to a cache
        m_vertexAttributes.AddRange(pVertexElements, nVertexElements);
    }

    // pull in constant buffers
    if (header.UniformBlockCount > 0)
    {
        m_uniformBuffers.Resize(header.UniformBlockCount);
        for (uint32 uniformBlockIndex = 0; uniformBlockIndex < m_uniformBuffers.GetSize(); uniformBlockIndex++)
        {
            OpenGLShaderCacheEntryUniformBlock inUniformBlock;
            SmallString uniformBlockName;
            if (!binaryReader.SafeReadBytes(&inUniformBlock, sizeof(inUniformBlock)) || !binaryReader.SafeReadFixedString(inUniformBlock.NameLength, &uniformBlockName))
                return false;

            ShaderUniformBuffer *uniformBuffer = &m_uniformBuffers[uniformBlockIndex];
            uniformBuffer->Name = uniformBlockName;
            uniformBuffer->ParameterIndex = inUniformBlock.ParameterIndex;
            uniformBuffer->Size = inUniformBlock.Size;
            uniformBuffer->EngineConstantBufferIndex = 0;
            uniformBuffer->BlockIndex = -1;
            uniformBuffer->BindSlot = -1;
            uniformBuffer->pLocalGPUBuffer = nullptr;
            uniformBuffer->pLocalBuffer = nullptr;
            uniformBuffer->LocalBufferDirtyLowerBounds = uniformBuffer->LocalBufferDirtyUpperBounds = -1;

            if (inUniformBlock.IsLocal)
            {
                // create local buffer
                uniformBuffer->pLocalBuffer = new byte[uniformBuffer->Size];
                Y_memzero(uniformBuffer->pLocalBuffer, uniformBuffer->Size);

                // create gpu buffer
                GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_WRITABLE | GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER, uniformBuffer->Size);
                uniformBuffer->pLocalGPUBuffer = static_cast<OpenGLGPUBuffer *>(pRenderer->CreateBuffer(&bufferDesc, uniformBuffer->pLocalBuffer));
                if (uniformBuffer->pLocalGPUBuffer == nullptr)
                {
                    Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: Failed to create local uniform buffer in GPU memory: %s (size: %u)", uniformBuffer->Name.GetCharArray(), uniformBuffer->Size);
                    return false;
                }
            }
            else
            {
                // lookup in engine constant buffer list
                const ShaderConstantBuffer *pEngineConstantBuffer = ShaderConstantBuffer::GetShaderConstantBufferByName(uniformBuffer->Name, RENDERER_PLATFORM_OPENGL, OpenGLRenderBackend::GetInstance()->GetFeatureLevel());
                if (pEngineConstantBuffer == nullptr)
                {
                    Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: Shader is requesting unknown non-local constant buffer named '%s'.", uniformBuffer->Name.GetCharArray());
                    return false;
                }

                // store index
                uniformBuffer->EngineConstantBufferIndex = pEngineConstantBuffer->GetIndex();
            }
        }
    }

    // pull in parameters
    if (header.ParameterCount > 0)
    {
        m_parameters.Resize(header.ParameterCount);
        for (uint32 parameterIndex = 0; parameterIndex < m_parameters.GetSize(); parameterIndex++)
        {
            OpenGLShaderCacheEntryParameter inParameter;
            SmallString parameterName;
            if (!binaryReader.SafeReadBytes(&inParameter, sizeof(inParameter)) || !binaryReader.SafeReadFixedString(inParameter.NameLength, &parameterName))
                return false;

            // create parameter information
            ShaderParameter *outParameter = &m_parameters[parameterIndex];
            outParameter->Name = parameterName;
            outParameter->Type = (SHADER_PARAMETER_TYPE)inParameter.Type;
            outParameter->UniformBlockIndex = inParameter.UniformBlockIndex;
            outParameter->UniformBlockOffset = inParameter.UniformBlockOffset;
            outParameter->ArraySize = inParameter.ArraySize;
            outParameter->ArrayStride = inParameter.ArrayStride;
            outParameter->BindTarget = (OPENGL_SHADER_BIND_TARGET)inParameter.BindTarget;
            outParameter->BindLocation = -1;
            outParameter->BindSlot = -1;
        }
    }

    // bind outputs
    if (header.FragmentDataCount > 0)
    {
        for (uint32 fragDataIndex = 0; fragDataIndex < header.FragmentDataCount; fragDataIndex++)
        {
            OpenGLShaderCacheEntryFragmentData fragData;
            SmallString variableName;
            if (!binaryReader.SafeReadBytes(&fragData, sizeof(fragData)) || !binaryReader.SafeReadFixedString(fragData.NameLength, &variableName))
                return false;

            // bind to the corresponding attribute location
            glBindFragDataLocation(m_iGLProgramId, fragData.RenderTargetIndex, variableName);
        }

        // check for errors
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: One or more glBindAttribLocation calls failed: ");
            return false;
        }
    }

    // log
    Log_PerfPrintf("OpenGLGPUShaderProgram::Create: Took %.4f msec to bind.", operationTimer.GetTimeMilliseconds());
    operationTimer.Reset();

    // link program
    {
        glLinkProgram(m_iGLProgramId);
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLGPUShaderProgram::Create: glLinkProgram failed: ");
            return false;
        }

        // check link status
        GLint shaderStatus = GL_FALSE;
        GLint shaderInfoLogLength = 0;
        glGetProgramiv(m_iGLProgramId, GL_LINK_STATUS, &shaderStatus);
        glGetProgramiv(m_iGLProgramId, GL_INFO_LOG_LENGTH, &shaderInfoLogLength);

        if (shaderStatus == GL_FALSE)
        {
            Log_ErrorPrintf("OpenGLGPUShaderProgram::Create: Program failed to link:\n");

            if (shaderInfoLogLength > 1)
            {
                GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
                GLint shaderInfoLogLengthWritten;
                glGetProgramInfoLog(m_iGLProgramId, shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
                shaderInfoLog[shaderInfoLogLength - 1] = 0;
                Log_ErrorPrint(shaderInfoLog);
                delete[] shaderInfoLog;
            }

            return false;
        }

        if (shaderInfoLogLength > 1)
        {
            Log_WarningPrintf("OpenGLGPUShaderProgram::Create: Program linked, with warnings:\n");

            GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
            GLint shaderInfoLogLengthWritten;
            glGetProgramInfoLog(m_iGLProgramId, shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
            shaderInfoLog[shaderInfoLogLength - 1] = 0;
            Log_WarningPrint(shaderInfoLog);
            delete[] shaderInfoLog;
        }

        // logging
        Log_PerfPrintf("OpenGLGPUShaderProgram::Create: Took %.4f msec to link.", operationTimer.GetTimeMilliseconds());
        operationTimer.Reset();
    }

#ifdef Y_BUILD_CONFIG_DEBUG
    if (header.DebugNameLength > 0)
    {
        SmallString debugName;
        if (binaryReader.SafeReadFixedString(header.DebugNameLength, &debugName))
            SetDebugName(debugName);
    }
#endif

#if 1   // todo: lazy bind

    // store the current program for this thread's context
    GLint currentProgramId = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgramId);

    // bind this program
    glUseProgram(m_iGLProgramId);

    // slot allocators
    uint32 textureSlotAllocator = 0;
    uint32 uniformBufferSlotAllocator = 0;
    uint32 imageSlotAllocator = 0;

    // get uniform block locations
    for (uint32 i = 0; i < m_uniformBuffers.GetSize(); i++)
    {
        ShaderUniformBuffer *uniformBuffer = &m_uniformBuffers[i];
        
        // find in the linked program
        GLuint blockIndex = glGetUniformBlockIndex(m_iGLProgramId, uniformBuffer->Name);
        if (blockIndex == GL_INVALID_INDEX)
        {
            // optimized out
            continue;
        }

        // store the location, and allocate a slot
        uniformBuffer->BlockIndex = blockIndex;
        uniformBuffer->BindSlot = uniformBufferSlotAllocator++;

        // and update the linked parameter
        ShaderParameter *parameter = &m_parameters[uniformBuffer->ParameterIndex];
        DebugAssert(parameter->BindTarget == OPENGL_SHADER_BIND_TARGET_UNIFORM_BUFFER);
        parameter->BindLocation = uniformBuffer->BlockIndex;
        parameter->BindSlot = uniformBuffer->BlockIndex;

        // bind the actual parameter
        glUniformBlockBinding(m_iGLProgramId, uniformBuffer->BlockIndex, uniformBuffer->BindSlot);
    }

    // get parameter locations
    for (uint32 i = 0; i < m_parameters.GetSize(); i++)
    {
        ShaderParameter *parameter = &m_parameters[i];
        switch (parameter->BindTarget)
        {
        case OPENGL_SHADER_BIND_TARGET_UNIFORM:
            {
                // Find the uniform location for this parameter
                GLint uniformLocation = glGetUniformLocation(m_iGLProgramId, parameter->Name);
                if (uniformLocation < 0)
                {
                    // optimized out
                    continue;
                }

                // Store the location
                parameter->BindLocation = uniformLocation;
            }
            break;

        case OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT:
            {
                // Find the uniform location for this parameter
                GLint uniformLocation = glGetUniformLocation(m_iGLProgramId, parameter->Name);
                if (uniformLocation < 0)
                {
                    // optimized out
                    continue;
                }

                // Store the location, and allocate a slot
                parameter->BindLocation = uniformLocation;
                parameter->BindSlot = textureSlotAllocator++;

                // and pass this to the shader
                glUniform1i(parameter->BindLocation, parameter->BindSlot);
            }
            break;

        case OPENGL_SHADER_BIND_TARGET_IMAGE_UNIT:
            {
                // Find the uniform location for this parameter
                GLint uniformLocation = glGetUniformLocation(m_iGLProgramId, parameter->Name);
                if (uniformLocation < 0)
                {
                    // optimized out
                    continue;
                }

                // Store the location, and allocate a slot
                parameter->BindLocation = uniformLocation;
                parameter->BindSlot = imageSlotAllocator++;

                // and pass this to the shader
                glUniform1i(parameter->BindLocation, parameter->BindSlot);
            }
            break;
        }
    }

    // rebind old program
    glUseProgram(currentProgramId);

    Log_PerfPrintf("OpenGLGPUShaderProgram::Create: Took %.4f msec to do parameter linking.", operationTimer.GetTimeMilliseconds());

#endif

    // all ok
    return true;
}

void OpenGLGPUShaderProgram::Bind(OpenGLGPUContext *pContext)
{
    DebugAssert(m_pBoundContext == NULL);

    // bind program
    glUseProgram(m_iGLProgramId);
    m_pBoundContext = pContext;

    // bind attributes
    pContext->SetShaderVertexAttributes(m_vertexAttributes.GetBasePointer(), m_vertexAttributes.GetSize());

    // bind params
    InternalBindAutomaticParameters(pContext);
}

void OpenGLGPUShaderProgram::InternalBindAutomaticParameters(OpenGLGPUContext *pContext)
{
    // bind local and global constant buffers
    for (uint32 constantBufferIndex = 0; constantBufferIndex < m_uniformBuffers.GetSize(); constantBufferIndex++)
    {
        const ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[constantBufferIndex];
        if (constantBuffer->BlockIndex < 0)
            continue;

        // local?
        OpenGLGPUBuffer *pUniformBuffer = (constantBuffer->pLocalGPUBuffer != nullptr) ? constantBuffer->pLocalGPUBuffer : m_pBoundContext->GetConstantBuffer(constantBuffer->EngineConstantBufferIndex);
        pContext->SetShaderUniformBlock(constantBuffer->BindSlot, pUniformBuffer);
    }
}

void OpenGLGPUShaderProgram::Switch(OpenGLGPUContext *pContext, OpenGLGPUShaderProgram *pCurrentProgram)
{
#ifdef LAZY_RESOURCE_CLEANUP_AFTER_SWITCH
    // bind program
    glUseProgram(m_iGLProgramId);

    // swap pointers
    pCurrentProgram->m_pBoundContext = nullptr;
    m_pBoundContext = pContext;

    // bind attributes
    pContext->SetShaderVertexAttributes(m_vertexAttributes.GetBasePointer(), m_vertexAttributes.GetSize());

    // bind params
    InternalBindAutomaticParameters(pContext);
#else
    pCurrentProgram->Unbind(pContext);
    Bind(pContext);
#endif
}

void OpenGLGPUShaderProgram::CommitLocalConstantBuffers(OpenGLGPUContext *pContext)
{
    // commit constant buffers
    for (uint32 i = 0; i < m_uniformBuffers.GetSize(); i++)
    {
        ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[i];
        if (constantBuffer->LocalBufferDirtyLowerBounds >= 0)
        {
            pContext->WriteBuffer(constantBuffer->pLocalGPUBuffer, reinterpret_cast<const byte *>(constantBuffer->pLocalBuffer) + constantBuffer->LocalBufferDirtyLowerBounds, 
                                  constantBuffer->LocalBufferDirtyLowerBounds, constantBuffer->LocalBufferDirtyUpperBounds - constantBuffer->LocalBufferDirtyLowerBounds);

            constantBuffer->LocalBufferDirtyLowerBounds = constantBuffer->LocalBufferDirtyUpperBounds = -1;
        }
    }
}

void OpenGLGPUShaderProgram::Unbind(OpenGLGPUContext *pContext)
{
    DebugAssert(m_pBoundContext == pContext);

#ifndef LAZY_RESOURCE_CLEANUP_AFTER_SWITCH
    // unset parameters that we match against
    for (uint32 parameterIndex = 0; parameterIndex < m_parameters.GetSize(); parameterIndex++)
    {
        const ShaderParameter *parameter = &m_parameters[parameterIndex];
        if (parameter->BindSlot < 0)
            continue;

        switch (parameter->BindTarget)
        {
        case OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT:
            pContext->SetShaderTextureUnit(parameter->BindSlot, nullptr, nullptr);
            break;

        case OPENGL_SHADER_BIND_TARGET_UNIFORM_BUFFER:
            pContext->SetShaderUniformBlock(parameter->BindSlot, nullptr);
            break;

        case OPENGL_SHADER_BIND_TARGET_IMAGE_UNIT:
            pContext->SetShaderImageUnit(parameter->BindSlot, nullptr);
            break;

        case OPENGL_SHADER_BIND_TARGET_SHADER_STORAGE_BUFFER:
            pContext->SetShaderStorageBuffer(parameter->BindSlot, nullptr);
            break;
        }
    }
#endif

    // unbind program
    glUseProgram(0);

    m_pBoundContext = nullptr;
}

static void GLUniformWrapper(SHADER_PARAMETER_TYPE type, GLuint location, GLuint arraySize, const void *pValue)
{
    // Invoke the appropriate glUniform() command
    switch (type)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        glUniform1iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL2:
        glUniform2iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL3:
        glUniform3iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL4:
        glUniform4iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT:
        glUniform1iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT2:
        glUniform2iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT3:
        glUniform3iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT4:
        glUniform4iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_UINT:
        glUniform1uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_UINT2:
        glUniform2uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_UINT3:
        glUniform3uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_UINT4:
        glUniform4uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        glUniform1fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        glUniform2fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        glUniform3fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        glUniform4fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2X2:
        glUniformMatrix2fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3X3:
        glUniformMatrix3fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3X4:
        glUniformMatrix4x3fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4X4:
        glUniformMatrix4fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;
    }
}

void OpenGLGPUShaderProgram::InternalSetParameterValue(OpenGLGPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == valueType);
    DebugAssert(pContext == m_pBoundContext);
    
    // Local constant buffer?
    if (parameterInfo->UniformBlockIndex >= 0)
    {
        // get the constant buffer
        ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[parameterInfo->UniformBlockIndex];
        DebugAssert(constantBuffer->pLocalBuffer != nullptr);

        // get size to copy
        uint32 valueSize = ShaderParameterValueTypeSize(parameterInfo->Type);
        DebugAssert(valueSize > 0);

        // compare memory first
        byte *pBufferPtr = constantBuffer->pLocalBuffer + parameterInfo->UniformBlockOffset;
        if (Y_memcmp(pBufferPtr, pValue, valueSize) != 0)
        {
            // write to the local constant buffer
            Y_memcpy(pBufferPtr, pValue, valueSize);
            if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
            {
                constantBuffer->LocalBufferDirtyLowerBounds = (int32)parameterInfo->UniformBlockOffset;
                constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)valueSize;
            }
            else
            {
                constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)parameterInfo->UniformBlockOffset);
                constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)valueSize);
            }
        }
    }
    // GL uniform?
    else if (parameterInfo->BindLocation >= 0)
    {
        // Send to GL
        GLUniformWrapper(parameterInfo->Type, parameterInfo->BindLocation, 1, pValue);
    }
}

void OpenGLGPUShaderProgram::InternalSetParameterValueArray(OpenGLGPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == valueType);
    DebugAssert(pContext == m_pBoundContext);

    // Local constant buffer?
    if (parameterInfo->UniformBlockIndex >= 0)
    {
        // get the constant buffer
        ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[parameterInfo->UniformBlockIndex];
        DebugAssert(constantBuffer->pLocalBuffer != nullptr);

        // get size to copy
        uint32 valueSize = ShaderParameterValueTypeSize(parameterInfo->Type);
        DebugAssert(valueSize > 0);

        // if there is no padding, this can be done in a single operation
        byte *pBufferPtr = constantBuffer->pLocalBuffer + parameterInfo->UniformBlockOffset + (firstElement * parameterInfo->ArrayStride);
        if (valueSize == parameterInfo->ArrayStride)
        {
            uint32 copySize = valueSize * numElements;
            if (Y_memcmp(pBufferPtr, pValue, copySize) != 0)
            {
                // write to the local constant buffer
                Y_memcpy(pBufferPtr, pValue, copySize);
                if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride);
                    constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride);
                }
                else
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride));
                    constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride));
                }
            }
        }
        else
        {
            if (Y_memcmp_stride(pBufferPtr, parameterInfo->ArrayStride, pValue, valueSize, valueSize, numElements) != 0)
            {
                // write to the local constant buffer
                Y_memcpy_stride(pBufferPtr, parameterInfo->ArrayStride, pValue, valueSize, valueSize, numElements);
                if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride);
                    constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride);
                }
                else
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride));
                    constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride));
                }
            }
        }
    }
    // GL uniform?
    else if (parameterInfo->BindLocation >= 0)
    {
        // Send to GL
        GLUniformWrapper(parameterInfo->Type, parameterInfo->BindLocation + firstElement, numElements, pValue);
    }
}

void OpenGLGPUShaderProgram::InternalSetParameterStruct(OpenGLGPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == SHADER_PARAMETER_TYPE_STRUCT);
    DebugAssert(pContext == m_pBoundContext && valueSize < parameterInfo->ArrayStride);
    
    // Local constant buffer?
    if (parameterInfo->UniformBlockIndex >= 0)
    {
        // get the constant buffer
        ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[parameterInfo->UniformBlockIndex];
        DebugAssert(constantBuffer->pLocalBuffer != nullptr);

        // compare memory first
        byte *pBufferPtr = constantBuffer->pLocalBuffer + parameterInfo->UniformBlockOffset;
        if (Y_memcmp(pBufferPtr, pValue, valueSize) != 0)
        {
            // write to the local constant buffer
            Y_memcpy(pBufferPtr, pValue, valueSize);
            if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
            {
                constantBuffer->LocalBufferDirtyLowerBounds = (int32)parameterInfo->UniformBlockOffset;
                constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)valueSize;
            }
            else
            {
                constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)parameterInfo->UniformBlockOffset);
                constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)valueSize);
            }
        }
    }
}

void OpenGLGPUShaderProgram::InternalSetParameterStructArray(OpenGLGPUContext *pContext, uint32 parameterIndex, const void *pValues, uint32 valueSize, uint32 firstElement, uint32 numElements)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == SHADER_PARAMETER_TYPE_STRUCT && valueSize <= parameterInfo->ArrayStride);
    DebugAssert(pContext == m_pBoundContext);

    // Local constant buffer?
    if (parameterInfo->UniformBlockIndex >= 0)
    {
        // get the constant buffer
        ShaderUniformBuffer *constantBuffer = &m_uniformBuffers[parameterInfo->UniformBlockIndex];
        DebugAssert(constantBuffer->pLocalBuffer != nullptr);


        // if there is no padding, this can be done in a single operation
        byte *pBufferPtr = constantBuffer->pLocalBuffer + parameterInfo->UniformBlockOffset + (firstElement * parameterInfo->ArrayStride);
        if (valueSize == parameterInfo->ArrayStride)
        {
            uint32 copySize = valueSize * numElements;
            if (Y_memcmp(pBufferPtr, pValues, copySize) != 0)
            {
                // write to the local constant buffer
                Y_memcpy(pBufferPtr, pValues, copySize);
                if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride);
                    constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride);
                }
                else
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride));
                    constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride));
                }
            }
        }
        else
        {
            if (Y_memcmp_stride(pBufferPtr, parameterInfo->ArrayStride, pValues, valueSize, valueSize, numElements) != 0)
            {
                // write to the local constant buffer
                Y_memcpy_stride(pBufferPtr, parameterInfo->ArrayStride, pValues, valueSize, valueSize, numElements);
                if (constantBuffer->LocalBufferDirtyLowerBounds < 0)
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride);
                    constantBuffer->LocalBufferDirtyUpperBounds = constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride);
                }
                else
                {
                    constantBuffer->LocalBufferDirtyLowerBounds = Min(constantBuffer->LocalBufferDirtyLowerBounds, (int32)(parameterInfo->UniformBlockOffset + firstElement * parameterInfo->ArrayStride));
                    constantBuffer->LocalBufferDirtyUpperBounds = Max(constantBuffer->LocalBufferDirtyUpperBounds, constantBuffer->LocalBufferDirtyLowerBounds + (int32)(valueSize * parameterInfo->ArrayStride));
                }
            }
        }
    }
}

void OpenGLGPUShaderProgram::InternalSetParameterResource(OpenGLGPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, OpenGLGPUSamplerState *pLinkedSamplerState)
{
    const ShaderParameter *parameter = &m_parameters[parameterIndex];
    if (parameter->BindSlot < 0)
        return;

    switch (parameter->BindTarget)
    {
    case OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT:
        pContext->SetShaderTextureUnit(parameter->BindSlot, static_cast<GPUTexture *>(pResource), pLinkedSamplerState);
        break;

    case OPENGL_SHADER_BIND_TARGET_UNIFORM_BUFFER:
        DebugAssert(pResource->GetResourceType() == GPU_RESOURCE_TYPE_BUFFER);
        pContext->SetShaderUniformBlock(parameter->BindSlot, static_cast<OpenGLGPUBuffer *>(pResource));
        break;

    case OPENGL_SHADER_BIND_TARGET_IMAGE_UNIT:
        pContext->SetShaderImageUnit(parameter->BindSlot, static_cast<GPUTexture *>(pResource));
        break;

    case OPENGL_SHADER_BIND_TARGET_SHADER_STORAGE_BUFFER:
        pContext->SetShaderStorageBuffer(parameter->BindSlot, pResource);
        break;
    }
}

uint32 OpenGLGPUShaderProgram::GetParameterCount() const
{
    return m_parameters.GetSize();
}

void OpenGLGPUShaderProgram::GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize)
{
    const ShaderParameter *parameter = &m_parameters[index];
    *name = parameter->Name;
    *type = parameter->Type;
    *arraySize = parameter->ArraySize;
}

GPUShaderProgram *OpenGLGPUDevice::CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    OpenGLGPUShaderProgram *pProgram = new OpenGLGPUShaderProgram();
    if (!pProgram->LoadBlob(this, pVertexElements, nVertexElements, pByteCodeStream))
    {
        pProgram->Release();
        return nullptr;
    }

    FlushOffThreadCommands();
    return pProgram;
}

GPUShaderProgram *OpenGLGPUDevice::CreateComputeProgram(ByteStream *pByteCodeStream)
{
    OpenGLGPUShaderProgram *pProgram = new OpenGLGPUShaderProgram();
    if (!pProgram->LoadBlob(this, nullptr, 0, pByteCodeStream))
    {
        pProgram->Release();
        return nullptr;
    }

    FlushOffThreadCommands();
    return pProgram;
}
