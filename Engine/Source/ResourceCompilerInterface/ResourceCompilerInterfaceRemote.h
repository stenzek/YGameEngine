#pragma once
#include "ResourceCompilerInterface/ResourceCompilerInterface.h"
#include "ResourceCompiler/ResourceCompilerCallbacks.h"
#include "YBaseLib/Subprocess.h"

enum REMOTE_COMMAND
{
    REMOTE_COMMAND_EXIT,
    REMOTE_COMMAND_COMPILE_TEXTURE,
    REMOTE_COMMAND_COMPILE_MATERIAL_SHADER,
    REMOTE_COMMAND_COMPILE_MATERIAL,
    REMOTE_COMMAND_COMPILE_FONT,
    REMOTE_COMMAND_COMPILE_BLOCK_PALETTE,
    REMOTE_COMMAND_COMPILE_STATIC_MESH,
    REMOTE_COMMAND_COMPILE_BLOCK_MESH,
    REMOTE_COMMAND_COMPILE_SKELETON,
    REMOTE_COMMAND_COMPILE_SKELETAL_ANIMATION,
    REMOTE_COMMAND_COMPILE_SKELETAL_MESH,
    REMOTE_COMMAND_COMPILE_PARTICLE_SYSTEM,
    REMOTE_COMMAND_COMPILE_TERRAIN_LAYER_LIST,
    REMOTE_COMMAND_COMPILE_SHADER,
    REMOTE_COMMAND_GET_FILE_CONTENTS,
    REMOTE_COMMAND_GET_COMPILED_MATERIAL_SHADER,
    REMOTE_COMMAND_GET_COMPILED_SKELETON,
    REMOTE_COMMAND_SUCCESS,
    REMOTE_COMMAND_FAILURE,
    NUM_REMOTE_COMMANDS
};

#pragma pack(push, 4)
struct REMOTE_COMMAND_HEADER
{
    uint32 Command;
    uint32 PayloadSize;
};
#pragma pack(pop)

class ResourceCompilerInterfaceRemote : public ResourceCompilerInterface
{
public:
    ResourceCompilerInterfaceRemote(Subprocess *pSubProcess, Subprocess::Connection *pConnection);
    virtual ~ResourceCompilerInterfaceRemote();

    virtual BinaryBlob *CompileTexture(uint32 texturePlatform, const char *name) override;
    virtual BinaryBlob *CompileMaterialShader(const char *name) override;
    virtual BinaryBlob *CompileMaterial(const char *name) override;
    virtual BinaryBlob *CompileFont(uint32 texturePlatform, const char *name) override;
    virtual BinaryBlob *CompileBlockPalette(uint32 texturePlatform, const char *name) override;
    virtual BinaryBlob *CompileStaticMesh(const char *name) override;
    virtual BinaryBlob *CompileBlockMesh(const char *name) override;
    virtual BinaryBlob *CompileSkeleton(const char *name) override;
    virtual BinaryBlob *CompileSkeletalMesh(const char *name) override;
    virtual BinaryBlob *CompileSkeletalAnimation(const char *name) override;
    virtual BinaryBlob *CompileParticleSystem(const char *name) override;
    virtual BinaryBlob *CompileTerrainLayerList(const char *name) override;

    virtual bool CompileShader(const ShaderCompilerParameters *pParameters, ByteStream *pOutByteCodeStream, ByteStream *pOutInfoLogStream) override;

    static void RemoteProcessLoop();

private:
    Subprocess *m_pRemoteProcess;
    Subprocess::Connection *m_pRemoteConnection;
};

