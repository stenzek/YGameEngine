#include "Engine/PrecompiledHeader.h"
#include "Engine/SkeletalAnimationPlayer.h"
#include "Renderer/RenderProxies/SkeletalMeshRenderProxy.h"
Log_SetChannel(SkeletalAnimationPlayer);

SkeletalAnimationPlayer::MeshState::MeshState()
{

}

SkeletalAnimationPlayer::MeshState::MeshState(uint32 boneCount)
{
    m_transforms.Resize(boneCount);
    m_transforms.ZeroContents();
}

void SkeletalAnimationPlayer::MeshState::SetBoneCount(uint32 boneCount)
{
    m_transforms.Resize(boneCount);
    m_transforms.ZeroContents();
}

SkeletalAnimationPlayer::MeshState *SkeletalAnimationPlayer::MeshState::Copy() const
{
    MeshState *newState = new MeshState();
    newState->m_transforms.Assign(m_transforms);
    return newState;
}

void SkeletalAnimationPlayer::MeshState::CopyFrom(const MeshState *state)
{
    m_transforms.Assign(state->m_transforms);
}

void SkeletalAnimationPlayer::Channel::Clear()
{
    SAFE_RELEASE(m_pAnimation);

    if (m_pTransitionMeshState != nullptr)
    {
        delete m_pTransitionMeshState;
        m_pTransitionMeshState = nullptr;
    }

    m_transitionTime = 0.0f;
    m_time = 0.0f;
    m_playing = false;
    m_playbackSpeed = 0.0f;
    m_loopCount = 0;
}

void SkeletalAnimationPlayer::Channel::PlayAnimation(const SkeletalAnimation *pAnimation, float playbackSpeed /* = 1.0f */, int32 loopCount /* = 0 */, float transitionTime /* = 0.0f */, bool resetTime /* = true */)
{
    if (pAnimation == nullptr)
    {
        Clear();
        return;
    }

    if (transitionTime > 0.0f)
    {
        // enable a transition
        if (m_pTransitionMeshState != nullptr)
            m_pTransitionMeshState->CopyFrom(m_pPlayer->GetMeshState());
        else
            m_pTransitionMeshState = m_pPlayer->GetMeshState()->Copy();

        // remove all the local space transforms
        //for (uint32 boneIndex = 0; boneIndex < m_pTransitionMeshState->GetBoneTransformArraySize(); boneIndex++)
            //m_pTransitionMeshState->SetBoneTransform(boneIndex, Transform::ConcatenateTransforms(m_pTransitionMeshState->GetBoneTransform(boneIndex), m_pPlayer->GetSkeletalMesh()->GetBone(boneIndex)->LocalToBoneTransform.Inverse()));

        // set the time for it
        m_transitionTime = transitionTime;
    }

    // remove current animation
    if (m_pAnimation != nullptr)
        m_pAnimation->Release();

    // set new animation
    m_pAnimation = pAnimation;
    m_pAnimation->AddRef();
    m_playbackSpeed = playbackSpeed;
    m_loopCount = loopCount;
    m_time = (resetTime || pAnimation->GetDuration() == 0.0f) ? 0.0f : Y_fmodf(m_time, pAnimation->GetDuration());
    m_playing = true;
}

