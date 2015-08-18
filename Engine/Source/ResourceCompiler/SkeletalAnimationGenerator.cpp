#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/SkeletalAnimationGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(SkeletalAnimationGenerator);

SkeletalAnimationGenerator::TransformTrack::TransformTrack()
{

}

SkeletalAnimationGenerator::TransformTrack::~TransformTrack()
{

}

const SkeletalAnimationGenerator::TransformTrack::KeyFrame *SkeletalAnimationGenerator::TransformTrack::GetKeyFrame(uint32 index) const
{
    return (index < m_keyFrames.GetSize()) ? &m_keyFrames[index] : nullptr;
}

SkeletalAnimationGenerator::TransformTrack::KeyFrame *SkeletalAnimationGenerator::TransformTrack::GetKeyFrame(uint32 index)
{
    return (index < m_keyFrames.GetSize()) ? &m_keyFrames[index] : nullptr;
}

SkeletalAnimationGenerator::TransformTrack::KeyFrame *SkeletalAnimationGenerator::TransformTrack::AddKeyFrame(float time, const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */)
{
    DebugAssert(time >= 0.0f);

    // update duration
    m_duration = Max(m_duration, time);

    // find the position to insert at
    uint32 insertPosition;
    for (insertPosition = 0; insertPosition < m_keyFrames.GetSize(); insertPosition++)
    {
        if (time < m_keyFrames[insertPosition].GetTime())
            break;
    }

    // insert at position
    KeyFrame newKeyFrame(time, position, rotation, scale);
    m_keyFrames.Insert(newKeyFrame, insertPosition);
    return &m_keyFrames[insertPosition];
}

void SkeletalAnimationGenerator::TransformTrack::DeleteKeyFrame(uint32 index)
{
    DebugAssert(index < m_keyFrames.GetSize());
    m_keyFrames.OrderedRemove(index);
}

static int KeyFrameCompareFunction(const SkeletalAnimationGenerator::TransformTrack::KeyFrame *pLeft, const SkeletalAnimationGenerator::TransformTrack::KeyFrame *pRight)
{
    return Math::CompareResult(pLeft->GetTime(), pRight->GetTime());
}

void SkeletalAnimationGenerator::TransformTrack::SortKeyFrames()
{
    m_keyFrames.Sort(KeyFrameCompareFunction);
}

void SkeletalAnimationGenerator::TransformTrack::UpdateDuration()
{
    m_duration = 0.0f;

    for (uint32 keyFrameIndex = 0; keyFrameIndex < m_keyFrames.GetSize(); keyFrameIndex++)
        m_duration = Max(m_duration, m_keyFrames[keyFrameIndex].GetTime());
}

const SkeletalAnimationGenerator::BoneTrack::KeyFrame *SkeletalAnimationGenerator::TransformTrack::GetKeyFrameForTime(float time) const
{
    if (m_keyFrames.GetSize() == 1 || time < m_keyFrames[0].GetTime())
    {
        // use first and only position
        return &m_keyFrames[0];
    }

    // find the key that we reside in with a time of <= t
    uint32 keyIndex;
    for (keyIndex = 0; keyIndex < (m_keyFrames.GetSize() - 1); keyIndex++)
    {
        if (time < m_keyFrames[keyIndex + 1].GetTime())
            break;
    }

    // is this the last key?
    uint32 nextKeyIndex = keyIndex + 1;
    if (nextKeyIndex == m_keyFrames.GetSize())
    {
        // use this (last) key
        return &m_keyFrames[keyIndex];
    }

    // get factor
    const double thisKeyTime = m_keyFrames[keyIndex].GetTime();
    const double nextKeyTime = m_keyFrames[nextKeyIndex].GetTime();
    const double factor = (time - thisKeyTime) / (nextKeyTime - thisKeyTime);
    DebugAssert(factor >= 0.0 && factor <= 1.0);

    // return appropriate frame
    if (factor < 0.5)
        return &m_keyFrames[keyIndex];
    else
        return &m_keyFrames[nextKeyIndex];
}

