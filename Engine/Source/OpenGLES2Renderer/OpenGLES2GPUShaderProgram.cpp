#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUShaderProgram.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2Renderer.h"
#include "OpenGLRenderer/OpenGLShaderCacheEntry.h"
Log_SetChannel(Renderer);

OpenGLES2GPUShaderProgram::OpenGLES2GPUShaderProgram()
    : m_pBoundContext(nullptr)
{
    Y_memzero(m_iGLShaderId, sizeof(m_iGLShaderId));
    m_iGLProgramId = 0;
}

OpenGLES2GPUShaderProgram::~OpenGLES2GPUShaderProgram()
{
    DebugAssert(m_pBoundContext == nullptr);

    if (m_iGLProgramId > 0)
        glDeleteProgram(m_iGLProgramId);

    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_iGLShaderId[i] > 0)
            glDeleteShader(m_iGLShaderId[i]);
    }
}

void OpenGLES2GPUShaderProgram::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    // fixme
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void OpenGLES2GPUShaderProgram::SetDebugName(const char *name)
{
    // set shaders
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (m_iGLShaderId[i] != 0)
            OpenGLES2Helpers::SetObjectDebugName(GL_SHADER, m_iGLShaderId[i], name);
    }

    // set program
    OpenGLES2Helpers::SetObjectDebugName(GL_PROGRAM, m_iGLProgramId, name);
}

bool OpenGLES2GPUShaderProgram::LoadProgram(OpenGLES2Renderer *pRenderer, const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    // binary reader
    BinaryReader binaryReader(pByteCodeStream);

    // read/validate header
    OpenGLShaderCacheEntryHeader header;
    if (!binaryReader.SafeReadBytes(&header, sizeof(header)) || header.Signature != OPENGL_SHADER_CACHE_ENTRY_HEADER || header.Platform != RENDERER_PLATFORM_OPENGLES2 ||
        header.UniformBlockCount > 0 || header.FragmentDataCount > 0 || 
        header.StageSize[SHADER_PROGRAM_STAGE_HULL_SHADER] > 0 ||
        header.StageSize[SHADER_PROGRAM_STAGE_DOMAIN_SHADER] > 0 ||
        header.StageSize[SHADER_PROGRAM_STAGE_GEOMETRY_SHADER] > 0 || 
        header.StageSize[SHADER_PROGRAM_STAGE_COMPUTE_SHADER] > 0)
    {
        Log_ErrorPrintf("OpenGLES2GPUShaderProgram::Create: Shader cache entry header corrupted.");
        return false;
    }

    // begin checked section
    GL_CHECKED_SECTION_BEGIN();

    // time stuff
    Timer operationTimer;

    // generate shader ids
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if ((i != SHADER_PROGRAM_STAGE_VERTEX_SHADER && i != SHADER_PROGRAM_STAGE_PIXEL_SHADER) || header.StageSize[i] == 0)
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
        static const GLenum shaderStageMapping[SHADER_PROGRAM_STAGE_COUNT] = {
            GL_VERTEX_SHADER, 0, 0, 0, GL_FRAGMENT_SHADER, 0
        };

        // create the shader object
        if ((m_iGLShaderId[i] = glCreateShader(shaderStageMapping[i])) == 0)
        {
            GL_PRINT_ERROR("OpenGLES2GPUShaderProgram::Create: glCreateShader failed: ");
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
            Log_ErrorPrintf("OpenGLES2GPUShaderProgram::Create: %s failed compilation:\n", NameTable_GetNameString(NameTables::ShaderProgramStage, i));

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
            Log_WarningPrintf("OpenGLES2GPUShaderProgram::Create: %s compiled, with warnings:\n", NameTable_GetNameString(NameTables::ShaderProgramStage, i));

            GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
            GLint shaderInfoLogLengthWritten;
            glGetShaderInfoLog(m_iGLShaderId[i], shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
            shaderInfoLog[shaderInfoLogLength - 1] = 0;
            Log_WarningPrint(shaderInfoLog);
            delete[] shaderInfoLog;
        }

        Log_PerfPrintf("OpenGLES2GPUShaderProgram::Create: %s took %.4f msec to compile.", NameTable_GetNameString(NameTables::ShaderProgramStage, i), operationTimer.GetTimeMilliseconds());
        operationTimer.Reset();
    }

    // generate program id
    if ((m_iGLProgramId = glCreateProgram()) == 0)
    {
        GL_PRINT_ERROR("GLShaderProgram::Create: glCreateProgram failed: ");
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
        GL_PRINT_ERROR("GLShaderProgram::Create: One or more glAttachShader calls failed: ");
        return false;
    }

    // log
    Log_PerfPrintf("OpenGLES2GPUShaderProgram::Create: Took %.4f msec to attach.", operationTimer.GetTimeMilliseconds());
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
                Log_ErrorPrintf("OpenGLES2GPUShaderProgram::Create: Failed to find binding for required shader input attribute '%s'", attributeVariableName.GetCharArray());
                return false;
            }
        }

        // check for errors
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLES2GPUShaderProgram::Create: One or more glBindAttribLocation calls failed: ");
            return false;
        }

        // to avoid bouncing around vertex formats too often we're going to just store the whole thing, this could be moved to a cache
        m_vertexAttributes.AddRange(pVertexElements, nVertexElements);
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
            outParameter->ArraySize = inParameter.ArraySize;
            outParameter->ArrayStride = inParameter.ArrayStride;
            outParameter->BindTarget = inParameter.BindTarget;
            outParameter->BindLocation = -1;
            outParameter->BindSlot = -1;
            outParameter->LibraryID = OpenGLES2ConstantLibrary::ConstantIndexInvalid;
            outParameter->LibraryValueChanged = false;
        }
    }

    // log
    Log_PerfPrintf("OpenGLES2GPUShaderProgram::Create: Took %.4f msec to bind.", operationTimer.GetTimeMilliseconds());
    operationTimer.Reset();

    // link program
    {
        glLinkProgram(m_iGLProgramId);
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLES2GPUShaderProgram::Create: glLinkProgram failed: ");
            return false;
        }

        // check link status
        GLint shaderStatus = GL_FALSE;
        GLint shaderInfoLogLength = 0;
        glGetProgramiv(m_iGLProgramId, GL_LINK_STATUS, &shaderStatus);
        glGetProgramiv(m_iGLProgramId, GL_INFO_LOG_LENGTH, &shaderInfoLogLength);

        if (shaderStatus == GL_FALSE)
        {
            Log_ErrorPrintf("OpenGLES2GPUShaderProgram::Create: Program failed to link:\n");

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
            Log_WarningPrintf("OpenGLES2GPUShaderProgram::Create: Program linked, with warnings:\n");

            GLchar *shaderInfoLog = new GLchar[shaderInfoLogLength];
            GLint shaderInfoLogLengthWritten;
            glGetProgramInfoLog(m_iGLProgramId, shaderInfoLogLength, &shaderInfoLogLengthWritten, shaderInfoLog);
            shaderInfoLog[shaderInfoLogLength - 1] = 0;
            Log_WarningPrint(shaderInfoLog);
            delete[] shaderInfoLog;
        }

        // logging
        Log_PerfPrintf("OpenGLES2GPUShaderProgram::Create: Took %.4f msec to link.", operationTimer.GetTimeMilliseconds());
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

                // we have to check the library for a mapping of a constant buffer entry to this parameter.
                parameter->LibraryID = pRenderer->GetConstantLibrary()->LookupConstantID(parameter->Name);
                parameter->LibraryValueChanged = (parameter->LibraryID != OpenGLES2ConstantLibrary::ConstantIndexInvalid);
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
        }
    }

    // rebind old program
    glUseProgram(currentProgramId);

    Log_PerfPrintf("OpenGLES2GPUShaderProgram::Create: Took %.4f msec to do parameter linking.", operationTimer.GetTimeMilliseconds());