void SkeletalAnimationPlayer::Channel::Update(float deltaTime)
{
    // is playing?
    if (m_pAnimation == nullptr || !m_playing)
        return;

    // handle transitions
    if (m_pTransitionMeshState != nullptr)
    {
        // transition complete?
        if ((m_time + deltaTime) >= m_transitionTime)
        {
            // completed, so subtract the time
            deltaTime -= (m_transitionTime - m_time);
            m_time = 0.0f;

            // and clear the transition state
            delete m_pTransitionMeshState;
            m_pTransitionMeshState = nullptr;
            m_transitionTime = 0.0f;
        }
        else
        {
            // transition still in progress
            m_time += deltaTime;
            return;
        }
    }

    // factor in playback speed
    deltaTime *= Math::Abs(m_playbackSpeed);

    // loop
    for (;;)
    {
        // animation complete?
        if ((m_time + deltaTime) >= m_pAnimation->GetDuration())
        {
            // last loop cycle?
            if (m_loopCount == 1)
            {
                // handle wrap modes
                switch (m_wrapMode)
                {
                case WrapMode_None:
                    m_pAnimation->Release();
                    m_pAnimation = nullptr;
                    m_time = 0.0f;
                    break;

                case WrapMode_Once:
                    m_time = 0.0f;
                    break;

                case WrapMode_Hold:
                    m_time = m_pAnimation->GetDuration();
                    break;

                case WrapMode_PingPong:
                    m_time = (m_playbackSpeed >= 0.0f) ? m_pAnimation->GetDuration() : 0.0f;
                    break;
                }

                m_loopCount = 0;
                m_playing = false;
                return;
            }
            else
            {
                // loop cycles remaining
                if (m_loopCount > 0)
                    m_loopCount--;

                // subtract remaining time off
                deltaTime -= (m_pAnimation->GetDuration() - m_time);
                m_time = 0.0f;

                // handle wrap modes
                if (m_wrapMode == WrapMode_PingPong)
                    m_playbackSpeed = -m_playbackSpeed;

                // handle pose animations, preventing infinite loop here
                if (Math::NearEqual(m_pAnimation->GetDuration(), 0.0f, Y_FLT_EPSILON))
                    break;
            }
        }
        else
        {
            // add time
            m_time += deltaTime;
            break;
        }
    }
}

SkeletalAnimationPlayer::SkeletalAnimationPlayer()
    : m_pSkeletalMesh(nullptr)
{
    // initialize a single channel
    SetChannelCount(1);
}

SkeletalAnimationPlayer::SkeletalAnimationPlayer(const SkeletalMesh *pSkeletalMesh)
    : SkeletalAnimationPlayer()
{
    SetSkeletalMesh(pSkeletalMesh);
}

SkeletalAnimationPlayer::~SkeletalAnimationPlayer()
{
    for (uint32 channelIndex = 0; channelIndex < m_channels.GetSize(); channelIndex++)
        ClearAnimation(channelIndex);

    if (m_pSkeletalMesh != nullptr)
        m_pSkeletalMesh->Release();
}

void SkeletalAnimationPlayer::SetChannelCount(uint32 channelCount)
{
    DebugAssert(channelCount > 0);

    // calculate change amounts
    uint32 startIndex = m_channels.GetSize();
    uint32 clearCount = (channelCount <= m_channels.GetSize()) ? (m_channels.GetSize() - channelCount) : 0;
    uint32 newCount = (channelCount >= m_channels.GetSize()) ? (channelCount - m_channels.GetSize()) : 0;

    // clear channels up to channelCount
    for (uint32 channelIndex = 0; channelIndex < clearCount; channelIndex++)
        m_channels[channelIndex].Clear();

    // add new channels
    m_channels.Resize(channelCount);
    for (uint32 channelIndex = 0; channelIndex < newCount; channelIndex++)
    {
        Channel *channel = &m_channels[startIndex + channelIndex];
        Y_memzero(channel, sizeof(Channel));
        channel->m_pPlayer = this;
        channel->m_index = startIndex + channelIndex;
        channel->m_weight = 1.0f;
        channel->m_wrapMode = WrapMode_Once;
    }
}

void SkeletalAnimationPlayer::SetSkeletalMesh(const SkeletalMesh *pSkeletalMesh)
{
    if (pSkeletalMesh == nullptr)
    {
        if (m_pSkeletalMesh != nullptr)
        {
            ClearAllAnimations();
            m_pSkeletalMesh->Release();
            m_pSkeletalMesh = nullptr;
        }

        return;
    }

    if (m_pSkeletalMesh != pSkeletalMesh)
    {
        if (m_pSkeletalMesh != nullptr)
            m_pSkeletalMesh->Release();

        m_pSkeletalMesh = pSkeletalMesh;
        m_pSkeletalMesh->AddRef();
    }

    // initalize transform array
    m_meshState.SetBoneCount(pSkeletalMesh->GetBoneCount());

    // set to base frame
    ResetToBaseFrame();
    UpdateMeshState();
}

