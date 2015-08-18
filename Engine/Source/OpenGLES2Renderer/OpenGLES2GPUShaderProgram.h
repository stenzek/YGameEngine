#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2ConstantLibrary.h"

class OpenGLES2GPUShaderProgram : public GPUShaderProgram
{
public:
    struct ShaderParameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        uint32 ArraySize;
        uint32 ArrayStride;
        uint32 BindTarget;
        int32 BindLocation;
        int32 BindSlot;
        OpenGLES2ConstantLibrary::ConstantID LibraryID;
        bool LibraryValueChanged;
    };

public:
    OpenGLES2GPUShaderProgram();
    virtual ~OpenGLES2GPUShaderProgram();

    // resource virtuals
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    // creation
    bool LoadProgram(OpenGLES2Renderer *pRenderer, const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream);

    // bind to a context that has no current shader
    void Bind(OpenGLES2GPUContext *pContext);

    // switch context to a new shader
    void Switch(OpenGLES2GPUContext *pContext, OpenGLES2GPUShaderProgram *pCurrentProgram);

    // when a constant parameter changes
    void OnLibraryConstantChanged(OpenGLES2ConstantLibrary::ConstantID constantID);

    // update the program copy of parameters before draw
    void CommitLibraryConstants(OpenGLES2GPUContext *pContext);

    // unbind from a context, resetting all shader states
    void Unbind(OpenGLES2GPUContext *pContext);

    // --- internal query api ---
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ShaderParameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(OpenGLES2GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(OpenGLES2GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterTexture(OpenGLES2GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource);

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
    // shader objects
    GLuint m_iGLShaderId[SHADER_PROGRAM_STAGE_COUNT];
    GLuint m_iGLProgramId;

    // parameters
    typedef MemArray<GPU_VERTEX_ELEMENT_DESC> VertexAttributeArray;
    typedef Array<ShaderParameter> ParameterArray;
    VertexAttributeArray m_vertexAttributes;
    ParameterArray m_parameters;

    // context we are bound to
    OpenGLES2GPUContext *m_pBoundContext;
};