void SkeletalAnimationGenerator::TransformTrack::InterpolateKeyFrameAtTime(float time, KeyFrame *pOutKeyFrame) const
{
    if (m_keyFrames.GetSize() == 1 || time < m_keyFrames[0].GetTime())
    {
        // use first and only position
        *pOutKeyFrame = m_keyFrames[0];
        return;
    }

    // find the key that we reside in with a time of <= t
    uint32 keyIndex;
    for (keyIndex = 0; keyIndex < (m_keyFrames.GetSize() - 1); keyIndex++)
    {
        if (time < m_keyFrames[keyIndex + 1].GetTime())
            break;
    }

    // is this the last key?
    uint32 nextKeyIndex = keyIndex + 1;
    if (nextKeyIndex == m_keyFrames.GetSize())
    {
        // use this (last) key
        *pOutKeyFrame = m_keyFrames[keyIndex];
        return;
    }

    // get factor
    const double thisKeyTime = m_keyFrames[keyIndex].GetTime();
    const KeyFrame &thisKeyFrame = m_keyFrames[keyIndex];
    const double nextKeyTime = m_keyFrames[nextKeyIndex].GetTime();
    const KeyFrame &nextKeyFrame = m_keyFrames[nextKeyIndex];
    const float factor = static_cast<float>((time - thisKeyTime) / (nextKeyTime - thisKeyTime));
    DebugAssert(factor >= 0.0 && factor <= 1.0);

    // factor == 0 or 1?
    if (Math::NearEqual(factor, 0.0f, Y_FLT_EPSILON))
    {
        *pOutKeyFrame = m_keyFrames[keyIndex];
    }
    else if (Math::NearEqual(factor, 1.0f, Y_FLT_EPSILON))
    {
        *pOutKeyFrame = m_keyFrames[nextKeyIndex];
    }
    else
    {
        pOutKeyFrame->SetTime(time);
        pOutKeyFrame->SetPosition(thisKeyFrame.GetPosition().Lerp(nextKeyFrame.GetPosition(), factor));
        pOutKeyFrame->SetRotation(Quaternion::LinearInterpolate(thisKeyFrame.GetRotation(), nextKeyFrame.GetRotation(), factor));
        pOutKeyFrame->SetScale(thisKeyFrame.GetScale().Lerp(nextKeyFrame.GetScale(), factor));
    }
}

void SkeletalAnimationGenerator::TransformTrack::ClipKeyFrames(float startTime, float endTime)
{
    MemArray<KeyFrame> newKeyFrames;
    KeyFrame newKeyFrame;

    // is there a frame with an exact match on the start time?
    bool foundExactStartFrame = false;
    for (uint32 i = 0; i < m_keyFrames.GetSize(); i++)
    {
        if (m_keyFrames[i].GetTime() == startTime)
        {
            foundExactStartFrame = true;
            break;
        }
    }

    // if not found, calculate the key frame at the specified start time
    if (!foundExactStartFrame)
    {
        InterpolateKeyFrameAtTime(startTime, &newKeyFrame);
        newKeyFrame.SetTime(0.0f);
        newKeyFrames.Add(newKeyFrame);
    }

    // add any key frames within the range to the new frame list
    for (uint32 i = 0; i < m_keyFrames.GetSize(); i++)
    {
        const KeyFrame &sourceKeyFrame = m_keyFrames[i];
        float kfTime = sourceKeyFrame.GetTime();
        if (kfTime >= startTime && kfTime <= endTime)
        {
            // calculate new time
            kfTime -= startTime;
            DebugAssert(kfTime >= 0.0f);

            // create new keyframe
            newKeyFrame.SetTime(kfTime);
            newKeyFrame.SetPosition(sourceKeyFrame.GetPosition());
            newKeyFrame.SetRotation(sourceKeyFrame.GetRotation());
            newKeyFrame.SetScale(sourceKeyFrame.GetScale());
            newKeyFrames.Add(newKeyFrame);
        }
    }

    // swap out the arrays
    newKeyFrames.Shrink();
    m_keyFrames.Swap(newKeyFrames);
}

