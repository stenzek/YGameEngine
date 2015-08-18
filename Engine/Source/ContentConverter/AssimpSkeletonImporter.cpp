#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpSkeletonImporter.h"
#include "ContentConverter/AssimpCommon.h"
#include "ResourceCompiler/SkeletonGenerator.h"

AssimpSkeletonImporter::AssimpSkeletonImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(NULL),
      m_pLogStream(NULL),
      m_pScene(NULL),
      m_pOutputGenerator(NULL)
{

}

AssimpSkeletonImporter::~AssimpSkeletonImporter()
{
    delete m_pOutputGenerator;
    delete m_pLogStream;
    delete m_pImporter;
}

void AssimpSkeletonImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
    pOptions->TransformMatrix.SetIdentity();
}

bool AssimpSkeletonImporter::Execute()
{
    Timer timer;

    if (m_pOptions->SourcePath.IsEmpty() ||
        m_pOptions->OutputResourceName.IsEmpty())
    {
        m_pProgressCallbacks->ModalError("Missing parameters");
        return false;
    }

    // ensure assimp is initialized
    if (!AssimpHelpers::InitializeAssimp())
    {
        m_pProgressCallbacks->ModalError("assimp initialization failed");
        return false;
    }

    timer.Reset();
    if (!LoadScene())
    {
        m_pProgressCallbacks->ModalError("LoadScene() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("LoadScene(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!CreateBones())
    {
        m_pProgressCallbacks->ModalError("CreateBones() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateBones(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!WriteOutput())
    {
        m_pProgressCallbacks->ModalError("WriteOutput() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("WriteOutput(): %.4f ms", timer.GetTimeMilliseconds());

    return true;
}

bool AssimpSkeletonImporter::LoadScene()
{
    // create importer
    m_pImporter = new Assimp::Importer();

    // hook up log interface
    m_pLogStream = new AssimpHelpers::ProgressCallbacksLogStream(m_pProgressCallbacks);

    // pass filename to assimp
    m_pScene = m_pImporter->ReadFile(m_pOptions->SourcePath, 0);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ReadFile failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // get post process flags
    uint32 postProcessFlags = AssimpHelpers::GetAssimpSkeletonImportPostProcessingFlags();

    // get coordinate system transform
    float4x4 coordinateSystemTransform(float4x4::MakeCoordinateSystemConversionMatrix(m_pOptions->CoordinateSystem, COORDINATE_SYSTEM_Z_UP_RH, nullptr));

    // create global transform
    m_globalTransform = m_pOptions->TransformMatrix * coordinateSystemTransform;
    m_inverseGlobalTransform = m_globalTransform.Inverse();

    // apply postprocessing
    m_pScene = m_pImporter->ApplyPostProcessing(postProcessFlags);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ApplyPostProcessing failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // display info
    m_pProgressCallbacks->DisplayFormattedInformation("Scene info: %u animations, %u camera, %u lights, %u materials, %u meshes, %u textures", m_pScene->mNumAnimations, m_pScene->mNumCameras, m_pScene->mNumLights, m_pScene->mNumMaterials, m_pScene->mNumMeshes, m_pScene->mNumTextures);
    
    // ok
    return true;
}

bool AssimpSkeletonImporter::CreateBones()
{
    // create generator
    m_pOutputGenerator = new SkeletonGenerator();

    // read the root bone
    if (m_pScene->mRootNode == NULL || !ReadBoneInfoFromNode(m_pScene->mRootNode, -1))
        return false;

    m_pProgressCallbacks->DisplayFormattedInformation("Created %u bones", m_pOutputGenerator->GetBoneCount());
    return true;
}

bool AssimpSkeletonImporter::ReadBoneInfoFromNode(const aiNode *pNode, int32 parentBoneIndex)
{
//     // fix coordinate system
//     aiMatrix4x4 CSTransform(AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4());
// 
//     // decompose the matrix to get the base frame transform from this node
//     aiVector3D baseFrameScaling;
//     aiQuaternion baseFrameRotation;
//     aiVector3D baseFramePosition;
//     (CSTransform * pNode->mTransformation).Decompose(baseFrameScaling, baseFrameRotation, baseFramePosition);

//     // for first bone, transform to our coordinate system
//     if (parentBoneIndex < 0)
//     {
//         aiMatrix4x4 tempMatrix(AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4() * pNode->mTransformation);
//         tempMatrix.Decompose(baseFrameScaling, baseFrameRotation, baseFramePosition);
//     }

    // decompose the matrix to get the base frame transform from this node
    aiVector3D baseFrameScaling;
    aiQuaternion baseFrameRotation;
    aiVector3D baseFramePosition;
    if (parentBoneIndex < 0)
    {
        //aiMatrix4x4 tempMatrix(AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4() * pNode->mTransformation);
        aiMatrix4x4 tempMatrix(AssimpHelpers::Float4x4ToAssimpMatrix4x4(m_globalTransform) * pNode->mTransformation);
        tempMatrix.Decompose(baseFrameScaling, baseFrameRotation, baseFramePosition);
    }
    else
    {
        pNode->mTransformation.Decompose(baseFrameScaling, baseFrameRotation, baseFramePosition);
    }

    // get bone name
    SmallString boneName;
    if (pNode->mName.length == 0)
        boneName.Format("__bone_%u__", m_pOutputGenerator->GetBoneCount());
    else
        boneName = pNode->mName.C_Str();

    // handle duplicate bone names
    if (m_pOutputGenerator->GetBoneByName(boneName) != nullptr)
    {
        boneName.Format("__bone_%u__", m_pOutputGenerator->GetBoneCount());
        m_pProgressCallbacks->DisplayFormattedWarning("duplicate bone name: '%s', changing to '%s'", pNode->mName.C_Str(), boneName.GetCharArray());
    }

    // create base frame transform
    Transform baseFrameTransform(AssimpHelpers::AssimpVector3ToFloat3(baseFramePosition),
                                 AssimpHelpers::AssimpQuaternionToQuaternion(baseFrameRotation),
                                 AssimpHelpers::AssimpVector3ToFloat3(baseFrameScaling));

    // find parent bone in output hierarchy
    SkeletonGenerator::Bone *pParentBone = (parentBoneIndex >= 0) ? m_pOutputGenerator->GetBoneByIndex((uint32)parentBoneIndex) : NULL;

    // create bone
    SkeletonGenerator::Bone *pBone = m_pOutputGenerator->CreateBone(boneName, pParentBone, baseFrameTransform);
    if (pBone == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("could not create bone '%s'", boneName.GetCharArray());
        return false;
    }

    // dump node
    m_pProgressCallbacks->DisplayFormattedDebugMessage("new bone '%s' (parent '%s') at %s",
                                                       pBone->GetName().GetCharArray(),
                                                       (pBone->GetParent() != NULL) ? pBone->GetParent()->GetName().GetCharArray() : "NULL",
                                                       StringConverter::Float3ToString(pBone->GetBaseFrameTransform().GetPosition()).GetCharArray());

    // read children
    for (uint32 i = 0; i < pNode->mNumChildren; i++)
    {
        if (!ReadBoneInfoFromNode(pNode->mChildren[i], (int32)pBone->GetIndex()))
            return false;
    }

    // ok
    return true;
}

bool AssimpSkeletonImporter::WriteOutput()
{
    PathString outputResourceFileName;
    outputResourceFileName.Format("%s.skl.xml", m_pOptions->OutputResourceName.GetCharArray());

    ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputResourceFileName,
                                                         BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | 
                                                         BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | 
                                                         BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("could not open file '%s'", outputResourceFileName.GetCharArray());
        return false;
    }

    if (!m_pOutputGenerator->SaveToXML(pStream))
    {
        m_pProgressCallbacks->DisplayError("failed to save xml file");
        pStream->Discard();
        pStream->Release();
        return false;
    }

    pStream->Commit();
    pStream->Release();
    return true;
}

