#include "Engine/PrecompiledHeader.h"
#include "Engine/SkeletalAnimation.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
Log_SetChannel(SkeletalAnimation);

DEFINE_RESOURCE_TYPE_INFO(SkeletalAnimation);
DEFINE_RESOURCE_GENERIC_FACTORY(SkeletalAnimation);

SkeletalAnimation::SkeletalAnimation(const ResourceTypeInfo *pResourceTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pResourceTypeInfo),
      m_pBoneTracks(nullptr),
      m_ppBoneIndexToBoneTrack(nullptr),
      m_boneTrackCount(0),
      m_pRootMotionTrack(nullptr)
{

}

SkeletalAnimation::~SkeletalAnimation()
{
    delete m_pRootMotionTrack;
    delete[] m_ppBoneIndexToBoneTrack;
    delete[] m_pBoneTracks;
}

void SkeletalAnimation::TransformTrack::GetKeyFrames(float time, const KeyFrame **ppStartKeyFrame, const KeyFrame **ppEndKeyFrame, float *pFactor) const
{
    DebugAssert(time >= 0.0f);
    
    // only one key?
    if (m_keyFrames.GetSize() == 1)
    {
        *ppStartKeyFrame = &m_keyFrames[0];
        *ppEndKeyFrame = nullptr;
        *pFactor = 0.0f;
        return;
    }

    // find the key that we reside in with a time of <= t
    uint32 keyIndex;
    for (keyIndex = 0; keyIndex < (m_keyFrames.GetSize() - 1); keyIndex++)
    {
        if (time < m_keyFrames[keyIndex + 1].Key)
            break;
    }

    // is this the last key?
    uint32 nextKeyIndex = keyIndex + 1;
    if (nextKeyIndex == m_keyFrames.GetSize())
    {
        // use the last key
        *ppStartKeyFrame = &m_keyFrames[keyIndex];
        *ppEndKeyFrame = nullptr;
        *pFactor = 0.0f;
        return;
    }

    // calculate factor
    const float thisKeyTime = m_keyFrames[keyIndex].Key;
    const float nextKeyTime = m_keyFrames[nextKeyIndex].Key;
    const float factor = (time - thisKeyTime) / (nextKeyTime - thisKeyTime);
    DebugAssert(factor >= 0.0f && factor <= 1.0f);

    // store values
    *ppStartKeyFrame = &m_keyFrames[keyIndex];
    *ppEndKeyFrame = &m_keyFrames[nextKeyIndex];
    *pFactor = factor;
}

void SkeletalAnimation::TransformTrack::GetBoneTransform(float time, Transform *pTransform, bool interpolate /* = true */) const
{
    // find the keyframes for this time spec
    const KeyFrame *startKeyFrame;
    const KeyFrame *endKeyFrame;
    float factor;
    GetKeyFrames(time, &startKeyFrame, &endKeyFrame, &factor);

    // do we have to interpolate
    if (endKeyFrame != nullptr)
    {
        // handle cases where it is very close to zero/one
        if (Math::NearEqual(factor, 0.0f, Y_FLT_EPSILON))
        {
            // use the start keyframe
            *pTransform = startKeyFrame->Value;
        }
        else if (Math::NearEqual(factor, 1.0f, Y_FLT_EPSILON))
        {
            // use the end keyframe
            *pTransform = endKeyFrame->Value;
        }
        else
        {
            // allow interpolation?
            if (interpolate)
            {
                // linear interpolate between the two
                *pTransform = Transform::LinearInterpolate(startKeyFrame->Value, endKeyFrame->Value, factor);
            }
            else
            {
                // if factor >= 0.5, use end, otherwise start
                if (factor < 0.5f)
                    *pTransform = startKeyFrame->Value;
                else
                    *pTransform = endKeyFrame->Value;
            }
        }
    }
    else
    {
        // only have one keyframe reference
        DebugAssert(startKeyFrame != nullptr);
        *pTransform = startKeyFrame->Value;
    }
}