void SkeletalAnimationGenerator::TransformTrack::Optimize()
{
    SortKeyFrames();

    // remove any frames that are doubling up on everything
    uint32 groupStart = 0;
    uint32 groupCount = 1;
    for (uint32 keyFrameIndex = 1; keyFrameIndex < m_keyFrames.GetSize(); )
    {
        // same as group?
        if (m_keyFrames[keyFrameIndex].GetPosition().NearEqual(m_keyFrames[groupStart].GetPosition(), Y_FLT_EPSILON) &&
            m_keyFrames[keyFrameIndex].GetRotation() == m_keyFrames[groupStart].GetRotation() &&
            m_keyFrames[keyFrameIndex].GetScale().NearEqual(m_keyFrames[groupStart].GetScale(), Y_FLT_EPSILON))
        {
            // increment group count
            groupCount++;
            keyFrameIndex++;
            continue;
        }

        // start of a new group
        // was the previous group large enough to clip?
        if (groupCount > 2)
        {
            uint32 removeCount = groupCount - 2;
            Log_DevPrintf("SkeletalAnimationGenerator::TransformTrack::Optimize: removing %u frames starting at %u (%.2f)", removeCount, groupStart, m_keyFrames[groupStart].GetTime());
            for (uint32 i = 0; i < removeCount; i++)
                m_keyFrames.OrderedRemove(groupStart + 1);

            // reset counter back to start
            groupStart = 0;
            groupCount = 1;
            keyFrameIndex = 1;
            continue;
        }

        // update group state
        groupStart = keyFrameIndex;
        groupCount = 1;
        keyFrameIndex++;
        continue;
    }

    // was this a group that didn't end?
    if (groupCount > 2)
    {
        uint32 removeCount = groupCount - 2;
        Log_DevPrintf("SkeletalAnimationGenerator::TransformTrack::Optimize: removing %u frames starting at %u (%.2f)", removeCount, groupStart, m_keyFrames[groupStart].GetTime());
        for (uint32 i = 0; i < removeCount; i++)
            m_keyFrames.OrderedRemove(groupStart + 1);
    }
}

SkeletalAnimationGenerator::BoneTrack::BoneTrack(const char *boneName)
    : TransformTrack(),
      m_boneName(boneName)
{

}

SkeletalAnimationGenerator::BoneTrack::~BoneTrack()
{

}

SkeletalAnimationGenerator::RootMotionTrack::RootMotionTrack()
    : TransformTrack()
{

}

SkeletalAnimationGenerator::RootMotionTrack::~RootMotionTrack()
{

}

SkeletalAnimationGenerator::SkeletalAnimationGenerator()
    : m_pRootMotionTrack(nullptr)
{

}

SkeletalAnimationGenerator::~SkeletalAnimationGenerator()
{
    delete m_pRootMotionTrack;

    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
        delete m_boneTracks[i];
}

const String &SkeletalAnimationGenerator::GetPreviewMeshName() const
{
    return m_properties.GetPropertyValueDefaultString("PreviewMeshName");
}

void SkeletalAnimationGenerator::SetPreviewMeshName(const char *previewMeshName)
{
    m_properties.SetPropertyValue("PreviewMeshName", previewMeshName);
}

void SkeletalAnimationGenerator::UpdateDuration()
{
    m_duration = 0.0f;
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
        m_duration = Max(m_duration, m_boneTracks[i]->GetDuration());
}

const SkeletalAnimationGenerator::BoneTrack *SkeletalAnimationGenerator::GetBoneTrackByName(const char *boneName) const
{
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
    {
        if (m_boneTracks[i]->GetBoneName().Compare(boneName))
            return m_boneTracks[i];
    }

    return nullptr;
}

SkeletalAnimationGenerator::BoneTrack *SkeletalAnimationGenerator::GetBoneTrackByName(const char *boneName)
{
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
    {
        if (m_boneTracks[i]->GetBoneName().Compare(boneName))
            return m_boneTracks[i];
    }

    return nullptr;
}

void SkeletalAnimationGenerator::AddBoneTrack(BoneTrack *boneTrack)
{
    Assert(GetBoneTrackByName(boneTrack->GetBoneName()) == nullptr);
    m_boneTracks.Add(boneTrack);
    m_duration = Max(m_duration, boneTrack->GetDuration());
}

SkeletalAnimationGenerator::BoneTrack *SkeletalAnimationGenerator::CreateBoneTrack(const char *boneName)
{
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
    {
        if (m_boneTracks[i]->GetBoneName().Compare(boneName))
            return nullptr;
    }

    BoneTrack *track = new BoneTrack(boneName);
    m_boneTracks.Add(track);
    return track;
}

