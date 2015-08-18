#pragma once
#include "ResourceCompiler/ShaderCompiler.h"

#ifdef Y_PLATFORM_WINDOWS

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
    bool CompileShaderStage(SHADER_PROGRAM_STAGE stage, byte **ppShaderByteCode, uint32 *pShaderByteCodeSize);
};

#endif      // Y_PLATFORM_WINDOWS
