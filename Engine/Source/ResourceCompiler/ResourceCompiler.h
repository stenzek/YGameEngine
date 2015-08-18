#pragma once
#include "ResourceCompiler/Common.h"
#include "ResourceCompiler/ResourceCompilerCallbacks.h"

struct ShaderCompilerParameters;

namespace ResourceCompiler
{
    BinaryBlob *CompileTexture(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name);
    BinaryBlob *CompileMaterialShader(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileMaterial(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileFont(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name);
    BinaryBlob *CompileBlockPalette(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name);
    BinaryBlob *CompileStaticMesh(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileBlockMesh(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileSkeleton(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileSkeletalMesh(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileSkeletalAnimation(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileParticleSystem(ResourceCompilerCallbacks *pCallbacks, const char *name);
    BinaryBlob *CompileTerrainLayerList(ResourceCompilerCallbacks *pCallbacks, const char *name);

    bool CompileShader(ResourceCompilerCallbacks *pCallbacks, const ShaderCompilerParameters *pParameters, ByteStream *pByteCodeStream, ByteStream *pInfoLogStream);
}