void SkeletalAnimationGenerator::RemoveBoneTrack(BoneTrack *boneTrack)
{
    int32 index = m_boneTracks.IndexOf(boneTrack);
    Assert(index >= 0);
    m_boneTracks.OrderedRemove((uint32)index);
}

SkeletalAnimationGenerator::RootMotionTrack *SkeletalAnimationGenerator::AddRootMotionTrack()
{
    delete m_pRootMotionTrack;
    m_pRootMotionTrack = new RootMotionTrack();
    return m_pRootMotionTrack;
}

void SkeletalAnimationGenerator::DeleteRootMotionTrack()
{
    delete m_pRootMotionTrack;
    m_pRootMotionTrack = nullptr;
}

void SkeletalAnimationGenerator::ClipAnimation(float startTime, float endTime)
{
    // clip bone tracks
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
        m_boneTracks[i]->ClipKeyFrames(startTime, endTime);

    // clip root motion track
    if (m_pRootMotionTrack != nullptr)
        m_pRootMotionTrack->ClipKeyFrames(startTime, endTime);
}

void SkeletalAnimationGenerator::Optimize(bool removeEmptyTracks /* = true */, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    pProgressCallbacks->SetProgressRange(m_boneTracks.GetSize());
    pProgressCallbacks->SetProgressValue(0);

    for (uint32 i = 0; i < m_boneTracks.GetSize();)
    {
        pProgressCallbacks->SetProgressValue(i);

        BoneTrack *track = m_boneTracks[i];
        uint32 oldKeyFrameCount = track->GetKeyFrameCount();

        track->Optimize();
        track->UpdateDuration();

        uint32 newKeyFrameCount = track->GetKeyFrameCount();
        
        pProgressCallbacks->DisplayFormattedInformation("SkeletalAnimationGenerator::Optimize: Bone '%s' from %u frames to %u frames", track->GetBoneName().GetCharArray(), oldKeyFrameCount, newKeyFrameCount);
        if (newKeyFrameCount == 0 && removeEmptyTracks)
        {
            m_boneTracks.OrderedRemove(i);
            delete track;
            continue;
        }
        else
        {
            i++;
            continue;
        }
    }

    if (m_pRootMotionTrack != nullptr)
        m_pRootMotionTrack->Optimize();

    UpdateDuration();
}

