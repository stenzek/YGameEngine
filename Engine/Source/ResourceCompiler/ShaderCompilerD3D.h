#pragma once
#include "ResourceCompiler/ShaderCompiler.h"

#ifdef Y_PLATFORM_WINDOWS

#include "D3D11Renderer/D3DShaderCacheEntry.h"
#include <d3dcompiler.h>

class ShaderCompilerD3D : public ShaderCompiler
{
public:
    ShaderCompilerD3D(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters);
    virtual ~ShaderCompilerD3D();

protected:
    // compile
    virtual bool InternalCompile(ByteStream *pByteCodeStream, ByteStream *pInfoLogStream) override;

    // helper functions
    void BuildD3DDefineList(SHADER_PROGRAM_STAGE stage, MemArray<D3D_SHADER_MACRO> &D3DMacroArray);
    bool CompileShaderStage(SHADER_PROGRAM_STAGE stage);
    bool ReflectShader(SHADER_PROGRAM_STAGE stage);
    bool LinkResourceSamplerParameters();
    bool LinkLocalConstantBuffersToParameters();

private:
    ID3DBlob *m_pStageByteCode[SHADER_PROGRAM_STAGE_COUNT];

    MemArray<D3DShaderCacheEntryVertexAttribute> m_outVertexAttributes;
    MemArray<D3DShaderCacheEntryConstantBuffer> m_outConstantBuffers;
    MemArray<D3DShaderCacheEntryParameter> m_outParameters;

    Array<String> m_outConstantBufferNames;
    Array<String> m_outParameterNames;
};

#endif      // Y_PLATFORM_WINDOWS
