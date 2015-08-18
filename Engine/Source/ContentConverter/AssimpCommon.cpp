#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpCommon.h"
#include "Core/PixelFormat.h"

// Include the .lib
#if Y_PLATFORM_WINDOWS
    #if Y_BUILD_CONFIG_DEBUG
        #pragma comment(lib, "assimp-vc130-mtd.lib")
    #else
        #pragma comment(lib, "assimp-vc130-mt.lib")
    #endif
#endif      // Y_PLATFORM_WINDOWS

float4x4 AssimpHelpers::AssimpMatrix4x4ToFloat4x4(const aiMatrix4x4 &mat)
{
    float4x4 out(mat.a1, mat.a2, mat.a3, mat.a4,
                 mat.b1, mat.b2, mat.b3, mat.b4,
                 mat.c1, mat.c2, mat.c3, mat.c4,
                 mat.d1, mat.d2, mat.d3, mat.d4);

    return out;
}

Quaternion AssimpHelpers::AssimpQuaternionToQuaternion(const aiQuaternion &quat)
{
    Quaternion out(quat.x, quat.y, quat.z, quat.w);
    return out;
}

float2 AssimpHelpers::AssimpVector2ToFloat2(const aiVector2D &vec)
{
    return float2(vec.x, vec.y);
}

float3 AssimpHelpers::AssimpVector3ToFloat3(const aiVector3D &vec)
{
    return float3(vec.x, vec.y, vec.z);
}

uint32 AssimpHelpers::AssimpColor3ToColor(const aiColor3D &vec)
{
    uint32 r = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.r * 255.0f)), 0, 255);
    uint32 g = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.g * 255.0f)), 0, 255);
    uint32 b = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.b * 255.0f)), 0, 255);

    return MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, 255);
}

uint32 AssimpHelpers::AssimpColor4ToColor(const aiColor4D &vec)
{
    uint32 r = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.r * 255.0f)), 0, 255);
    uint32 g = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.g * 255.0f)), 0, 255);
    uint32 b = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.b * 255.0f)), 0, 255);
    uint32 a = (uint32)Math::Clamp(Math::Truncate(Math::Round(vec.a * 255.0f)), 0, 255);

    return MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a);
}

aiMatrix4x4 AssimpHelpers::Float4x4ToAssimpMatrix4x4(const float4x4 &mat)
{
    aiMatrix4x4 out(mat.m00, mat.m01, mat.m02, mat.m03,
                    mat.m10, mat.m11, mat.m12, mat.m13,
                    mat.m20, mat.m21, mat.m22, mat.m23,
                    mat.m30, mat.m31, mat.m32, mat.m33);

    return out;
}

static bool s_assimpInitialized = false;

static void DeinitializeAssimp()
{
    if (!s_assimpInitialized)
        return;

    Assimp::DefaultLogger::kill();
    s_assimpInitialized = false;
}

bool AssimpHelpers::InitializeAssimp()
{
    if (s_assimpInitialized)
        return true;

    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, 0, NULL);

    s_assimpInitialized = true;
    atexit(DeinitializeAssimp);
    return true;
}

int AssimpHelpers::GetAssimpStaticMeshImportPostProcessingFlags()
{
    // determine postprocess flags
    int postProcessingFlags = 0;

    // we want tangent vectors
    postProcessingFlags |= aiProcess_GenSmoothNormals;
    postProcessingFlags |= aiProcess_CalcTangentSpace;

    // join identical vertices
    postProcessingFlags |= aiProcess_JoinIdenticalVertices;

    // force triangles
    postProcessingFlags |= aiProcess_Triangulate;

    // improve cache locality
    postProcessingFlags |= aiProcess_ImproveCacheLocality;

    // remove unused materials
    postProcessingFlags |= aiProcess_RemoveRedundantMaterials;

    // improve overall scene
    postProcessingFlags |= aiProcess_OptimizeMeshes;
    postProcessingFlags |= aiProcess_OptimizeGraph;

    // use d3d convention for texcoords
    postProcessingFlags |= aiProcess_FlipUVs;

    // sort by prim type
    postProcessingFlags |= aiProcess_SortByPType;

    return postProcessingFlags;
}