bool SkeletalAnimationGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
        return false;

    if (!xmlReader.SkipToElement("skeletal-animation"))
    {
        xmlReader.PrintError("could not skip to skeletal-animation element.");
        return false;
    }

    // get skeleton name
    const char *skeletonNameStr = xmlReader.FetchAttribute("skeleton");
    if (skeletonNameStr == nullptr)
    {
        xmlReader.PrintError("missing attributes");
        return false;
    }

    // set skeleton name
    m_skeletonName = skeletonNameStr;

    // move to element
    if (!xmlReader.MoveToElement())
        return false;

    // process xml nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 skeletalAnimationSelection = xmlReader.Select("properties|bone-track|root-motion-track");
            if (skeletalAnimationSelection < 0)
                return false;

            switch (skeletalAnimationSelection)
            {
                // properties
            case 0:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        if (!m_properties.LoadFromXML(xmlReader))
                            return false;

                        if (!xmlReader.ExpectEndOfElementName("properties"))
                            return false;
                    }
                }
                break;

                // bone-track
            case 1:
                {
                    const char *boneName = xmlReader.FetchAttribute("bone");
                    if (boneName == nullptr)
                    {
                        xmlReader.PrintError("missing attributes for bone track");
                        return false;
                    }

                    if (GetBoneTrackByName(boneName) != nullptr)
                    {
                        xmlReader.PrintError("duplicate bone track: %s", boneName);
                        return false;
                    }

                    // create it
                    BoneTrack *boneTrack = new BoneTrack(boneName);
                    m_boneTracks.Add(boneTrack);

                    // add bones
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 keyframesSelection = xmlReader.Select("keyframe");
                                if (keyframesSelection < 0)
                                    return false;

                                switch (keyframesSelection)
                                {
                                    // keyframe
                                case 0:
                                    {
                                        const char *timeStr = xmlReader.FetchAttribute("time");
                                        const char *positionStr = xmlReader.FetchAttribute("position");
                                        const char *rotationStr = xmlReader.FetchAttribute("rotation");
                                        const char *scaleStr = xmlReader.FetchAttribute("scale");

                                        if (timeStr == nullptr || positionStr == nullptr || rotationStr == nullptr || scaleStr == nullptr)
                                        {
                                            xmlReader.PrintError("missing keyframe attributes");
                                            return false;
                                        }

                                        // parse it
                                        float kfTime = StringConverter::StringToFloat(timeStr);
                                        float3 kfPosition(StringConverter::StringToFloat3(positionStr));
                                        Quaternion kfRotation(StringConverter::StringToQuaternion(rotationStr));
                                        float3 kfScale(StringConverter::StringToFloat3(scaleStr));

                                        // validate it
                                        if (kfTime < 0.0f)
                                        {
                                            xmlReader.PrintError("invalid keyframe");
                                            return false;
                                        }

                                        // create keyframe and add it to the list
                                        BoneTrack::KeyFrame keyFrame(kfTime, kfPosition, kfRotation, kfScale);
                                        boneTrack->m_keyFrames.Add(keyFrame);

                                        // skip element if it has contents
                                        if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                            return false;
                                    }
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "bone-track") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                return false;
                            }
                        }
                    }

                    // sort keyframes, and update the duration
                    boneTrack->SortKeyFrames();
                    boneTrack->UpdateDuration();
                }
                break;

                // root-motion-track
            case 2:
                {
                    if (m_pRootMotionTrack != nullptr)
                    {
                        xmlReader.PrintError("duplicate root motion track");
                        return false;
                    }

                    // create it
                    RootMotionTrack *rootMotionTrack = new RootMotionTrack();
                    m_pRootMotionTrack = rootMotionTrack;

                    // add bones
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 keyframesSelection = xmlReader.Select("keyframe");
                                if (keyframesSelection < 0)
                                    return false;

                                switch (keyframesSelection)
                                {
                                    // keyframe
                                case 0:
                                    {
                                        const char *timeStr = xmlReader.FetchAttribute("time");
                                        const char *positionStr = xmlReader.FetchAttribute("position");
                                        const char *rotationStr = xmlReader.FetchAttribute("rotation");
                                        const char *scaleStr = xmlReader.FetchAttribute("scale");

                                        if (timeStr == nullptr || positionStr == nullptr || rotationStr == nullptr || scaleStr == nullptr)
                                        {
                                            xmlReader.PrintError("missing keyframe attributes");
                                            return false;
                                        }

                                        // parse it
                                        float kfTime = StringConverter::StringToFloat(timeStr);
                                        float3 kfPosition(StringConverter::StringToFloat3(positionStr));
                                        Quaternion kfRotation(StringConverter::StringToQuaternion(rotationStr));
                                        float3 kfScale(StringConverter::StringToFloat3(scaleStr));

                                        // validate it
                                        if (kfTime < 0.0f)
                                        {
                                            xmlReader.PrintError("invalid keyframe");
                                            return false;
                                        }

                                        // create keyframe and add it to the list
                                        BoneTrack::KeyFrame keyFrame(kfTime, kfPosition, kfRotation, kfScale);
                                        rootMotionTrack->m_keyFrames.Add(keyFrame);

                                        // skip element if it has contents
                                        if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                            return false;
                                    }
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "root-motion-track") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                return false;
                            }
                        }
                    }

                    // sort keyframes, and update the duration
                    rootMotionTrack->SortKeyFrames();
                    rootMotionTrack->UpdateDuration();
                }
                break;

            default:
                UnreachableCode();
                break;
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "skeletal-animation") == 0);
            break;
        }
        else
        {
            xmlReader.PrintError("parse error");
            return false;
        }
    }

    UpdateDuration();
    return true;
}

