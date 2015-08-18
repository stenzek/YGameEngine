#pragma once
#include "ResourceCompiler/Common.h"

struct ShaderCompilerParameters;

class ResourceCompilerInterface : public ReferenceCounted
{
public:
    // Resources
    virtual BinaryBlob *CompileTexture(uint32 texturePlatform, const char *name) = 0;
    virtual BinaryBlob *CompileMaterialShader(const char *name) = 0;
    virtual BinaryBlob *CompileMaterial(const char *name) = 0;
    virtual BinaryBlob *CompileFont(uint32 texturePlatform, const char *name) = 0;
    virtual BinaryBlob *CompileBlockPalette(uint32 texturePlatform, const char *name) = 0;
    virtual BinaryBlob *CompileStaticMesh(const char *name) = 0;
    virtual BinaryBlob *CompileBlockMesh(const char *name) = 0;
    virtual BinaryBlob *CompileSkeleton(const char *name) = 0;
    virtual BinaryBlob *CompileSkeletalMesh(const char *name) = 0;
    virtual BinaryBlob *CompileSkeletalAnimation(const char *name) = 0;
    virtual BinaryBlob *CompileParticleSystem(const char *name) = 0;
    virtual BinaryBlob *CompileTerrainLayerList(const char *name) = 0;

    // Shader Compiler
    virtual bool CompileShader(const ShaderCompilerParameters *pParameters, ByteStream *pOutByteCodeStream, ByteStream *pOutInfoLogStream) = 0;

    // Interface creation
    static ResourceCompilerInterface *CreateIntegratedInterface();
    static ResourceCompilerInterface *CreateRemoteInterface();
};