bool SkeletalAnimation::CalculateRelativeBoneTransform(uint32 boneIndex, float time, Transform *pTransform, bool interpolate /* = true */) const
{
    DebugAssert(boneIndex < m_pSkeleton->GetBoneCount());

    // find the track
    const TransformTrack *pBoneTrack = m_ppBoneIndexToBoneTrack[boneIndex];
    if (pBoneTrack != nullptr)
    {
        // get the transform for the specified time
        pBoneTrack->GetBoneTransform(time, pTransform, interpolate);
        return true;
    }
    else
    {
        // we have no track for this bone
        return false;
    }
}

void SkeletalAnimation::CalculateAbsoluteBoneTransform(uint32 boneIndex, float time, Transform *pTransform, bool interpolate /* = true */) const
{
    DebugAssert(boneIndex < m_pSkeleton->GetBoneCount());

    // bone has parent?
    const Skeleton::Bone *pBone = m_pSkeleton->GetBoneByIndex(boneIndex);
    if (pBone->GetParentBone() == nullptr)
    {
        // no parent, this must be the root, so just return the relative transform of the root
        if (!CalculateRelativeBoneTransform(boneIndex, time, pTransform, interpolate))
            *pTransform = m_pSkeleton->GetBoneByIndex(boneIndex)->GetRelativeBaseFrameTransform();
    }
    else
    {
        // get the parent's absolute transform
        Transform parentTransform;
        CalculateAbsoluteBoneTransform(pBone->GetParentBone()->GetIndex(), time, &parentTransform, interpolate);

        // and this bone's relative transform
        Transform thisBoneTransform;
        if (!CalculateRelativeBoneTransform(boneIndex, time, &thisBoneTransform, interpolate))
            *pTransform = m_pSkeleton->GetBoneByIndex(boneIndex)->GetRelativeBaseFrameTransform();

        // apply parent after bone
        *pTransform = Transform::ConcatenateTransforms(thisBoneTransform, parentTransform);
    }
}

bool SkeletalAnimation::LoadFromStream(const char *name, ByteStream *pStream)
{
    DF_SKELETALANIMATION_HEADER fileHeader;
    if (!pStream->Read2(&fileHeader, sizeof(fileHeader)))
    {
        Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Could not read header");
        return false;
    }

    // verify header
    if (fileHeader.Magic != DF_SKELETALANIMATION_HEADER_MAGIC ||
        fileHeader.HeaderSize != sizeof(fileHeader))
    {
        Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Invalid header");
        return false;
    }

    // read skeleton name
    {
        String skeletonName;
        skeletonName.Resize(fileHeader.SkeletonNameLength);
        if (!pStream->SeekAbsolute(fileHeader.SkeletonNameOffset) || !pStream->Read2(skeletonName.GetWriteableCharArray(), fileHeader.SkeletonNameLength))
        {
            Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Invalid skeleton name");
            return false;
        }

        // load skeleton
        if ((m_pSkeleton = g_pResourceManager->GetSkeleton(skeletonName)) == nullptr)
        {
            Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Could not load skeleton '%s'", skeletonName.GetCharArray());
            return false;
        }
    }

    // verify skeleton matches
    if (m_pSkeleton->GetBoneCount() != fileHeader.SkeletonBoneCount)
    {
        Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Bone count in animation does not match skeleton (%u / %u)", fileHeader.SkeletonBoneCount, m_pSkeleton->GetBoneCount());
        return false;
    }

    // allocate everything
    m_duration = fileHeader.Duration;
    m_strName = name;

    // read in bone tracks
    if (fileHeader.BoneTrackCount > 0)
    {
        // seek to start of bone track data
        if (!pStream->SeekAbsolute(fileHeader.BoneTrackOffset))
        {
            Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Failed to seek to bone track offset.");
            return false;
        }

        m_pBoneTracks = new BoneTrack[fileHeader.BoneTrackCount];
        m_boneTrackCount = fileHeader.BoneTrackCount;

        for (uint32 i = 0; i < m_boneTrackCount; i++)
        {
            if (!m_pBoneTracks[i].LoadFromStream(pStream, m_pSkeleton))
            {
                Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Failed to load bone track %u.", i);
                return false;
            }
        }

        // bind bone tracks to bones
        uint32 skeletonBoneCount = m_pSkeleton->GetBoneCount();
        m_ppBoneIndexToBoneTrack = new const BoneTrack *[skeletonBoneCount];
        Y_memzero(m_ppBoneIndexToBoneTrack, sizeof(const BoneTrack *) * skeletonBoneCount);
        for (uint32 i = 0; i < m_boneTrackCount; i++)
        {
            uint32 boneIndex = m_pBoneTracks[i].GetBoneIndex();
            if (boneIndex >= skeletonBoneCount || m_ppBoneIndexToBoneTrack[boneIndex] != nullptr)
            {
                Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Duplicate or invalid bone index %u in track %u", boneIndex, i);
                return false;
            }

            m_ppBoneIndexToBoneTrack[boneIndex] = &m_pBoneTracks[i];
        }
    }

    // read in root motion track
    if (fileHeader.RootMotionOffset != 0)
    {
        // seek to start of track data
        if (!pStream->SeekAbsolute(fileHeader.RootMotionOffset))
        {
            Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Failed to seek to root motion track offset.");
            return false;
        }

        m_pRootMotionTrack = new RootMotionTrack();
        if (!m_pRootMotionTrack->LoadFromStream(pStream, m_pSkeleton))
        {
            Log_ErrorPrintf("SkeletalAnimation::LoadFromStream: Failed to load root motion track.");
            return false;
        }
    }

    // done
    return true;
}

