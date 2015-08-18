#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompilerInterface/ResourceCompilerInterfaceRemote.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/MaterialShader.h"
#include "Engine/MaterialShader.h"
#include "Engine/Skeleton.h"
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Renderer/VertexFactoryTypeInfo.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "YBaseLib/BinaryWriteBuffer.h"
#include "YBaseLib/BinaryReadBuffer.h"
Log_SetChannel(ResourceCompilerInterfaceRemote);

static bool SendCommandAndPayload(Subprocess::Connection *pConnection, REMOTE_COMMAND command, const void *payload, uint32 payloadSize)
{
    REMOTE_COMMAND_HEADER hdr;
    hdr.Command = command;
    hdr.PayloadSize = payloadSize;
    if (pConnection->WriteData(&hdr, sizeof(hdr)) != sizeof(hdr))
        return false;

    if (payloadSize > 0 && pConnection->WriteData(payload, payloadSize) != payloadSize)
        return false;

    return true;
}

static bool WaitForCommandResult(Subprocess::Connection *pConnection, BinaryBlob **ppResultsBlob)
{
    for (;;)
    {
        REMOTE_COMMAND_HEADER hdr;
        if (pConnection->ReadDataBlocking(&hdr, sizeof(hdr)) != sizeof(hdr))
        {
            *ppResultsBlob = nullptr;
            return false;
        }

        switch (hdr.Command)
        {
        case REMOTE_COMMAND_SUCCESS:
        case REMOTE_COMMAND_FAILURE:
            {
                if (hdr.PayloadSize == 0)
                {
                    *ppResultsBlob = nullptr;
                }
                else
                {
                    BinaryBlob *pBlob = BinaryBlob::Allocate(hdr.PayloadSize);
                    if (pConnection->ReadDataBlocking(pBlob->GetDataPointer(), hdr.PayloadSize) != hdr.PayloadSize)
                    {
                        pBlob->Release();
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    *ppResultsBlob = pBlob;
                }

                return (hdr.Command == REMOTE_COMMAND_SUCCESS);
            }
            break;
        }
    }
}

struct RemoteProcessCallbacks : public ResourceCompilerCallbacks
{
    RemoteProcessCallbacks(Subprocess::Connection *pConnection) : m_pConnection(pConnection) {}

    virtual BinaryBlob *GetFileContents(const char *name) override
    {
        if (!SendCommandAndPayload(m_pConnection, REMOTE_COMMAND_GET_FILE_CONTENTS, name, Y_strlen(name)))
            return nullptr;

        BinaryBlob *pBlob;
        if (!WaitForCommandResult(m_pConnection, &pBlob) && pBlob != nullptr)
        {
            pBlob->Release();
            pBlob = nullptr;
        }
        return pBlob;
    }

    virtual const MaterialShader *GetCompiledMaterialShader(const char *name) override
    {
        if (!SendCommandAndPayload(m_pConnection, REMOTE_COMMAND_GET_COMPILED_MATERIAL_SHADER, name, Y_strlen(name)))
            return nullptr;

        BinaryBlob *pBlob;
        if (!WaitForCommandResult(m_pConnection, &pBlob))
        {
            if (pBlob != nullptr)
                pBlob->Release();

            return nullptr;
        }

        MaterialShader *pMaterialShader = new MaterialShader();
        ByteStream *pStream = pBlob->CreateReadOnlyStream();
        if (!pMaterialShader->Load(name, pStream))
        {
            Log_ErrorPrintf("RemoteProcessCallbacks::GetCompiledMaterialShader: Failed to load MaterialShader data provided by client (name: %s)", name);
            pStream->Release();
            pMaterialShader->Release();
            return nullptr;
        }

        pStream->Release();
        pBlob->Release();
        return pMaterialShader;
    }

    virtual const Skeleton *GetCompiledSkeleton(const char *name) override
    {
        if (!SendCommandAndPayload(m_pConnection, REMOTE_COMMAND_GET_COMPILED_SKELETON, name, Y_strlen(name)))
            return nullptr;

        BinaryBlob *pBlob;
        if (!WaitForCommandResult(m_pConnection, &pBlob))
        {
            if (pBlob != nullptr)
                pBlob->Release();

            return nullptr;
        }

        Skeleton *pSkeleton = new Skeleton();
        ByteStream *pStream = pBlob->CreateReadOnlyStream();
        if (!pSkeleton->LoadFromStream(name, pStream))
        {
            Log_ErrorPrintf("RemoteProcessCallbacks::GetCompiledSkeleton: Failed to load skeleton data provided by client (name: %s)", name);
            pStream->Release();
            pSkeleton->Release();
            return nullptr;
        }

        pStream->Release();
        pBlob->Release();
        return pSkeleton;
    }

private:
    Subprocess::Connection *m_pConnection;
};

static BinaryBlob *ProcessCompileFontCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    uint32 texturePlatform;
    if (!buffer.SafeReadUInt32(&texturePlatform) ||
        !buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile texture command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling font '%s' for platform '%s'", resourceName.GetCharArray(), NameTable_GetNameString(NameTables::TexturePlatform, texturePlatform));
    return ResourceCompiler::CompileFont(pCallbacks, (TEXTURE_PLATFORM)texturePlatform, resourceName);
}

static BinaryBlob *ProcessCompileTextureCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    uint32 texturePlatform;
    if (!buffer.SafeReadUInt32(&texturePlatform) ||
        !buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile texture command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling texture '%s' for platform '%s'", resourceName.GetCharArray(), NameTable_GetNameString(NameTables::TexturePlatform, texturePlatform));
    return ResourceCompiler::CompileTexture(pCallbacks, (TEXTURE_PLATFORM)texturePlatform, resourceName);
}

static BinaryBlob *ProcessCompileMaterialCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile material command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling material '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileMaterial(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileMaterialShaderCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile material shader command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling material shader '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileMaterialShader(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileStaticMeshCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile static mesh command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling static mesh '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileStaticMesh(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileSkeletonCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile skeleton command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling skeleton '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileSkeleton(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileSkeletalMeshCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile skeletal mesh command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling skeletal mesh '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileSkeletalMesh(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileSkeletalAnimationCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile skeletal animation command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling skeletal animation '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileSkeletalAnimation(pCallbacks, resourceName);
}

static BinaryBlob *ProcessCompileBlockPaletteCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    uint32 texturePlatform;
    if (!buffer.SafeReadUInt32(&texturePlatform) ||
        !buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile block palette command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling block palette '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileBlockPalette(pCallbacks, (TEXTURE_PLATFORM)texturePlatform, resourceName);
}

static BinaryBlob *ProcessCompileBlockMeshCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    String resourceName;
    if (!buffer.SafeReadSizePrefixedString(&resourceName))
    {
        Log_ErrorPrintf("Failed to parse compile block mesh command");
        return nullptr;
    }

    Log_InfoPrintf("Compiling block mesh '%s'", resourceName.GetCharArray());
    return ResourceCompiler::CompileBlockMesh(pCallbacks, resourceName);
}

static bool ProcessCompileShaderCommand(Subprocess::Connection *pConnection, RemoteProcessCallbacks *pCallbacks, BinaryReadBuffer &buffer)
{
    uint32 rendererPlatform;
    uint32 rendererFeatureLevel;
    uint32 compilerFlags;
    if (!buffer.SafeReadUInt32(&rendererPlatform) ||
        !buffer.SafeReadUInt32(&rendererFeatureLevel) ||
        !buffer.SafeReadUInt32(&compilerFlags))
    {
        Log_ErrorPrintf("Failed to parse compile shader command");
        return false;
    }

    ShaderCompilerParameters compilerParameters;
    compilerParameters.Platform = (RENDERER_PLATFORM)rendererPlatform;
    compilerParameters.FeatureLevel = (RENDERER_FEATURE_LEVEL)rendererFeatureLevel;
    compilerParameters.CompilerFlags = compilerFlags;

    // read stage info
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        if (!buffer.SafeReadSizePrefixedString(&compilerParameters.StageFileNames[i]) ||
            !buffer.SafeReadSizePrefixedString(&compilerParameters.StageEntryPoints[i]))
        {
            Log_ErrorPrintf("Failed to parse compile shader command");
            return false;
        }
    }

    uint32 macroCount;
    if (!buffer.SafeReadSizePrefixedString(&compilerParameters.VertexFactoryFileName) ||
        !buffer.SafeReadSizePrefixedString(&compilerParameters.MaterialShaderName) ||
        !buffer.SafeReadUInt32(&compilerParameters.MaterialShaderFlags) ||
        !buffer.SafeReadBool(&compilerParameters.EnableVerboseInfoLog) ||
        !buffer.SafeReadUInt32(&macroCount))
    {
        Log_ErrorPrintf("Failed to parse compile shader command");
        return false;
    }

    // read macros
    for (uint32 i = 0; i < macroCount; i++)
    {
        ShaderCompilerParameters::PreprocessorMacro macro;
        if (!buffer.SafeReadSizePrefixedString(&macro.Key) ||
            !buffer.SafeReadSizePrefixedString(&macro.Value))
        {
            Log_ErrorPrintf("Failed to parse compile shader command");
            return false;
        }

        Log_DevPrintf("  Preprocessor macro: %s %s", macro.Key.GetCharArray(), macro.Value.GetCharArray());
        compilerParameters.PreprocessorMacros.Add(macro);
    }

    // pass through to shader compiler
    GrowableMemoryByteStream *pCodeStream = ByteStream_CreateGrowableMemoryStream();
    GrowableMemoryByteStream *pInfoLogStream = (compilerParameters.EnableVerboseInfoLog) ? ByteStream_CreateGrowableMemoryStream() : nullptr;
    bool result = ResourceCompiler::CompileShader(pCallbacks, &compilerParameters, pCodeStream, pInfoLogStream);

    // still append the streams
    BinaryWriteBuffer outBuffer;
    outBuffer.WriteUInt32((uint32)pCodeStream->GetSize());
    outBuffer.WriteUInt32((pInfoLogStream != nullptr) ? (uint32)pInfoLogStream->GetSize() : 0);
    pCodeStream->SeekAbsolute(0);
    ByteStream_AppendStream(pCodeStream, outBuffer.GetStream());
    if (pInfoLogStream != nullptr)
    {
        pInfoLogStream->SeekAbsolute(0);
        ByteStream_AppendStream(pInfoLogStream, outBuffer.GetStream());
    }

    if (pInfoLogStream != nullptr)
        pInfoLogStream->Release();

    pCodeStream->Release();

    // write output
    return SendCommandAndPayload(pConnection, (result) ? REMOTE_COMMAND_SUCCESS : REMOTE_COMMAND_FAILURE, outBuffer.GetBufferPointer(), outBuffer.GetBufferSize());
}