bool SkeletalAnimationGenerator::SaveToXML(ByteStream *pStream) const
{
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("skeletal-animation");
    xmlWriter.WriteAttribute("skeleton", m_skeletonName);

    // write properties
    xmlWriter.StartElement("properties");
    m_properties.SaveToXML(xmlWriter);
    xmlWriter.EndElement();

    // write bone tracks
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
    {
        const BoneTrack *boneTrack = m_boneTracks[i];

        xmlWriter.StartElement("bone-track");
        xmlWriter.WriteAttribute("bone", boneTrack->GetBoneName());
        {
            // keyframes
            for (uint32 j = 0; j < boneTrack->m_keyFrames.GetSize(); j++)
            {
                const BoneTrack::KeyFrame *keyFrame = &boneTrack->m_keyFrames[j];

                xmlWriter.StartElement("keyframe");
                {
                    xmlWriter.WriteAttribute("time", StringConverter::FloatToString(keyFrame->GetTime()));
                    xmlWriter.WriteAttribute("position", StringConverter::Float3ToString(keyFrame->GetPosition()));
                    xmlWriter.WriteAttribute("rotation", StringConverter::QuaternionToString(keyFrame->GetRotation()));
                    xmlWriter.WriteAttribute("scale", StringConverter::Float3ToString(keyFrame->GetScale()));
                }
                xmlWriter.EndElement(); // </keyframe>
            }
        }
        xmlWriter.EndElement(); // </bone-track>
    }

    // write root motion track
    if (m_pRootMotionTrack != nullptr)
    {
        xmlWriter.StartElement("root-motion-track");
        {
            // keyframes
            for (uint32 i = 0; i < m_pRootMotionTrack->m_keyFrames.GetSize(); i++)
            {
                const BoneTrack::KeyFrame *keyFrame = &m_pRootMotionTrack->m_keyFrames[i];

                xmlWriter.StartElement("keyframe");
                {
                    xmlWriter.WriteAttribute("time", StringConverter::FloatToString(keyFrame->GetTime()));
                    xmlWriter.WriteAttribute("position", StringConverter::Float3ToString(keyFrame->GetPosition()));
                    xmlWriter.WriteAttribute("rotation", StringConverter::QuaternionToString(keyFrame->GetRotation()));
                    xmlWriter.WriteAttribute("scale", StringConverter::Float3ToString(keyFrame->GetScale()));
                }
                xmlWriter.EndElement(); // </keyframe>
            }
        }
        xmlWriter.EndElement(); // </root-motion-track>
    }

    xmlWriter.EndElement(); // </skeletal-animation>

    return (!xmlWriter.InErrorState() && !pStream->InErrorState());
}

void SkeletalAnimationGenerator::Copy(const SkeletalAnimationGenerator *pCopy)
{
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
        delete m_boneTracks[i];
    m_boneTracks.Obliterate();
    delete m_pRootMotionTrack;
    m_pRootMotionTrack = nullptr;

    m_skeletonName = pCopy->m_skeletonName;
    m_duration = pCopy->m_duration;

    for (uint32 i = 0; i < pCopy->m_boneTracks.GetSize(); i++)
    {
        BoneTrack *boneTrack = new BoneTrack(pCopy->m_boneTracks[i]->GetBoneName());
        boneTrack->m_duration = pCopy->m_boneTracks[i]->m_duration;
        boneTrack->m_keyFrames.Assign(pCopy->m_boneTracks[i]->m_keyFrames);
        m_boneTracks.Add(boneTrack);
    }

    if (pCopy->m_pRootMotionTrack != nullptr)
    {
        m_pRootMotionTrack = new RootMotionTrack();
        m_pRootMotionTrack->m_duration = pCopy->m_pRootMotionTrack->m_duration;
        m_pRootMotionTrack->m_keyFrames.Assign(pCopy->m_pRootMotionTrack->m_keyFrames);
    }
}

// we use recursion-based transform concatenation to preserve as much floating-point accuracy as possible
// static Transform CalculateAbsoluteBoneBaseFrameTransform(const Skeleton::Bone *pBone, const Transform *pTransformArray)
// {
//     if (pBone->GetParentBone() == nullptr)
//         return pTransformArray[pBone->GetIndex()];
// 
//     return Transform::ConcatenateTransforms(CalculateAbsoluteBoneBaseFrameTransform(pBone->GetParentBone()), pTransformArray[pBone->GetIndex()]);
// }

