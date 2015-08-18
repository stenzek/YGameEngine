#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/SkeletalAnimation.h"
#include "Core/PropertyTable.h"

struct ResourceCompilerCallbacks;

class SkeletalAnimationGenerator
{
public:
    class TransformTrack
    {
        friend class SkeletalAnimationGenerator;

    public:
        struct KeyFrame
        {
            KeyFrame() {}
            KeyFrame(const float time, const float3 &position, const Quaternion &rotation, const float3 &scale)
                : m_time(time), m_position(position), m_rotation(rotation), m_scale(scale) {}

            const float &GetTime() const { return m_time; }
            const float3 &GetPosition() const { return m_position; }
            const Quaternion &GetRotation() const { return m_rotation; }
            const float3 &GetScale() const { return m_scale; }
            void SetTime(float time) { m_time = time; }
            void SetPosition(const float3 &position) { m_position = position; }
            void SetRotation(const Quaternion &rotation) { m_rotation = rotation; }
            void SetScale(const float3 &scale) { m_scale = scale; }

        private:
            float m_time;
            float3 m_position;
            Quaternion m_rotation;
            float3 m_scale;
        };

    public:
        TransformTrack();
        ~TransformTrack();

        // keyframes
        const uint32 GetKeyFrameCount() const { return m_keyFrames.GetSize(); }
        const KeyFrame *GetKeyFrame(uint32 index) const;
        KeyFrame *GetKeyFrame(uint32 index);
        KeyFrame *AddKeyFrame(float time, const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One);
        void DeleteKeyFrame(uint32 index);
        void SortKeyFrames();

        // duration
        const float GetDuration() const { return m_duration; }
        void UpdateDuration();

        // get keyframe for a time, may fail
        const KeyFrame *GetKeyFrameForTime(float time) const;

        // interpolate and calculate keyframe for the specified time
        void InterpolateKeyFrameAtTime(float time, KeyFrame *pOutKeyFrame) const;

        // clip keyframes
        void ClipKeyFrames(float startTime, float endTime);

        // optimize the animation sequence
        void Optimize();

    private:
        MemArray<KeyFrame> m_keyFrames;
        float m_duration;
    };

    class BoneTrack : public TransformTrack
    {
    public:
        BoneTrack(const char *boneName);
        ~BoneTrack();

        // referenced bone
        const String &GetBoneName() const { return m_boneName; }
        void SetBoneName(const char *boneName) { m_boneName = boneName; }

    private:
        String m_boneName;
    };

    class RootMotionTrack : public TransformTrack
    {
    public:
        RootMotionTrack();
        ~RootMotionTrack();

    private:
    };

public:
    SkeletalAnimationGenerator();
    ~SkeletalAnimationGenerator();

    // properties
    const PropertyTable *GetPropertyTable() const { return &m_properties; }

    // skeleton
    const String &GetSkeletonName() const { return m_skeletonName; }
    void SetSkeletonName(const char *skeletonName) { m_skeletonName = skeletonName; }

    // preview mesh
    const String &GetPreviewMeshName() const;
    void SetPreviewMeshName(const char *previewMeshName);

    // duration
    const float GetDuration() const { return m_duration; }
    void UpdateDuration();

    // bone tracks
    const BoneTrack *GetBoneTrackByIndex(uint32 index) const { return m_boneTracks[index]; }
    const BoneTrack *GetBoneTrackByName(const char *boneName) const;
    void AddBoneTrack(BoneTrack *boneTrack);
    BoneTrack *CreateBoneTrack(const char *boneName);
    BoneTrack *GetBoneTrackByIndex(uint32 index) { return m_boneTracks[index]; }
    BoneTrack *GetBoneTrackByName(const char *boneName);
    uint32 GetBoneTrackCount() const { return m_boneTracks.GetSize(); }
    void RemoveBoneTrack(BoneTrack *boneTrack);

    // root motion track
    const RootMotionTrack *GetRootMotionTrack() const { return m_pRootMotionTrack; }
    RootMotionTrack *GetRootMotionTrack() { return m_pRootMotionTrack; }
    void SetRootMotionTrack(RootMotionTrack *rootMotionTrack) { delete m_pRootMotionTrack; m_pRootMotionTrack = rootMotionTrack; }
    RootMotionTrack *AddRootMotionTrack();
    void DeleteRootMotionTrack();

    // clip the animation to the specified time range, performs interpolation if necessary
    void ClipAnimation(float startTime, float endTime);

    // optimize animations
    void Optimize(bool removeEmptyTracks = true, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // Loading interface (from XML)
    bool LoadFromXML(const char *FileName, ByteStream *pStream);

    // Output interface
    bool SaveToXML(ByteStream *pStream) const;
    bool Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream, bool optimize = true) const;

    // Copy existing animation
    void Copy(const SkeletalAnimationGenerator *pCopy);

    // generate a list of times for keyframes, this is not stored with the animation but can be generated on demand
    void GenerateKeyFrameTimeList(PODArray<float> *pKeyFrameTimeArray) const;

private:
    bool InternalCompile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const;
    static bool InternalWriteTransformKeyFrame(ByteStream *pStream, const TransformTrack::KeyFrame *pKeyFrame);

    String m_skeletonName;
    PropertyTable m_properties;
    float m_duration;
    PODArray<BoneTrack *> m_boneTracks;
    RootMotionTrack *m_pRootMotionTrack;
};

