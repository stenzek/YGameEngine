#include "Engine/PrecompiledHeader.h"
#include "Engine/Skeleton.h"
#include "Engine/DataFormats.h"
Log_SetChannel(Skeleton);

DEFINE_RESOURCE_TYPE_INFO(Skeleton);
DEFINE_RESOURCE_GENERIC_FACTORY(Skeleton);

Skeleton::Skeleton(const ResourceTypeInfo *pResourceTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pResourceTypeInfo)
{

}

Skeleton::~Skeleton()
{

}

const Skeleton::Bone *Skeleton::GetBoneByName(const char *boneName) const
{
    for (uint32 i = 0; i < m_bones.GetSize(); i++)
    {
        if (m_bones[i].m_name.Compare(boneName))
            return &m_bones[i];
    }

    return nullptr;
}

bool Skeleton::LoadFromStream(const char *name, ByteStream *pStream)
{
    DF_SKELETON_HEADER fileHeader;
    if (!pStream->Read2(&fileHeader, sizeof(fileHeader)))
        return false;

    if (fileHeader.Magic != DF_SKELETON_HEADER_MAGIC || fileHeader.HeaderSize != sizeof(fileHeader))
        return false;

    // has to have bones
    if (fileHeader.BoneCount == 0)
    {
        Log_ErrorPrintf("Skeleton::LoadFromStream: Skeleton '%s' has no bones", name);
        return false;
    }

    // allocate everything
    m_strName = name;
    m_bones.Resize(fileHeader.BoneCount);

    // read bones
    {
        if (!pStream->SeekAbsolute(fileHeader.BonesOffset))
            return false;

        // next bone offset
        uint64 nextBoneOffset = pStream->GetPosition();
        uint32 *pChildBones = (uint32 *)alloca(sizeof(uint32) * fileHeader.BoneCount);

        // read in each bone
        for (uint32 boneIndex = 0; boneIndex < m_bones.GetSize(); boneIndex++)
        {
            // seek to next bone
            if (!pStream->SeekAbsolute(nextBoneOffset))
                return false;

            // read in bone hader
            DF_SKELETON_BONE boneHeader;
            if (!pStream->Read2(&boneHeader, sizeof(boneHeader)) || boneHeader.BoneNameLength == 0)
                return false;

            // init bone structure
            Bone *pDestinationBone = &m_bones[boneIndex];
            pDestinationBone->m_index = boneIndex;
            pDestinationBone->m_pParentBone = (boneHeader.ParentBoneIndex != 0xFFFFFFFF) ? &m_bones[boneHeader.ParentBoneIndex] : nullptr;
            pDestinationBone->m_relativeBaseFrameTransform.SetPosition(float3(boneHeader.RelativeBaseFrameTransformPosition));
            pDestinationBone->m_relativeBaseFrameTransform.SetRotation(Quaternion(float4(boneHeader.RelativeBaseFrameTransformRotation)));
            pDestinationBone->m_relativeBaseFrameTransform.SetScale(float3(boneHeader.RelativeBaseFrameTransformScale));
            pDestinationBone->m_absoluteBaseFrameTransform.SetPosition(float3(boneHeader.AbsoluteBaseFrameTransformPosition));
            pDestinationBone->m_absoluteBaseFrameTransform.SetRotation(Quaternion(float4(boneHeader.AbsoluteBaseFrameTransformRotation)));
            pDestinationBone->m_absoluteBaseFrameTransform.SetScale(float3(boneHeader.AbsoluteBaseFrameTransformScale));

            // read in the name
            String boneName;
            boneName.Resize(boneHeader.BoneNameLength);
            if (!pStream->Read2(boneName.GetWriteableCharArray(), boneHeader.BoneNameLength))
                return false;

            // set name
            pDestinationBone->m_name = boneName;

            // initialize child bones
            if (boneHeader.ChildBoneCount > 0)
            {
                // read in child bone indices
                DebugAssert(boneHeader.ChildBoneCount < fileHeader.BoneCount);
                if (!pStream->Read2(pChildBones, sizeof(uint32) * boneHeader.ChildBoneCount))
                    return false;

                // process them
                pDestinationBone->m_childBones.Resize(boneHeader.ChildBoneCount);
                for (uint32 childBoneIndex = 0; childBoneIndex < pDestinationBone->m_childBones.GetSize(); childBoneIndex++)
                {
                    DebugAssert(pChildBones[childBoneIndex] < m_bones.GetSize());
                    pDestinationBone->m_childBones[childBoneIndex] = &m_bones[pChildBones[childBoneIndex]];
                }
            }

            // figure out next bone offset
            nextBoneOffset += boneHeader.BoneSize;
        }
    }

    return true;
}

