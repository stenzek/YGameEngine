#pragma once
#include "Engine/Common.h"
#include "Engine/Skeleton.h"

class SkeletalAnimation : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(SkeletalAnimation, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(SkeletalAnimation);

public:
    class TransformTrack
    {
    public:
        typedef KeyValuePair<float, Transform> KeyFrame;

    public:
        const float GetDuration() const { return m_duration; }

        // look up the keyframes for a specified time
        void GetKeyFrames(float time, const KeyFrame **ppStartKeyFrame, const KeyFrame **ppEndKeyFrame, float *pFactor) const;

        // look up the transform for a specified time
        void GetBoneTransform(float time, Transform *pTransform, bool interpolate = true) const;

    protected:
        // load keyframes from stream
        bool LoadKeyFramesFromStream(ByteStream *pStream, float duration, uint32 keyFrameCount);
        
        // keyframe data
        float m_duration;
        MemArray<KeyFrame> m_keyFrames;
    };

    class BoneTrack : public TransformTrack
    {
    public:
        // look up bone index
        const uint32 GetBoneIndex() const { return m_boneIndex; }

        // read from a stream
        bool LoadFromStream(ByteStream *pStream, const Skeleton *pSkeleton);

    protected:
        uint32 m_boneIndex;
    };

    class RootMotionTrack : public TransformTrack
    {
    public:
        // read from a stream
        bool LoadFromStream(ByteStream *pStream, const Skeleton *pSkeleton);
    };

public:
    SkeletalAnimation(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    virtual ~SkeletalAnimation();

    // skeleton
    const Skeleton *GetSkeleton() const { return m_pSkeleton; }

    // parameters
    float GetDuration() const { return m_duration; }

    // bone tracks
    const TransformTrack *GetBoneTrack(uint32 trackIndex) { DebugAssert(trackIndex < m_boneTrackCount); return &m_pBoneTracks[trackIndex]; }
    const TransformTrack *GetBoneTrackForBone(uint32 boneIndex) { DebugAssert(boneIndex < m_pSkeleton->GetBoneCount()); return m_ppBoneIndexToBoneTrack[boneIndex]; }
    const uint32 GetBoneTrackCount() const { return m_boneTrackCount; }

    // root motion track
    const RootMotionTrack *GetRootMotionTrack() const { return m_pRootMotionTrack; }
    bool HasRootMotionTrack() const { return (m_pRootMotionTrack != nullptr); }

    // initialization
    bool LoadFromStream(const char *name, ByteStream *pStream);

    // fast path to find the relative transform for a bone
    bool CalculateRelativeBoneTransform(uint32 boneIndex, float time, Transform *pTransform, bool interpolate = true) const;

    // slow path to find the absolute transform for a bone
    void CalculateAbsoluteBoneTransform(uint32 boneIndex, float time, Transform *pTransform, bool interpolate = true) const;

private:
    const Skeleton *m_pSkeleton;

    float m_duration;

    BoneTrack *m_pBoneTracks;
    const BoneTrack **m_ppBoneIndexToBoneTrack;
    uint32 m_boneTrackCount;

    RootMotionTrack *m_pRootMotionTrack;
};

