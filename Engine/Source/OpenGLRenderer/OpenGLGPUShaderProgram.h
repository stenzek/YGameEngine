#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLShaderCacheEntry.h"

class OpenGLGPUShaderProgram : public GPUShaderProgram
{
public:
    struct ShaderUniformBuffer
    {
        String Name;
        uint32 ParameterIndex;
        uint32 Size;

        uint32 EngineConstantBufferIndex;
        int32 BlockIndex;
        int32 BindSlot;

        OpenGLGPUBuffer *pLocalGPUBuffer;
        byte *pLocalBuffer;
        int32 LocalBufferDirtyLowerBounds;
        int32 LocalBufferDirtyUpperBounds;
    };

    struct ShaderParameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        int32 UniformBlockIndex;
        uint32 UniformBlockOffset;
        uint32 ArraySize;
        uint32 ArrayStride;
        OPENGL_SHADER_BIND_TARGET BindTarget;
        int32 BindLocation;
        int32 BindSlot;
    };

public:
    OpenGLGPUShaderProgram();
    virtual ~OpenGLGPUShaderProgram();

    // resource virtuals
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    // creation
    bool LoadBlob(OpenGLGPUDevice *pRenderer, const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream);

    // bind to a context that has no current shader
    void Bind(OpenGLGPUContext *pContext);

    // switch context to a new shader
    void Switch(OpenGLGPUContext *pContext, OpenGLGPUShaderProgram *pCurrentProgram);

    // update the local constant buffers for this shader with pending values
    void CommitLocalConstantBuffers(OpenGLGPUContext *pContext);

    // unbind from a context, resetting all shader states
    void Unbind(OpenGLGPUContext *pContext);

    // --- internal query api ---
    uint32 InternalGetConstantBufferCount() const { return m_uniformBuffers.GetSize(); }
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ShaderUniformBuffer *GetConstantBuffer(uint32 Index) const { DebugAssert(Index < m_uniformBuffers.GetSize()); return &m_uniformBuffers[Index]; }
    const ShaderParameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(OpenGLGPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(OpenGLGPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterStruct(OpenGLGPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize);
    void InternalSetParameterStructArray(OpenGLGPUContext *pContext, uint32 parameterIndex, const void *pValues, uint32 valueSize, uint32 firstElement, uint32 numElements);
    void InternalSetParameterResource(OpenGLGPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, OpenGLGPUSamplerState *pLinkedSamplerState);
    void InternalBindAutomaticParameters(OpenGLGPUContext *pContext);

    // --- public parameter api ---
    virtual uint32 GetParameterCount() const override;
    virtual void GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize) override;
    virtual void SetParameterValue(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue) override;
    virtual void SetParameterValueArray(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements) override;
    virtual void SetParameterStruct(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize) override;
    virtual void SetParameterStructArray(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) override;
    virtual void SetParameterResource(GPUContext *pContext, uint32 index, GPUResource *pResource) override;
    virtual void SetParameterTexture(GPUContext *pContext, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) override;

protected:
    // arrays
    typedef MemArray<GPU_VERTEX_ELEMENT_DESC> VertexAttributeArray;
    typedef MemArray<ShaderUniformBuffer> ConstantBufferArray;
    typedef MemArray<ShaderParameter> ParameterArray;

    // shader objects
    GLuint m_iGLShaderId[SHADER_PROGRAM_STAGE_COUNT];
    GLuint m_iGLProgramId;

    // arrays of above
    VertexAttributeArray m_vertexAttributes;
    ConstantBufferArray m_uniformBuffers;
    ParameterArray m_parameters;

    // context we are bound to
    OpenGLGPUContext *m_pBoundContext;
};
