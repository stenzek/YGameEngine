#include "ResourceCompilerInterface/ResourceCompilerInterfaceRemote.h"
#include "Engine/MaterialShader.h"
#include "Engine/Skeleton.h"
#include "Engine/ResourceManager.h"
#include "Engine/MaterialShader.h"
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Renderer/VertexFactoryTypeInfo.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "YBaseLib/BinaryWriteBuffer.h"
#include "YBaseLib/BinaryReadBuffer.h"
Log_SetChannel(ResourceCompilerInterfaceRemote);

#if defined(Y_PLATFORM_WINDOWS)
    #if defined(Y_BUILD_CONFIG_DEBUG)
        static const char *RESOURCE_COMPILER_EXECUTABLE_NAME = "ResourceCompiler-Debug.exe";
    #elif defined(Y_BUILD_CONFIG_DEBUGFAST)
        static const char *RESOURCE_COMPILER_EXECUTABLE_NAME = "ResourceCompiler-DebugFast.exe";
    #elif defined(Y_BUILD_CONFIG_RELEASE)
        static const char *RESOURCE_COMPILER_EXECUTABLE_NAME = "ResourceCompiler-Release.exe";
    #elif defined(Y_BUILD_CONFIG_SHIPPING)
        static const char *RESOURCE_COMPILER_EXECUTABLE_NAME = "ResourceCompiler-Shipping.exe";
    #else
        #error Unknown build config.
    #endif
#elif defined(Y_PLATFORM_LINUX)
    static const char *RESOURCE_COMPILER_EXECUTABLE_NAME = "ResourceCompiler";
#else
    #error Unknown platform
#endif


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
        case REMOTE_COMMAND_GET_FILE_CONTENTS:
            {
                // Read string parameter
                String fileName;
                fileName.Resize(hdr.PayloadSize);
                if (pConnection->ReadDataBlocking(fileName.GetWriteableCharArray(), hdr.PayloadSize) != hdr.PayloadSize)
                {
                    *ppResultsBlob = nullptr;
                    return false;
                }

                // Load file
                AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
                if (pStream == nullptr)
                {
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_FAILURE, nullptr, 0))
                    {
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    continue;
                }

                // Copy it into memory
                AutoReleasePtr<BinaryBlob> pBlob = BinaryBlob::CreateFromStream(pStream);
                if (pBlob == nullptr)
                {
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_FAILURE, nullptr, 0))
                    {
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    continue;
                }

                // Send to the other side
                if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_SUCCESS, pBlob->GetDataPointer(), pBlob->GetDataSize()))
                {
                    *ppResultsBlob = nullptr;
                    return false;
                }
                
                // Wait for next command
                continue;
            }
            break;

        case REMOTE_COMMAND_GET_COMPILED_MATERIAL_SHADER:
            {
                // Read string parameter
                String materialShaderName;
                materialShaderName.Resize(hdr.PayloadSize);
                if (pConnection->ReadDataBlocking(materialShaderName.GetWriteableCharArray(), hdr.PayloadSize) != hdr.PayloadSize)
                {
                    *ppResultsBlob = nullptr;
                    return false;
                }

                // force a load of it, this'll ensure the version on-disk is up-to-date.
                AutoReleasePtr<const MaterialShader> pMaterialShader = g_pResourceManager->UncachedGetMaterialShader(materialShaderName);
                if (pMaterialShader == nullptr)
                {
                    Log_WarningPrintf("WaitForCommandResult: Remote requested unavailable material shader '%s'", materialShaderName.GetCharArray());
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_FAILURE, nullptr, 0))
                    {
                        *ppResultsBlob = nullptr;
                        return false;
                    }
                }
                else
                {
                    // try searching for a compiled version on-disk
                    String fileName;
                    fileName.Format("%s.msh", materialShaderName.GetCharArray());
                    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
                    BinaryBlob *pCompiledBlob;
                    if (pStream == nullptr || (pCompiledBlob = BinaryBlob::CreateFromStream(pStream)) == nullptr)
                    {
                        Log_WarningPrintf("WaitForCommandResult: Remote requested material shader '%s', which loaded, but could not locate the compiled version.", materialShaderName.GetCharArray());
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    // send this version back
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_SUCCESS, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize()))
                    {
                        pCompiledBlob->Release();
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    pCompiledBlob->Release();
                }

                // wait for next command
                continue;
            }
            break;

        case REMOTE_COMMAND_GET_COMPILED_SKELETON:
            {
                // Read string parameter
                String skeletonName;
                skeletonName.Resize(hdr.PayloadSize);
                if (pConnection->ReadDataBlocking(skeletonName.GetWriteableCharArray(), hdr.PayloadSize) != hdr.PayloadSize)
                {
                    *ppResultsBlob = nullptr;
                    return false;
                }

                // force a load of it, this'll ensure the version on-disk is up-to-date.
                AutoReleasePtr<const Skeleton> pSkeleton = g_pResourceManager->UncachedGetSkeleton(skeletonName);
                if (pSkeleton == nullptr)
                {
                    Log_WarningPrintf("WaitForCommandResult: Remote requested unavailable skeleton '%s'", skeletonName.GetCharArray());
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_FAILURE, nullptr, 0))
                    {
                        *ppResultsBlob = nullptr;
                        return false;
                    }
                }
                else
                {
                    // try searching for a compiled version on-disk
                    String fileName;
                    fileName.Format("%s.skl", skeletonName.GetCharArray());
                    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
                    BinaryBlob *pCompiledBlob;
                    if (pStream == nullptr || (pCompiledBlob = BinaryBlob::CreateFromStream(pStream)) == nullptr)
                    {
                        Log_WarningPrintf("WaitForCommandResult: Remote requested skeleton '%s', which loaded, but could not locate the compiled version.", skeletonName.GetCharArray());
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    // send this version back
                    if (!SendCommandAndPayload(pConnection, REMOTE_COMMAND_SUCCESS, pCompiledBlob->GetDataPointer(), pCompiledBlob->GetDataSize()))
                    {
                        pCompiledBlob->Release();
                        *ppResultsBlob = nullptr;
                        return false;
                    }

                    pCompiledBlob->Release();
                }

                // wait for next command
                continue;
            }
            break;

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