bool SkeletalAnimation::TransformTrack::LoadKeyFramesFromStream(ByteStream *pStream, float duration, uint32 keyFrameCount)
{
    DebugAssert(duration >= 0.0f && keyFrameCount > 0);
    m_duration = duration;
    m_keyFrames.Resize(keyFrameCount);

    // read in keyframes
    const uint32 READ_KEYFRAME_COUNT = 16;
    DF_SKELETALANIMATION_TRANSFORM_TRACK_KEYFRAME readKeyframes[READ_KEYFRAME_COUNT];
    for (uint32 i = 0; i < keyFrameCount; i += READ_KEYFRAME_COUNT)
    {
        uint32 readCount = Min(READ_KEYFRAME_COUNT, keyFrameCount - i);
        if (!pStream->Read2(readKeyframes, sizeof(DF_SKELETALANIMATION_TRANSFORM_TRACK_KEYFRAME) * readCount))
            return false;

        for (uint32 j = 0; j < readCount; j++)
        {
            DF_SKELETALANIMATION_TRANSFORM_TRACK_KEYFRAME &rkf = readKeyframes[j];
            KeyFrame &kf = m_keyFrames[i + j];
            kf.Key = rkf.Time;
            kf.Value = Transform(float3(rkf.Position), Quaternion(rkf.Rotation[0], rkf.Rotation[1], rkf.Rotation[2], rkf.Rotation[3]), float3(rkf.Scale));
        }
    }

    return true;
}

bool SkeletalAnimation::BoneTrack::LoadFromStream(ByteStream *pStream, const Skeleton *pSkeleton)
{
    // read in header
    DF_SKELETALANIMATION_BONE_TRACK_HEADER header;
    if (!pStream->Read2(&header, sizeof(header)))
        return false;

    // validate bone index
    if (header.BoneIndex >= pSkeleton->GetBoneCount())
    {
        Log_ErrorPrintf("SkeletalAnimation::BoneTrack::LoadFromStream: Bone track contains invalid bone index: %u", header.BoneIndex);
        return false;
    }

    // set values
    m_boneIndex = header.BoneIndex;
    
    // read keyframes
    if (!LoadKeyFramesFromStream(pStream, header.Duration, header.KeyFrameCount))
        return false;

    // done
    return true;
}

bool SkeletalAnimation::RootMotionTrack::LoadFromStream(ByteStream *pStream, const Skeleton *pSkeleton)
{
    // read in header
    DF_SKELETALANIMATION_ROOT_MOTION_TRACK_HEADER header;
    if (!pStream->Read2(&header, sizeof(header)))
        return false;

    // read keyframes
    if (!LoadKeyFramesFromStream(pStream, header.Duration, header.KeyFrameCount))
        return false;

    // done
    return true;
}
