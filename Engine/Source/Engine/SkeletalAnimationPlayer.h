#pragma once
#include "Engine/Common.h"
#include "Engine/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalAnimation.h"

class SkeletalMeshRenderProxy;

class SkeletalAnimationPlayer
{
public:
    enum WrapMode
    {
        WrapMode_None,        // resets back to base frame at completion
        WrapMode_Once,        // plays animation once, then resets back to first frame
        WrapMode_Hold,        // plays animation once, and keeps state at last frame
        WrapMode_PingPong,    // plays animation forward then backwards, one repeat is one forward or backwards cycle and hold there
        WrapMode_Count,
    };

    class MeshState
    {
    public:
        MeshState();
        MeshState(uint32 boneCount);

        void SetBoneCount(uint32 boneCount);

        const Transform &GetBoneTransform(uint32 boneIndex) const { return m_transforms[boneIndex]; }
        void SetBoneTransform(uint32 boneIndex, const Transform &transform) { m_transforms[boneIndex] = transform; }

        const Transform *GetBoneTransformArray() const { return m_transforms.GetBasePointer(); }
        const uint32 GetBoneTransformArraySize() const { return m_transforms.GetSize(); }

        MeshState *Copy() const;
        void CopyFrom(const MeshState *state);

    private:
        MemArray<Transform> m_transforms;
    };

    class Channel
    {
        friend SkeletalAnimationPlayer;

    public:
        const uint32 GetIndex() const { return m_index; }
        const float GetWeight() const { return m_weight; }
        const WrapMode GetWrapMode() const { return m_wrapMode; }
        const float GetTime() const { return m_time; }
        const bool IsPlaying() const { return m_playing; }

        const SkeletalAnimation *GetAnimation() const { return m_pAnimation; }
        const float GetPlaybackSpeed() const { return Math::Abs(m_playbackSpeed); }
        const int32 GetLoopCount() const { return m_loopCount; }

        // change weight
        void SetWeight(float weight) { m_weight = weight; }

        // change repeat mode
        void SetWrapMode(WrapMode mode) { m_wrapMode = mode; }

        // change playback speed
        void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

        // change loop count
        void SetLoopCount(int32 loopCount) { m_loopCount = loopCount; }

        // clear any animation
        void Clear();

        // stops the animation in its current state
        void Stop() { m_playing = false; }

        // restarts the animation from the last position
        void Play() { m_playing = true; }

        // full play invoke
        void PlayAnimation(const SkeletalAnimation *pAnimation, float playbackSpeed = 1.0f, int32 loopCount = 0, float transitionTime = 0.0f, bool resetTime = true);

        // update
        void Update(float deltaTime);

    private:
        // channel information
        SkeletalAnimationPlayer *m_pPlayer;
        uint32 m_index;
        float m_weight;
        WrapMode m_wrapMode;
        float m_time;
        bool m_playing;

        // transitions
        MeshState *m_pTransitionMeshState;
        float m_transitionTime;

        // current state
        const SkeletalAnimation *m_pAnimation;
        float m_playbackSpeed;
        int32 m_loopCount;

        // queued animation
        /*struct QueuedAnimation
        {
            const SkeletalAnimation *pAnimation;
            float PlaybackSpeed;
            float TransitionTime;
            int32 LoopCount;
        };*/
    };

public:
    SkeletalAnimationPlayer();
    SkeletalAnimationPlayer(const SkeletalMesh *pSkeletalMesh);
    ~SkeletalAnimationPlayer();

    // reference skeleton
    const SkeletalMesh *GetSkeletalMesh() const { return m_pSkeletalMesh; }
    void SetSkeletalMesh(const SkeletalMesh *pSkeletalMesh);

    // channel allocation
    const Channel *GetChannel(uint32 channelIndex) const { return &m_channels[channelIndex]; }
    Channel *GetChannel(uint32 channelIndex) { return &m_channels[channelIndex]; }
    uint32 GetChannelCount() const { return m_channels.GetSize(); }
    void SetChannelCount(uint32 channelCount);

    // current state
    const SkeletalAnimation *GetChannelAnimation(uint32 channelIndex) const { return m_channels[channelIndex].GetAnimation(); }
    uint32 GetChannelLoopCount(uint32 channelIndex) const { return m_channels[channelIndex].GetLoopCount(); }
    float GetChannelPlaybackSpeed(uint32 channelIndex) const { return m_channels[channelIndex].GetPlaybackSpeed(); }
    float GetChannelElapsedTime(uint32 channelIndex) const { return m_channels[channelIndex].GetTime(); }
    bool GetChannelPlaying(uint32 channelIndex) const { return m_channels[channelIndex].IsPlaying(); }

    // playback control, negative playbackSpeed will play the animation in reverse, loopCount of -1 will loop infinitely, if hold is set, will remain in pose after completion
    void PlayAnimation(const SkeletalAnimation *pAnimation, float playbackSpeed = 1.0f, int32 loopCount = 1, bool hold = true, uint32 channelIndex = 0, float weight = 1.0f, float transitionTime = 0.3f);

    // temporarily pause playback, call ResumeAnimation to resume it
    void PauseAnimation(uint32 channelIndex = 0);
    
    // resume paused playback
    void ResumeAnimation(uint32 channelIndex = 0);

    // clear the current animation and reset to base frame
    void ClearAnimation(uint32 channelIndex = 0);

    // clear all channels animations
    void ClearAllAnimations();

    // modify playback
    void SetPlaybackSpeed(float playbackSpeed, uint32 channelIndex = 0);
    void SetPlaybackLoopCount(int32 loopCount, uint32 channelIndex = 0);
    void SetPlaybackHold(bool hold, uint32 channelIndex = 0);

    // call every frame
    void Update(const float deltaTime);

    // call before passing to render proxy
    void UpdateMeshState();

    // transform cache
    const MeshState *GetMeshState() const { return &m_meshState; }
    const Transform &GetBoneTransform(uint32 boneIndex) const { return m_meshState.GetBoneTransform(boneIndex); }

    // local space
    Transform GetFullBoneTransform(uint32 boneIndex) const;
    void UpdateSkeletalMeshRenderProxy(SkeletalMeshRenderProxy *pRenderProxy);

private:
    // reset to base frame
    void ResetToBaseFrame();

    // calculate the pose for a bone (assumes that the parent bone has been done)
    void UpdateBoneTransform(const Skeleton::Bone *pSkeletonBone, const Transform *parentBoneTransform);

    // vars
    const SkeletalMesh *m_pSkeletalMesh;
    
    // channel array
    MemArray<Channel> m_channels;

    // current mesh state in skeleton, not bone space
    MeshState m_meshState;
};
