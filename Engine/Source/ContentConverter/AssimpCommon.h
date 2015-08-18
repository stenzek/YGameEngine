#pragma once
#include "ContentConverter/Common.h"
#include "YBaseLib/ProgressCallbacks.h"
#include "assimp/Importer.hpp"
#include "assimp/Logger.hpp"
#include "assimp/LogStream.hpp"
#include "assimp/DefaultLogger.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/material.h"

namespace AssimpHelpers {

float4x4 AssimpMatrix4x4ToFloat4x4(const aiMatrix4x4 &mat);
Quaternion AssimpQuaternionToQuaternion(const aiQuaternion &quat);
float2 AssimpVector2ToFloat2(const aiVector2D &vec);
float3 AssimpVector3ToFloat3(const aiVector3D &vec);
uint32 AssimpColor3ToColor(const aiColor3D &vec);
uint32 AssimpColor4ToColor(const aiColor4D &vec);

aiMatrix4x4 Float4x4ToAssimpMatrix4x4(const float4x4 &mat);

class ProgressCallbacksLogStream : public Assimp::LogStream
{
public:
    ProgressCallbacksLogStream(ProgressCallbacks *pProgressCallbacks);
    virtual ~ProgressCallbacksLogStream();

    virtual void write(const char* message);    

private:
    ProgressCallbacks *m_pProgressCallbacks;
};

bool InitializeAssimp();

int GetAssimpStaticMeshImportPostProcessingFlags();
int GetAssimpSkeletonImportPostProcessingFlags();
int GetAssimpSkeletalAnimationImportPostProcessingFlags();
int GetAssimpSkeletalMeshImportPostProcessingFlags();

float4x4 GetAssimpToWorldCoordinateSystemFloat4x4();
aiMatrix4x4 GetAssimpToWorldCoordinateSystemAIMatrix4x4();

}