static float4x4 CalculateAbsoluteBoneBaseFrameTransform(const Skeleton::Bone *pBone, const Transform *pTransformArray)
{
    if (pBone->GetParentBone() == nullptr)
        return pTransformArray[pBone->GetIndex()].GetTransformMatrix4x4();
    else
        return CalculateAbsoluteBoneBaseFrameTransform(pBone->GetParentBone(), pTransformArray) * pTransformArray[pBone->GetIndex()].GetTransformMatrix4x4();
}

bool SkeletalAnimationGenerator::Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream, bool optimize /* = true */) const
{
    if (optimize)
    {
        SkeletalAnimationGenerator animationCopy;
        animationCopy.Copy(this);
        animationCopy.Optimize(true);
        return animationCopy.InternalCompile(pCallbacks, pStream);
    }
    else
    {
        return InternalCompile(pCallbacks, pStream);
    }
}

bool SkeletalAnimationGenerator::InternalCompile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const
{
    if (m_skeletonName.IsEmpty() || 
        (m_boneTracks.GetSize() == 0))
    {
        Log_ErrorPrintf("SkeletalAnimationGenerator::InternalCompile: Incomplete animation.");
        return false;
    }

    // load the skeleton we're using
    AutoReleasePtr<const Skeleton> pSkeleton = pCallbacks->GetCompiledSkeleton(m_skeletonName);
    if (pSkeleton == nullptr)
    {
        Log_ErrorPrintf("SkeletalAnimationGenerator::InternalCompile: Could not load Skeleton '%s'", m_skeletonName.GetCharArray());
        return false;
    }

    // build header
    DF_SKELETALANIMATION_HEADER fileHeader;
    fileHeader.Magic = DF_SKELETALANIMATION_HEADER_MAGIC;
    fileHeader.HeaderSize = sizeof(fileHeader);
    fileHeader.Flags = 0;
    fileHeader.SkeletonNameLength = m_skeletonName.GetLength();
    fileHeader.SkeletonNameOffset = 0xFFFFFFFF;
    fileHeader.SkeletonBoneCount = pSkeleton->GetBoneCount();
    fileHeader.Duration = m_duration;
    fileHeader.BoneTrackCount = 0;
    fileHeader.BoneTrackOffset = 0;
    fileHeader.RootMotionOffset = 0;

    // write header
    uint64 headerOffset = pStream->GetPosition();
    if (!pStream->Write2(&fileHeader, sizeof(fileHeader)))
        return false;

    // write skeleton name
    fileHeader.SkeletonNameOffset = (uint32)(pStream->GetPosition() - headerOffset);
    if (!pStream->Write2(m_skeletonName.GetCharArray(), m_skeletonName.GetLength()))
        return false;

    // write bone tracks
    if (m_boneTracks.GetSize() > 0)
    {
        fileHeader.BoneTrackOffset = (uint32)(pStream->GetPosition() - headerOffset);

        for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
        {
            const BoneTrack *pBoneTrack = m_boneTracks[i];
            const Skeleton::Bone *pBone = pSkeleton->GetBoneByName(pBoneTrack->GetBoneName());
            if (pBone == nullptr)
            {
                Log_ErrorPrintf("SkeletalAnimationGenerator::Compile: Animation references bone '%s' not found in skeleton.", pBoneTrack->GetBoneName().GetCharArray());
                return false;
            }

            // skip empty tracks
            if (pBoneTrack->GetKeyFrameCount() == 0)
                continue;

            // first keyframe should always have a time of zero
            if (pBoneTrack->GetKeyFrame(0)->GetTime() != 0.0f)
            {
                Log_ErrorPrintf("SkeletalAnimationGenerator::Compile: Bone '%s' does not have a keyframe at 0 seconds.", pBoneTrack->GetBoneName().GetCharArray());
                return false;
            }

            // create bone track header
            DF_SKELETALANIMATION_BONE_TRACK_HEADER boneTrackHeader;
            boneTrackHeader.BoneIndex = pBone->GetIndex();
            boneTrackHeader.Duration = pBoneTrack->GetDuration();
            boneTrackHeader.KeyFrameCount = pBoneTrack->GetKeyFrameCount();
            if (!pStream->Write2(&boneTrackHeader, sizeof(boneTrackHeader)))
                return false;

            // write keyframes
            for (uint32 keyFrameIndex = 0; keyFrameIndex < pBoneTrack->GetKeyFrameCount(); keyFrameIndex++)
            {
                const BoneTrack::KeyFrame *pKeyFrame = pBoneTrack->GetKeyFrame(keyFrameIndex);
                if (!InternalWriteTransformKeyFrame(pStream, pKeyFrame))
                    return false;
            }

            // increment bone track count
            fileHeader.BoneTrackCount++;
        }

        // reset offset if no tracks were written
        if (fileHeader.BoneTrackCount == 0)
            fileHeader.BoneTrackOffset = 0;
    }

    // write root motion track
    if (m_pRootMotionTrack != nullptr)
    {
        fileHeader.RootMotionOffset = (uint32)(pStream->GetPosition() - headerOffset);
        
        DF_SKELETALANIMATION_ROOT_MOTION_TRACK_HEADER rootMotionTrackHeader;
        rootMotionTrackHeader.Duration = m_pRootMotionTrack->GetDuration();
        rootMotionTrackHeader.KeyFrameCount = m_pRootMotionTrack->GetKeyFrameCount();
        if (!pStream->Write2(&rootMotionTrackHeader, sizeof(rootMotionTrackHeader)))
            return false;

        // write keyframes
        for (uint32 keyFrameIndex = 0; keyFrameIndex < m_pRootMotionTrack->GetKeyFrameCount(); keyFrameIndex++)
        {
            const BoneTrack::KeyFrame *pKeyFrame = m_pRootMotionTrack->GetKeyFrame(keyFrameIndex);
            if (!InternalWriteTransformKeyFrame(pStream, pKeyFrame))
                return false;
        }
    }

    // rewrite header
    uint64 endOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(headerOffset) || !pStream->Write2(&fileHeader, sizeof(fileHeader)) || !pStream->SeekAbsolute(endOffset))
        return false;

    // done
    return true;
}