void SkeletalAnimationPlayer::PlayAnimation(const SkeletalAnimation *pAnimation, float playbackSpeed /* = 1.0f */, int32 loopCount /* = 1 */, bool hold /* = false */, uint32 channelIndex /* = 0 */, float weight /* = 1.0f */, float transitionTime /* = 0.3f */)
{
    DebugAssert(m_pSkeletalMesh != nullptr && pAnimation->GetSkeleton() == m_pSkeletalMesh->GetSkeleton());
    DebugAssert(channelIndex < m_channels.GetSize());
    Channel &channel = m_channels[channelIndex];

    if (loopCount < 0)
        loopCount = 0;

    if (hold)
        channel.SetWrapMode(WrapMode_Hold);
    else
        channel.SetWrapMode(WrapMode_Once);

    channel.SetWeight(weight);
    channel.PlayAnimation(pAnimation, playbackSpeed, loopCount, transitionTime);
}

void SkeletalAnimationPlayer::PauseAnimation(uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    m_channels[channelIndex].Stop();
}

void SkeletalAnimationPlayer::ResumeAnimation(uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    m_channels[channelIndex].Play();
}

void SkeletalAnimationPlayer::ClearAnimation(uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    Channel &channel = m_channels[channelIndex];
    channel.Clear();
}

void SkeletalAnimationPlayer::ClearAllAnimations()
{
    for (uint32 channelIndex = 0; channelIndex < m_channels.GetSize(); channelIndex++)
        ClearAnimation(channelIndex);
}

void SkeletalAnimationPlayer::SetPlaybackSpeed(float playbackSpeed, uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    Channel &channel = m_channels[channelIndex];
    channel.SetPlaybackSpeed(playbackSpeed);
}

void SkeletalAnimationPlayer::SetPlaybackLoopCount(int32 loopCount, uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    Channel &channel = m_channels[channelIndex];

    if (loopCount < 0)
        loopCount = 0;

    channel.SetLoopCount(loopCount);
}

void SkeletalAnimationPlayer::SetPlaybackHold(bool hold, uint32 channelIndex /* = 0 */)
{
    DebugAssert(channelIndex < m_channels.GetSize());
    Channel &channel = m_channels[channelIndex];

    if (hold)
        channel.SetWrapMode(WrapMode_Hold);
    else
        channel.SetWrapMode(WrapMode_Once);
}