#endif

    // all ok
    return true;
}

void OpenGLES2GPUShaderProgram::Bind(OpenGLES2GPUContext *pContext)
{
    DebugAssert(m_pBoundContext == NULL);

    // bind program
    glUseProgram(m_iGLProgramId);
    m_pBoundContext = pContext;

    // bind attributes
    pContext->SetShaderVertexAttributes(m_vertexAttributes.GetBasePointer(), m_vertexAttributes.GetSize());

    // bind params
    for (ShaderParameter &parameter : m_parameters)
    {
        // if from constant library, reprogram
        if (parameter.LibraryID != OpenGLES2ConstantLibrary::ConstantIndexInvalid)
            parameter.LibraryValueChanged = true;
    }
}

void OpenGLES2GPUShaderProgram::Switch(OpenGLES2GPUContext *pContext, OpenGLES2GPUShaderProgram *pCurrentProgram)
{
    // bind program
    glUseProgram(m_iGLProgramId);

    // swap pointers
    pCurrentProgram->m_pBoundContext = nullptr;
    m_pBoundContext = pContext;

    // bind attributes
    pContext->SetShaderVertexAttributes(m_vertexAttributes.GetBasePointer(), m_vertexAttributes.GetSize());

    // bind params
    for (ShaderParameter &parameter : m_parameters)
    {
        // if from constant library, reprogram
        if (parameter.LibraryID != OpenGLES2ConstantLibrary::ConstantIndexInvalid)
            parameter.LibraryValueChanged = true;
    }
}

void OpenGLES2GPUShaderProgram::OnLibraryConstantChanged(OpenGLES2ConstantLibrary::ConstantID constantID)
{
    for (ShaderParameter &parameter : m_parameters)
    {
        if (parameter.LibraryID == constantID)
        {
            parameter.LibraryValueChanged = true;
            break;
        }
    }
}

void OpenGLES2GPUShaderProgram::CommitLibraryConstants(OpenGLES2GPUContext *pContext)
{
    for (ShaderParameter &parameter : m_parameters)
    {
        if (parameter.LibraryValueChanged)
        {
            // pull from constant library
            pContext->GetConstantLibrary()->UpdateProgramConstant(pContext->GetConstantLibraryBuffer(), parameter.LibraryID, parameter.BindLocation);
            parameter.LibraryValueChanged = false;
        }
    }
}