bool SkeletalAnimationGenerator::InternalWriteTransformKeyFrame(ByteStream *pStream, const TransformTrack::KeyFrame *pKeyFrame)
{
    DF_SKELETALANIMATION_TRANSFORM_TRACK_KEYFRAME fileKeyFrame;
    fileKeyFrame.Time = pKeyFrame->GetTime();
    fileKeyFrame.Position[0] = pKeyFrame->GetPosition().x;
    fileKeyFrame.Position[1] = pKeyFrame->GetPosition().y;
    fileKeyFrame.Position[2] = pKeyFrame->GetPosition().z;
    fileKeyFrame.Rotation[0] = pKeyFrame->GetRotation().x;
    fileKeyFrame.Rotation[1] = pKeyFrame->GetRotation().y;
    fileKeyFrame.Rotation[2] = pKeyFrame->GetRotation().z;
    fileKeyFrame.Rotation[3] = pKeyFrame->GetRotation().w;
    fileKeyFrame.Scale[0] = pKeyFrame->GetScale().x;
    fileKeyFrame.Scale[1] = pKeyFrame->GetScale().y;
    fileKeyFrame.Scale[2] = pKeyFrame->GetScale().z;
    return pStream->Write2(&fileKeyFrame, sizeof(fileKeyFrame));
}

void SkeletalAnimationGenerator::GenerateKeyFrameTimeList(PODArray<float> *pKeyFrameTimeArray) const
{
    for (uint32 i = 0; i < m_boneTracks.GetSize(); i++)
    {
        const BoneTrack *boneTrack = m_boneTracks[i];
        for (uint32 j = 0; j < boneTrack->GetKeyFrameCount(); j++)
        {
            float keyFrameTime = boneTrack->GetKeyFrame(j)->GetTime();
            if (pKeyFrameTimeArray->IndexOf(keyFrameTime) < 0)
                pKeyFrameTimeArray->Add(keyFrameTime);
        }
    }

    pKeyFrameTimeArray->SortCB([](float pLeft, float pRight) { return Math::CompareResult(pLeft, pRight); });
}

// Interface
BinaryBlob *ResourceCompiler::CompileSkeletalAnimation(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.ska.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileSkeletalAnimation: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    SkeletalAnimationGenerator *pGenerator = new SkeletalAnimationGenerator();
    if (!pGenerator->LoadFromXML(sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    pStream->Release();
    pSourceData->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pCallbacks, pOutputStream))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