ResourceCompilerInterfaceRemote::ResourceCompilerInterfaceRemote(Subprocess *pSubProcess, Subprocess::Connection *pConnection)
    : m_pRemoteProcess(pSubProcess), m_pRemoteConnection(pConnection)
{

}

ResourceCompilerInterfaceRemote::~ResourceCompilerInterfaceRemote()
{
    m_pRemoteProcess->RequestChildToExit();
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_EXIT, nullptr, 0))
        m_pRemoteProcess->TerminateChild();

    m_pRemoteProcess->WaitForChildToExit();
    delete m_pRemoteProcess;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileTexture(uint32 texturePlatform, const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteUInt32(texturePlatform);
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_TEXTURE, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileMaterialShader(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_MATERIAL_SHADER, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileMaterial(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_MATERIAL, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileFont(uint32 texturePlatform, const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteUInt32(texturePlatform);    
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_FONT, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileBlockPalette(uint32 texturePlatform, const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteUInt32(texturePlatform);
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_BLOCK_PALETTE, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileStaticMesh(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_STATIC_MESH, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileBlockMesh(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_BLOCK_MESH, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileSkeleton(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_SKELETON, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileSkeletalMesh(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_SKELETAL_MESH, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileSkeletalAnimation(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_SKELETAL_ANIMATION, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileParticleSystem(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_PARTICLE_SYSTEM, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

BinaryBlob *ResourceCompilerInterfaceRemote::CompileTerrainLayerList(const char *name)
{
    BinaryWriteBuffer buffer;
    buffer.WriteSizePrefixedString(name);
    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_TERRAIN_LAYER_LIST, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return nullptr;

    BinaryBlob *pResultBlob;
    if (!WaitForCommandResult(m_pRemoteConnection, &pResultBlob) && pResultBlob != nullptr)
    {
        pResultBlob->Release();
        pResultBlob = nullptr;
    }
    return pResultBlob;
}

bool ResourceCompilerInterfaceRemote::CompileShader(const ShaderCompilerParameters *pParameters, ByteStream *pOutByteCodeStream, ByteStream *pOutInfoLogStream)
{
    BinaryWriteBuffer buffer;
    buffer.WriteUInt32(pParameters->Platform);
    buffer.WriteUInt32(pParameters->FeatureLevel);
    buffer.WriteUInt32(pParameters->CompilerFlags);
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        buffer.WriteSizePrefixedString(pParameters->StageFileNames[i]);
        buffer.WriteSizePrefixedString(pParameters->StageEntryPoints[i]);
    }
    buffer.WriteSizePrefixedString(pParameters->VertexFactoryFileName);
    buffer.WriteSizePrefixedString(pParameters->MaterialShaderName);
    buffer.WriteUInt32(pParameters->MaterialShaderFlags);
    buffer.WriteBool(pParameters->EnableVerboseInfoLog);
    buffer.WriteUInt32(pParameters->PreprocessorMacros.GetSize());
    for (uint32 i = 0; i < pParameters->PreprocessorMacros.GetSize(); i++)
    {
        buffer.WriteSizePrefixedString(pParameters->PreprocessorMacros[i].Key);
        buffer.WriteSizePrefixedString(pParameters->PreprocessorMacros[i].Value);
    }

    if (!SendCommandAndPayload(m_pRemoteConnection, REMOTE_COMMAND_COMPILE_SHADER, buffer.GetBufferPointer(), buffer.GetBufferSize()))
        return false;

    BinaryBlob *pResultBlob;
    bool result = WaitForCommandResult(m_pRemoteConnection, &pResultBlob);

    // any results?
    if (pResultBlob != nullptr)
    {
        // copy from blob to streams
        ByteStream *pStream = pResultBlob->CreateReadOnlyStream();
        BinaryReader binaryReader(pStream);
        uint32 byteCodeLength, infoLogLength;
        if (binaryReader.SafeReadUInt32(&byteCodeLength) && binaryReader.SafeReadUInt32(&infoLogLength))
        {
            if (ByteStream_CopyBytes(pStream, byteCodeLength, pOutByteCodeStream) == byteCodeLength)
                result &= (ByteStream_CopyBytes(pStream, infoLogLength, pOutInfoLogStream) == infoLogLength);
            else
                result = false;
        }
        else
        {
            result = false;
        }

        pStream->Release();
        pResultBlob->Release();
    }

    return result;
}

ResourceCompilerInterface *ResourceCompilerInterface::CreateRemoteInterface()
{
    Log_DevPrintf("ResourceCompilerInterface::CreateRemoteInterface: Spawning ResourceCompiler process.");

    SmallString programFileName;
    SmallString resourceCompilerFileName;
    Platform::GetProgramFileName(programFileName);
    FileSystem::BuildPathRelativeToFile(resourceCompilerFileName, programFileName, RESOURCE_COMPILER_EXECUTABLE_NAME, true, true);

    Subprocess *pRemoteProcess = Subprocess::Create(resourceCompilerFileName, "REMOTE_COMPILER", true, true, true);
    if (pRemoteProcess == nullptr)
    {
        Log_ErrorPrintf("ResourceCompilerInterface::CreateRemoteInterface: Failed to create ResourceCompiler process (path: %s)", resourceCompilerFileName.GetCharArray());
        return nullptr;
    }

    Subprocess::Connection *pConnection = pRemoteProcess->ConnectToChild();
    if (pConnection == nullptr)
    {
        Log_ErrorPrint("ResourceCompilerInterface::CreateRemoteInterface: Failed to connect to ResourceCompiler.");
        pRemoteProcess->TerminateChild();
        delete pRemoteProcess;
        return nullptr;
    }

    ResourceCompilerInterfaceRemote *pInterface = new ResourceCompilerInterfaceRemote(pRemoteProcess, pConnection);
    return pInterface;
}