void OpenGLES2GPUShaderProgram::Unbind(OpenGLES2GPUContext *pContext)
{
    DebugAssert(m_pBoundContext == pContext);

    // unbind program
    glUseProgram(0);

    m_pBoundContext = nullptr;
}

void OpenGLES2GPUShaderProgram::InternalSetParameterValue(OpenGLES2GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == valueType);
    DebugAssert(parameterInfo->BindTarget == OPENGL_SHADER_BIND_TARGET_UNIFORM);
    DebugAssert(pContext == m_pBoundContext);
    
    // Library member? Shouldn't be set via this method, but just in case.
    DebugAssert(parameterInfo->LibraryID == OpenGLES2ConstantLibrary::ConstantIndexInvalid);

    // GL uniform
    if (parameterInfo->BindLocation >= 0)
    {
        // Send to GL
        OpenGLES2Helpers::GLUniformWrapper(parameterInfo->Type, parameterInfo->BindLocation, 1, pValue);
    }
}

void OpenGLES2GPUShaderProgram::InternalSetParameterValueArray(OpenGLES2GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements)
{
    const ShaderParameter *parameterInfo = &m_parameters[parameterIndex];
    DebugAssert(parameterInfo->Type == valueType);
    DebugAssert(parameterInfo->BindTarget == OPENGL_SHADER_BIND_TARGET_UNIFORM);
    DebugAssert(pContext == m_pBoundContext);

    // Library member? Shouldn't be set via this method, but just in case.
    DebugAssert(parameterInfo->LibraryID == OpenGLES2ConstantLibrary::ConstantIndexInvalid);

    // GL uniform?
    if (parameterInfo->BindLocation >= 0)
    {
        // Send to GL
        OpenGLES2Helpers::GLUniformWrapper(parameterInfo->Type, parameterInfo->BindLocation + firstElement, numElements, pValue);
    }
}

void OpenGLES2GPUShaderProgram::InternalSetParameterTexture(OpenGLES2GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource)
{
    const ShaderParameter *parameter = &m_parameters[parameterIndex];
    DebugAssert(parameter->BindTarget == OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT);
    if (parameter->BindSlot < 0)
        return;

    pContext->SetShaderTextureUnit(parameter->BindSlot, static_cast<GPUTexture *>(pResource));
}

uint32 OpenGLES2GPUShaderProgram::GetParameterCount() const
{
    return m_parameters.GetSize();
}

void OpenGLES2GPUShaderProgram::GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize)
{
    const ShaderParameter *parameter = &m_parameters[index];
    *name = parameter->Name;
    *type = parameter->Type;
    *arraySize = parameter->ArraySize;
}

void OpenGLES2GPUShaderProgram::SetParameterValue(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue)
{
    InternalSetParameterValue(static_cast<OpenGLES2GPUContext *>(pContext), index, valueType, pValue);
}

void OpenGLES2GPUShaderProgram::SetParameterValueArray(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements)
{
    InternalSetParameterValueArray(static_cast<OpenGLES2GPUContext *>(pContext), index, valueType, pValue, firstElement, numElements);
}

void OpenGLES2GPUShaderProgram::SetParameterStruct(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize)
{
    Log_ErrorPrintf("OpenGLES2GPUShaderProgram::SetParameterStruct: Unsupported in GLES 2.0");
    return;
}

void OpenGLES2GPUShaderProgram::SetParameterStructArray(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements)
{
    Log_ErrorPrintf("OpenGLES2GPUShaderProgram::SetParameterStructArray: Unsupported in GLES 2.0");
    return;
}

void OpenGLES2GPUShaderProgram::SetParameterResource(GPUContext *pContext, uint32 index, GPUResource *pResource)
{
    InternalSetParameterTexture(static_cast<OpenGLES2GPUContext *>(pContext), index, pResource);
}

void OpenGLES2GPUShaderProgram::SetParameterTexture(GPUContext *pContext, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState)
{
    if (pSamplerState != nullptr)
        Log_WarningPrintf("OpenGLES2GPUShaderProgram::SetParameterTexture: Sampler state will be ignored for GLES 2.0");
    
    InternalSetParameterTexture(static_cast<OpenGLES2GPUContext *>(pContext), index, pTexture);
}

GPUShaderProgram *OpenGLES2Renderer::CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    OpenGLES2GPUShaderProgram *pProgram = new OpenGLES2GPUShaderProgram();
    if (!pProgram->LoadProgram(this, pVertexElements, nVertexElements, pByteCodeStream))
    {
        pProgram->Release();
        return nullptr;
    }

    return pProgram;
}

GPUShaderProgram *OpenGLES2Renderer::CreateComputeProgram(ByteStream *pByteCodeStream)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateComputeProgram: Unsupported on GLES 2.0");
    return nullptr;
}