int AssimpHelpers::GetAssimpSkeletonImportPostProcessingFlags()
{
    // determine postprocess flags
    int postProcessingFlags = 0;

    // improve overall scene
    postProcessingFlags |= aiProcess_OptimizeMeshes;
    postProcessingFlags |= aiProcess_OptimizeGraph;

    // minimum of 4 weights
    postProcessingFlags |= aiProcess_LimitBoneWeights;

    return postProcessingFlags;
}

int AssimpHelpers::GetAssimpSkeletalAnimationImportPostProcessingFlags()
{
    // determine postprocess flags
    int postProcessingFlags = 0;

    // improve overall scene
    postProcessingFlags |= aiProcess_OptimizeMeshes;
    postProcessingFlags |= aiProcess_OptimizeGraph;

    // minimum of 4 weights
    postProcessingFlags |= aiProcess_LimitBoneWeights;

    return postProcessingFlags;
}

int AssimpHelpers::GetAssimpSkeletalMeshImportPostProcessingFlags()
{
    // determine postprocess flags
    int postProcessingFlags = 0;

    // we want tangent vectors
    postProcessingFlags |= aiProcess_GenSmoothNormals;
    postProcessingFlags |= aiProcess_CalcTangentSpace;

    // join identical vertices
    postProcessingFlags |= aiProcess_JoinIdenticalVertices;

    // force triangles
    postProcessingFlags |= aiProcess_Triangulate;

    // improve cache locality
    postProcessingFlags |= aiProcess_ImproveCacheLocality;

    // remove unused materials
    postProcessingFlags |= aiProcess_RemoveRedundantMaterials;

    // improve overall scene
    postProcessingFlags |= aiProcess_OptimizeMeshes;
    postProcessingFlags |= aiProcess_OptimizeGraph;

    // use d3d convention for texcoords
    postProcessingFlags |= aiProcess_FlipUVs;

    // minimum of 4 weights
    postProcessingFlags |= aiProcess_LimitBoneWeights;

    return postProcessingFlags;
}

float4x4 AssimpHelpers::GetAssimpToWorldCoordinateSystemFloat4x4()
{
    return float4x4(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, -1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

aiMatrix4x4 AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4()
{
    return aiMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, -1.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f);
}

AssimpHelpers::ProgressCallbacksLogStream::ProgressCallbacksLogStream(ProgressCallbacks *pProgressCallbacks)
    : m_pProgressCallbacks(pProgressCallbacks)
{
    Assimp::DefaultLogger::get()->attachStream(this, Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Warn | Assimp::Logger::Err);
}

AssimpHelpers::ProgressCallbacksLogStream::~ProgressCallbacksLogStream()
{
    Assimp::DefaultLogger::get()->detatchStream(this, Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Warn | Assimp::Logger::Err);
}
/*
bool AssimpHelpers::ProgressCallbacksLogStream::attachStream(Assimp::LogStream *pStream, unsigned int severity)
{
    return false;
}

bool AssimpHelpers::ProgressCallbacksLogStream::detatchStream(Assimp::LogStream *pStream, unsigned int severity)
{
    return false;
}

void AssimpHelpers::ProgressCallbacksLogStream::OnDebug(const char* message)
{
    m_pProgressCallbacks->DisplayFormattedDebugMessage("[Assimp] %s", message);
}

void AssimpHelpers::ProgressCallbacksLogStream::OnInfo(const char* message)
{
    m_pProgressCallbacks->DisplayFormattedInformation("[Assimp] %s", message);
}

void AssimpHelpers::ProgressCallbacksLogStream::OnWarn(const char* essage)
{
    m_pProgressCallbacks->DisplayFormattedWarning("[Assimp] %s", essage);
}

void AssimpHelpers::ProgressCallbacksLogStream::OnError(const char* message)
{
    m_pProgressCallbacks->DisplayFormattedError("[Assimp] %s", message);
}
*/

void AssimpHelpers::ProgressCallbacksLogStream::write(const char* message)
{
    SmallString echoMessage;
    echoMessage.Format("[Assimp] %s", message);
    echoMessage.RStrip();

    m_pProgressCallbacks->DisplayDebugMessage(echoMessage);
}