void SkeletalAnimationPlayer::UpdateBoneTransform(const Skeleton::Bone *pSkeletonBone, const Transform *parentBoneTransform)
{
    // find index
    uint32 meshBoneIndex;
    for (meshBoneIndex = 0; meshBoneIndex < m_pSkeletalMesh->GetBoneCount(); meshBoneIndex++)
    {
        if (m_pSkeletalMesh->GetBone(meshBoneIndex)->SkeletonBoneIndex == pSkeletonBone->GetIndex())
            break;
    }

    // if there was no bone index, and this is a leaf node, skip it entirely
    if (meshBoneIndex == m_pSkeletalMesh->GetBoneCount() && pSkeletonBone->GetChildBoneCount() == 0)
        return;

    // calculate the bone's transform
    Transform thisBoneTransform;
    Transform thisBoneTransformForChildren;
    Transform channelBonePose;
    Transform channelBonePoseForChildren;
    bool hasTransform = false;
    bool hasChildren = (pSkeletonBone->GetChildBoneCount() > 0);

    // for each channel
    for (uint32 channelIndex = 0; channelIndex < m_channels.GetSize(); channelIndex++)
    {
        const Channel *channel = &m_channels[channelIndex];

        // channel has an animation?
        if (channel->m_pAnimation == nullptr)
            continue;

        // channel is transitioning?
        if (channel->m_pTransitionMeshState != nullptr && meshBoneIndex != m_pSkeletalMesh->GetBoneCount())
        {
            // find the transform we are transitioning to
            // if we don't have a transform for this bone, and there isn't a current transform, set it to the base frame
            Transform toTransform;
            if (channel->m_pAnimation->CalculateRelativeBoneTransform(pSkeletonBone->GetIndex(), 0.0f, &toTransform, false))
            {
                // have to add the parent in here
                if (parentBoneTransform != nullptr)
                    toTransform = Transform::ConcatenateTransforms(toTransform, *parentBoneTransform);
            }
            else
            {
                if (hasTransform)
                    toTransform = thisBoneTransform;
                else
                    toTransform = pSkeletonBone->GetAbsoluteBaseFrameTransform();
            }

            // interpolate them, get from transform from the transition mesh state
            channelBonePose = Transform::LinearInterpolate(channel->m_pTransitionMeshState->GetBoneTransform(meshBoneIndex), toTransform, channel->m_time / channel->m_transitionTime);
            channelBonePoseForChildren = toTransform;
        }
        else
        {
            // reverse playback?
            float animationTime = channel->m_time;
            if (channel->m_playbackSpeed < 0.0f)
                animationTime = channel->m_pAnimation->GetDuration() - animationTime;

            // get transform
            if (!channel->m_pAnimation->CalculateRelativeBoneTransform(pSkeletonBone->GetIndex(), animationTime, &channelBonePose, true))
                continue;

            // apply parent transform
            if (parentBoneTransform != nullptr)
                channelBonePose = Transform::ConcatenateTransforms(channelBonePose, *parentBoneTransform);

            channelBonePoseForChildren = channelBonePose;
        }

        // handle weighting
        if (hasTransform)
        {
            if (channel->GetWeight() < 1.0f)
            {
                // eugh, ugly as hell
                float4x4 existingPose(thisBoneTransform.GetTransformMatrix3x4());
                float4x4 newPose(channelBonePose.GetTransformMatrix4x4());

                // weight the transforms
                existingPose *= (1.0f - channel->GetWeight());
                newPose *= channel->GetWeight();

                // construct new transform
                thisBoneTransform = Transform(existingPose + newPose);

                if (hasChildren)
                {
                    existingPose = thisBoneTransformForChildren.GetTransformMatrix3x4() * (1.0f - channel->GetWeight());
                    newPose = channelBonePoseForChildren.GetTransformMatrix3x4() * channel->GetWeight();
                    thisBoneTransformForChildren = Transform(existingPose + newPose);
                }
            }
            else
            {
                thisBoneTransform = channelBonePose;
                thisBoneTransformForChildren = channelBonePoseForChildren;
            }
        }
        else
        {
            // just set the transform
            thisBoneTransform = channelBonePose;
            thisBoneTransformForChildren = channelBonePose;
            hasTransform = true;
        }
    }

    // has a transform? if not, use base frame
    if (!hasTransform)
    {
        if (parentBoneTransform != nullptr)
            thisBoneTransform = Transform::ConcatenateTransforms(pSkeletonBone->GetRelativeBaseFrameTransform(), *parentBoneTransform);
        else
            thisBoneTransform = pSkeletonBone->GetRelativeBaseFrameTransform();

        thisBoneTransformForChildren = thisBoneTransform;
    }

    // calculate transform from local space
    if (meshBoneIndex != m_pSkeletalMesh->GetBoneCount())
        m_meshState.SetBoneTransform(meshBoneIndex, thisBoneTransform);
        //m_meshState.SetBoneTransform(meshBoneIndex, Transform::ConcatenateTransforms(m_pSkeletalMesh->GetBone(meshBoneIndex)->LocalToBoneTransform, thisBoneTransform));

    // handle any children
    for (uint32 childIndex = 0; childIndex < pSkeletonBone->GetChildBoneCount(); childIndex++)
        UpdateBoneTransform(pSkeletonBone->GetChildBone(childIndex), &thisBoneTransformForChildren);
}

void SkeletalAnimationPlayer::UpdateMeshState()
{
    // transform the root bone, the recusion will take care of the rest
    UpdateBoneTransform(m_pSkeletalMesh->GetSkeleton()->GetRootBone(), nullptr);
}

void SkeletalAnimationPlayer::Update(const float deltaTime)
{
    // update time on channels
    for (uint32 channelIndex = 0; channelIndex < m_channels.GetSize(); channelIndex++)
    {
        Channel *channel = &m_channels[channelIndex];
        channel->Update(deltaTime);
    }

#if 0

    if (m_pCurrentAnimation == nullptr || !m_currentAnimationPlaying)
        return;

    // update animation time
    m_currentAnimationTime += Math::Abs(m_currentAnimationPlaybackSpeed) * deltaTime;

    // end of animation?
    if (m_currentAnimationTime >= m_pCurrentAnimation->GetDuration())
    {
        // can we loop?
        if (m_currentAnimationLoopCount != 0 && (m_currentAnimationLoopCount < 0 || (--m_currentAnimationLoopCount) > 0))
        {
            // fix up the time
            m_currentAnimationTime = Y_fmodf(m_currentAnimationTime, m_pCurrentAnimation->GetDuration());
        }
        else
        {
            if (m_currentAnimationHold)
            {
                // ensure transforms are set to the last frame without interpolation
                SetTransformsForAnimationTime(m_pCurrentAnimation->GetDuration());
            }

            // clear animation references
            ClearAnimation(!m_currentAnimationHold);

            // no more work
            return;
        }
    }

    // interpolate transforms and stuff
    SetTransformsForAnimationTime(m_currentAnimationTime);

#endif
}