void ResourceCompilerInterfaceRemote::RemoteProcessLoop()
{
    Log_DevPrintf("ResourceCompilerInterfaceRemote::RemoteProcessLoop: Connecting to parent...");
    Subprocess::Connection *pConnection = Subprocess::ConnectToParent();
    if (pConnection == nullptr)
    {
        Log_ErrorPrint("ResourceCompilerInterfaceRemote::RemoteProcessLoop: Failed to connect to parent.");
        return;
    }

    // Create callback interface
    RemoteProcessCallbacks callbacks(pConnection);
    bool exitingWithError = false;

    // Main loop
    while (!pConnection->IsExitRequested() && pConnection->IsOtherSideAlive())
    {
        REMOTE_COMMAND_HEADER hdr;
        if (pConnection->ReadDataBlocking(&hdr, sizeof(hdr)) != sizeof(hdr))
        {
            Log_ErrorPrintf("Failed to read command id");
            exitingWithError = true;
            break;
        }

        // Handle exit command
        Log_DevPrintf("Recv command %u size %u", hdr.Command, hdr.PayloadSize);
        if (hdr.Command == REMOTE_COMMAND_EXIT)
            break;
        
        // Create a buffer of the received payload
        BinaryReadBuffer buffer(hdr.PayloadSize);
        Log_DevPrintf("buf sz = %u, ptr = %p", buffer.GetBufferSize(), buffer.GetBufferPointer());
        if (pConnection->ReadDataBlocking(buffer.GetBufferPointer(), hdr.PayloadSize) != hdr.PayloadSize)
        {
            Log_ErrorPrintf("Failed to read remaining payload bytes");
            exitingWithError = true;
            break;
        }

        // Invoke correct method based on type.
        BinaryBlob *pReturnBlob = nullptr;
        switch (hdr.Command)
        {
        case REMOTE_COMMAND_COMPILE_FONT:
            pReturnBlob = ProcessCompileFontCommand(pConnection, &callbacks, buffer);
            break;
            
        case REMOTE_COMMAND_COMPILE_TEXTURE:
            pReturnBlob = ProcessCompileTextureCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_MATERIAL_SHADER:
            pReturnBlob = ProcessCompileMaterialShaderCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_MATERIAL:
            pReturnBlob = ProcessCompileMaterialCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_STATIC_MESH:
            pReturnBlob = ProcessCompileStaticMeshCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_SKELETON:
            pReturnBlob = ProcessCompileSkeletonCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_SKELETAL_MESH:
            pReturnBlob = ProcessCompileSkeletalMeshCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_SKELETAL_ANIMATION:
            pReturnBlob = ProcessCompileSkeletalAnimationCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_BLOCK_PALETTE:
            pReturnBlob = ProcessCompileBlockPaletteCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_BLOCK_MESH:
            pReturnBlob = ProcessCompileBlockMeshCommand(pConnection, &callbacks, buffer);
            break;

        case REMOTE_COMMAND_COMPILE_SHADER:
            {
                if (!ProcessCompileShaderCommand(pConnection, &callbacks, buffer))
                {
                    Log_ErrorPrintf("Failed to process compile shader command");
                    exitingWithError = true;
                    break;
                }
            }
            continue;
        }

        // Send resource back
        if (pReturnBlob != nullptr)
        {
            if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_SUCCESS, pReturnBlob->GetDataPointer(), pReturnBlob->GetDataSize()))
            {
                Log_ErrorPrintf("Failed to send success and data");
                pReturnBlob->Release();
                exitingWithError = true;
                break;
            }

            pReturnBlob->Release();
        }
        else
        {
            if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_FAILURE, nullptr, 0))
            {
                Log_ErrorPrintf("Failed to send failure code");
                exitingWithError = true;
                break;
            }
        }
    }

    if (exitingWithError)
        Thread::Sleep(2500);
}