void SkeletalAnimationPlayer::UpdateSkeletalMeshRenderProxy(SkeletalMeshRenderProxy *pRenderProxy)
{
    DebugAssert(pRenderProxy->GetSkeletalMesh() == m_pSkeletalMesh);
    pRenderProxy->SetBoneTransforms(0, m_meshState.GetBoneTransformArraySize(), m_meshState.GetBoneTransformArray());
}

void SkeletalAnimationPlayer::ResetToBaseFrame()
{
    for (uint32 channelIndex = 0; channelIndex < m_channels.GetSize(); channelIndex++)
        ClearAnimation(channelIndex);
}

Transform SkeletalAnimationPlayer::GetFullBoneTransform(uint32 boneIndex) const
{
    DebugAssert(boneIndex < m_pSkeletalMesh->GetBoneCount());
    return Transform::ConcatenateTransforms(m_pSkeletalMesh->GetBone(boneIndex)->LocalToBoneTransform, m_meshState.GetBoneTransform(boneIndex));
}

#if 0

void SkeletalAnimationPlayer::SetTransformsForAnimationTime(float animationTime)
{
    // handle reverse
    if (m_currentAnimationPlaybackSpeed < 0.0f)
        animationTime = Max(m_pCurrentAnimation->GetDuration() - animationTime, 0.0f);

#if 1
    // slow path
    for (uint32 boneIndex = 0; boneIndex < m_pSkeleton->GetBoneCount(); boneIndex++)
    {
        // update transform
        m_pCurrentAnimation->CalculateAbsoluteBoneTransform(boneIndex, animationTime, &m_boneTransforms[boneIndex]);

        // update matrix
        m_boneTransformMatrices[boneIndex] = m_boneTransforms[boneIndex].GetTransformMatrix3x4();
    }
#endif


#if 0

    // calculate keyframe index
    float keyFrameTime = animationTime / (1.0f / m_pCurrentAnimation->GetKeyFramesPerSecond());
    uint32 keyFrameIndex = (uint32)Math::Truncate(keyFrameTime);
    float lerpFactor = Math::FractionalPart(keyFrameTime);
    bool doLerp = !Math::NearEqual(lerpFactor, 0.0f, Y_FLT_EPSILON);

    // handle out of range
    if ((keyFrameIndex + 1) >= m_pCurrentAnimation->GetKeyFrameCount())
    {
        keyFrameIndex = m_pCurrentAnimation->GetKeyFrameCount() - 1;
        doLerp = false;
    }

    //Log_DevPrintf("KF: %u, lerp = %s, lerpFactor = %s", keyFrameIndex, StringConverter::BoolToString(doLerp).GetCharArray(), StringConverter::FloatToString(lerpFactor).GetCharArray());
    //doLerp = false;

    // update bones
    for (uint32 boneIndex = 0; boneIndex < m_pSkeleton->GetBoneCount(); boneIndex++)
    {
        if (doLerp)
        {
            // get 2 transforms
            Transform startTransform(m_pCurrentAnimation->GetKeyFrameBoneTransform(keyFrameIndex, boneIndex));
            Transform endTransform(m_pCurrentAnimation->GetKeyFrameBoneTransform(keyFrameIndex + 1, boneIndex));

            // lerp between them, and set
            m_boneTransforms[boneIndex] = Transform::LinearInterpolate(startTransform, endTransform, lerpFactor);
        }
        else
        {
            // just set to transform
            m_boneTransforms[boneIndex] = m_pCurrentAnimation->GetKeyFrameBoneTransform(keyFrameIndex, boneIndex);
        }

        // update matrix
        m_boneTransformMatrices[boneIndex] = m_boneTransforms[boneIndex].GetTransformMatrix3x4();
    }

#endif
}
#endif
