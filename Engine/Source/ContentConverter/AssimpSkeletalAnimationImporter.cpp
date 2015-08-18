#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpSkeletalAnimationImporter.h"
#include "ContentConverter/AssimpSkeletonImporter.h"
#include "ContentConverter/AssimpCommon.h"
#include "ResourceCompiler/SkeletalAnimationGenerator.h"

using AssimpHelpers::AssimpVector3ToFloat3;
using AssimpHelpers::AssimpQuaternionToQuaternion;

AssimpSkeletalAnimationImporter::AssimpSkeletalAnimationImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(NULL),
      m_pLogStream(NULL),
      m_pScene(NULL)
{

}

AssimpSkeletalAnimationImporter::~AssimpSkeletalAnimationImporter()
{
    delete m_pLogStream;
    delete m_pImporter;
}

void AssimpSkeletalAnimationImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->CreateSkeleton = false;
    pOptions->ListOnly = false;
    pOptions->DefaultAnimationTicksPerSecond = 30.0f;
    pOptions->OverrideTicksPerSecond = 0.0f;
    pOptions->AllAnimations = false;
    pOptions->ClipAnimation = false;
    pOptions->ClipRangeStart = 0;
    pOptions->ClipRangeEnd = 0;
    pOptions->OptimizeAnimation = false;
}

bool AssimpSkeletalAnimationImporter::Execute()
{
    Timer timer;

    m_pProgressCallbacks->SetProgressValue(0);
    m_pProgressCallbacks->SetProgressRange(2);

    if (m_pOptions->SourcePath.IsEmpty() ||
        m_pOptions->OutputResourceName.IsEmpty() ||
        (m_pOptions->ClipAnimation && (m_pOptions->ClipRangeStart < 1 || m_pOptions->ClipRangeStart > m_pOptions->ClipRangeEnd)))
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

    m_pProgressCallbacks->PushState();
    timer.Reset();
    if (!LoadScene())
    {
        m_pProgressCallbacks->ModalError("LoadScene() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("LoadScene(): %.4f ms", timer.GetTimeMilliseconds());
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(1);

    ListAnimations();
    if (m_pOptions->ListOnly)
        return true;

    if (m_pOptions->CreateSkeleton)
    {
        timer.Reset();
        if (!CreateSkeleton())
        {
            m_pProgressCallbacks->ModalError("CreateSkeleton() failed. The log may contain more information as to why.");
            return false;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("CreateSkeleton(): %.4f ms", timer.GetTimeMilliseconds());
    }


    m_pProgressCallbacks->PushState();

    timer.Reset();
    if (!CreateAnimations())
    {
        m_pProgressCallbacks->ModalError("CreateAnimations() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateAnimations(): %.4f ms", timer.GetTimeMilliseconds());
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(2);

    return true;
}

bool AssimpSkeletalAnimationImporter::CreateSkeleton()
{
    String skeletonName = m_pOptions->SkeletonName;

    if (skeletonName.IsEmpty())
    {
        skeletonName = m_pOptions->OutputResourceName;
        m_pProgressCallbacks->DisplayFormattedError("Skeleton name not provided, using '%s'", skeletonName.GetCharArray());
    }

    AssimpSkeletonImporter::Options skeletonImportOptions;
    skeletonImportOptions.SourcePath = m_pOptions->SourcePath;
    skeletonImportOptions.OutputResourceName = skeletonName;

    AssimpSkeletonImporter skeletonImporter(&skeletonImportOptions, m_pProgressCallbacks);

    m_pProgressCallbacks->DisplayFormattedInformation("Importing skeleton from '%s' to '%s'", m_pOptions->SourcePath.GetCharArray(), skeletonName.GetCharArray());
    if (!skeletonImporter.Execute())
    {
        m_pProgressCallbacks->DisplayError("Skeleton import failed.");
        return false;
    }

    return true;
}

bool AssimpSkeletalAnimationImporter::LoadScene()
{
    m_pProgressCallbacks->SetStatusText("Loading scene...");

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
    uint32 postProcessFlags = AssimpHelpers::GetAssimpSkeletalAnimationImportPostProcessingFlags();

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

void AssimpSkeletalAnimationImporter::ListAnimations()
{
    for (uint32 i = 0; i < m_pScene->mNumAnimations; i++)
    {
        const aiAnimation *pAnimation = m_pScene->mAnimations[i];

        m_pProgressCallbacks->DisplayFormattedInformation("Animation '%s': %u channels/bones, %.2f TPS, %.2f duration (%.2f frames)",
                                                          pAnimation->mName.C_Str(),
                                                          pAnimation->mNumChannels,
                                                          pAnimation->mTicksPerSecond,
                                                          pAnimation->mDuration,
                                                          pAnimation->mDuration / ((pAnimation->mTicksPerSecond != 0.0f) ? pAnimation->mTicksPerSecond : (double)1.0f));
    }
}

bool AssimpSkeletalAnimationImporter::CreateAnimations()
{
    m_pProgressCallbacks->SetStatusText("Creating animations...");
    m_pProgressCallbacks->SetProgressRange((m_pOptions->AllAnimations) ? (uint32)m_pScene->mNumAnimations : (uint32)1);
    m_pProgressCallbacks->SetProgressValue(0);

    for (uint32 i = 0; i < m_pScene->mNumAnimations; i++)
    {
        const aiAnimation *pAnimation = m_pScene->mAnimations[i];

        if (!m_pOptions->AllAnimations)
        {
            // use filter name?
            if (m_pOptions->SingleAnimationName.GetLength() > 0)
            {
                if (!m_pOptions->SingleAnimationName.CompareInsensitive(pAnimation->mName.C_Str()))
                    continue;
            }

            // convert this animation to the output name
            return ParseAnimation(pAnimation, m_pOptions->OutputResourceName, m_pOptions->ClipAnimation, m_pOptions->ClipRangeStart, m_pOptions->ClipRangeEnd);
        }

        // importing all animations
        // generate the animation name
        SmallString cleanedResourceName;
        if (pAnimation->mName.length == 0)
        {
            // auto-generate name
            cleanedResourceName.Format("animation%u", i);
        }
        else
        {
            // use name
            cleanedResourceName.AppendString(pAnimation->mName.C_Str());
            FileSystem::SanitizeFileName(cleanedResourceName);
        }

        // generate resource name
        String outputResourceName;
        outputResourceName.Format("%s/%s%s", m_pOptions->OutputResourceDirectory.GetCharArray(), m_pOptions->OutputResourcePrefix.GetCharArray(), cleanedResourceName.GetCharArray());

        // parse it
        if (!ParseAnimation(m_pScene->mAnimations[i], outputResourceName, m_pOptions->ClipAnimation, m_pOptions->ClipRangeStart, m_pOptions->ClipRangeEnd))
            return false;

        m_pProgressCallbacks->SetProgressValue(i);
    }

    return true;
}

static aiVector3D FindAndInterpolateBoneVector(const double t, const aiVectorKey *pKeys, uint32 nKeys)
{
    DebugAssert(nKeys > 0);

    if (nKeys == 1)
    {
        // use first and only position
        return pKeys[0].mValue;
    }

    // find the key that we reside in with a time of <= t
    uint32 keyIndex;
    for (keyIndex = 0; keyIndex < (nKeys - 1); keyIndex++)
    {
        if (t < pKeys[keyIndex + 1].mTime)
            break;
    }

    // is this the last key?
    uint32 nextKeyIndex = keyIndex + 1;
    if (nextKeyIndex == nKeys)
    {
        // use this (last) key
        return pKeys[keyIndex].mValue;
    }

    // this case can happen when t is less than the first key
    if (t < pKeys[keyIndex].mTime)
    {
        // just use the first key
        return pKeys[keyIndex].mValue;
    }

    // get factor
    const double thisKeyTime = pKeys[keyIndex].mTime;
    const aiVector3D &thisKeyValue = pKeys[keyIndex].mValue;
    const double nextKeyTime = pKeys[nextKeyIndex].mTime;
    const aiVector3D &nextKeyValue = pKeys[nextKeyIndex].mValue;
    const double factor = (t - thisKeyTime) / (nextKeyTime - thisKeyTime);
    DebugAssert(factor >= 0.0 && factor <= 1.0);

    // factor == 0 or 1?
    if (Math::NearEqual(factor, 0.0, (double)Y_FLT_EPSILON))
        return thisKeyValue;
    else if (Math::NearEqual(factor, 1.0, (double)Y_FLT_EPSILON))
        return nextKeyValue;

    // interpolate between the two
    return thisKeyValue + ((nextKeyValue - thisKeyValue) * (float)factor);
}

static aiQuaternion FindAndInterpolateBoneQuaternion(const double t, const aiQuatKey *pKeys, uint32 nKeys)
{
    DebugAssert(nKeys > 0);

    if (nKeys == 1)
    {
        // use first and only position
        return pKeys[0].mValue;
    }

    // find the key that we reside in with a time of <= t
    uint32 keyIndex;
    for (keyIndex = 0; keyIndex < (nKeys - 1); keyIndex++)
    {
        if (t < pKeys[keyIndex + 1].mTime)
            break;
    }

    // is this the last key?
    uint32 nextKeyIndex = keyIndex + 1;
    if (nextKeyIndex == nKeys)
    {
        // use this (last) key
        return pKeys[keyIndex].mValue;
    }

    // this case can happen when t is less than the first key
    if (t < pKeys[keyIndex].mTime)
    {
        // just use the first key
        return pKeys[keyIndex].mValue;
    }

    // get factor
    const double thisKeyTime = pKeys[keyIndex].mTime;
    const aiQuaternion &thisKeyValue = pKeys[keyIndex].mValue;
    const double nextKeyTime = pKeys[nextKeyIndex].mTime;
    const aiQuaternion &nextKeyValue = pKeys[nextKeyIndex].mValue;
    const double factor = (t - thisKeyTime) / (nextKeyTime - thisKeyTime);
    DebugAssert(factor >= 0.0 && factor <= 1.0);

    // factor == 0 or 1?
    if (Math::NearEqual(factor, 0.0, (double)Y_FLT_EPSILON))
        return thisKeyValue;
    else if (Math::NearEqual(factor, 1.0, (double)Y_FLT_EPSILON))
        return nextKeyValue;

    // interpolate between the two
    aiQuaternion interpolatedRotation;
    aiQuaternion::Interpolate(interpolatedRotation, thisKeyValue, nextKeyValue, (float)factor);
    return interpolatedRotation.Normalize();
}

bool AssimpSkeletalAnimationImporter::ParseAnimation(const aiAnimation *pAnimation, const char *outputResourceName, bool clipAnimation /* = false */, uint32 clipKeyFrameStart /* = 0 */, uint32 clipKeyFrameEnd /* = 0 */)
{
    // get duration in ticks
    double animationDurationInTicks = pAnimation->mDuration;
    double animationTicksPerSecond = (pAnimation->mTicksPerSecond <= 0.0) ? (double)m_pOptions->DefaultAnimationTicksPerSecond : pAnimation->mTicksPerSecond;
    if (m_pOptions->OverrideTicksPerSecond > 0.0f)
        animationTicksPerSecond = (double)m_pOptions->OverrideTicksPerSecond;

    // determine the number of frames to be generated
    //double animationFramesGeneratedF = animationDurationInTicks / animationTicksPerSecond;
    //uint32 maximumKeyFrameCount = (uint32)Math::Truncate((float)animationFramesGeneratedF);
    //if (!Math::NearEqual(Math::FractionalPart((float)animationFramesGeneratedF), 0.0f, Y_FLT_EPSILON))
        //m_pProgressCallbacks->DisplayFormattedWarning("animation '%s' forms incomplete frame count (%f)", pAnimation->mName.C_Str(), animationFramesGeneratedF);

    // logging
    m_pProgressCallbacks->SetProgressRange(pAnimation->mNumChannels);
    m_pProgressCallbacks->SetProgressValue(0);
    m_pProgressCallbacks->DisplayFormattedInformation("animation '%s' has %u tracks in file", pAnimation->mName.C_Str(), pAnimation->mNumChannels);
    m_pProgressCallbacks->DisplayFormattedInformation("animation '%s' has a duration of %f ticks, or %f seconds", pAnimation->mName.C_Str(), animationDurationInTicks, animationDurationInTicks / animationTicksPerSecond);
    //m_pProgressCallbacks->DisplayFormattedInformation("animation '%s' will have a maximum of %u keyframes", pAnimation->mName.C_Str(), maximumKeyFrameCount);

    // create generator
    SkeletalAnimationGenerator animationGenerator;
    animationGenerator.SetSkeletonName(m_pOptions->SkeletonName);

    // create bone tracks
    for (uint32 channelIndex = 0; channelIndex < pAnimation->mNumChannels; channelIndex++)
    {
        const aiNodeAnim *pNodeAnim = pAnimation->mChannels[channelIndex];

        // logging
        m_pProgressCallbacks->SetProgressValue(channelIndex);
        m_pProgressCallbacks->SetFormattedStatusText("Processing bone '%s'", pNodeAnim->mNodeName.C_Str());

        // determine # of keyframes
        uint32 keyFrameCount = Max((uint32)pNodeAnim->mNumPositionKeys, Max((uint32)pNodeAnim->mNumRotationKeys, (uint32)pNodeAnim->mNumScalingKeys));
        m_pProgressCallbacks->DisplayFormattedInformation("bone '%s' has %u keyframes in file", pNodeAnim->mNodeName.C_Str(), keyFrameCount);

        // shouldn't happen
        if (keyFrameCount == 0)
        {
            m_pProgressCallbacks->DisplayFormattedWarning("no key frames, ignoring channel");
            continue;
        }

        // check for dupes, shouldn't happen
        if (animationGenerator.GetBoneTrackByName(pNodeAnim->mNodeName.C_Str()) != nullptr)
        {
            m_pProgressCallbacks->DisplayFormattedError("a track named '%s' already exists", pNodeAnim->mNodeName.C_Str());
            continue;
        }

        // allocate track
        SkeletalAnimationGenerator::BoneTrack *boneTrack = new SkeletalAnimationGenerator::BoneTrack(pNodeAnim->mNodeName.C_Str());

        // handle case where position count != rotation count != scale count
        if (keyFrameCount != pNodeAnim->mNumPositionKeys || keyFrameCount != pNodeAnim->mNumRotationKeys || pNodeAnim->mNumScalingKeys != keyFrameCount)
            m_pProgressCallbacks->DisplayFormattedWarning("position keys %u / rotation keys %u / scale keys %u - mismatched, interpolation will be performed", pNodeAnim->mNumPositionKeys, pNodeAnim->mNumRotationKeys, pNodeAnim->mNumScalingKeys);

        // handle case where the first timestamp is not zero
        double subtractTime = 0.0;
        if (pNodeAnim->mPositionKeys[0].mTime != 0.0)
        {
            subtractTime = Max(pNodeAnim->mPositionKeys[0].mTime, subtractTime);
            m_pProgressCallbacks->DisplayFormattedWarning("first position key has a nonzero time (%.3f), new subtract time = %.3f", pNodeAnim->mPositionKeys[0].mTime, subtractTime);
        }
        if (pNodeAnim->mRotationKeys[0].mTime != 0.0)
        {
            subtractTime = Max(pNodeAnim->mRotationKeys[0].mTime, subtractTime);
            m_pProgressCallbacks->DisplayFormattedWarning("first rotation key has a nonzero time (%.3f), new subtract time = %.3f", pNodeAnim->mRotationKeys[0].mTime, subtractTime);
        }
        if (pNodeAnim->mScalingKeys[0].mTime != 0.0)
        {
            subtractTime = Max(pNodeAnim->mScalingKeys[0].mTime, subtractTime);
            m_pProgressCallbacks->DisplayFormattedWarning("first scale key has a nonzero time (%.3f), new subtract time = %.3f", pNodeAnim->mScalingKeys[0].mTime, subtractTime);
        }

        // collect unique timestamps of keys
        PODArray<double> keyFrameTimeStamps;
        keyFrameTimeStamps.Reserve(keyFrameCount + 1);

        // ensure a zero key gets allocated
        keyFrameTimeStamps.Add(0.0);

        // for positions
        for (uint32 keyIndex = 0; keyIndex < pNodeAnim->mNumPositionKeys; keyIndex++)
        {
            double timeStamp = Max(pNodeAnim->mPositionKeys[keyIndex].mTime - subtractTime, 0.0);
            if (keyFrameTimeStamps.IndexOf(timeStamp) < 0)
                keyFrameTimeStamps.Add(timeStamp);
        }

        // for rotations
        for (uint32 keyIndex = 0; keyIndex < pNodeAnim->mNumRotationKeys; keyIndex++)
        {
            double timeStamp = Max(pNodeAnim->mRotationKeys[keyIndex].mTime - subtractTime, 0.0);
            if (keyFrameTimeStamps.IndexOf(timeStamp) < 0)
                keyFrameTimeStamps.Add(timeStamp);
        }

        // for scales
        for (uint32 keyIndex = 0; keyIndex < pNodeAnim->mNumScalingKeys; keyIndex++)
        {
            double timeStamp = Max(pNodeAnim->mScalingKeys[keyIndex].mTime - subtractTime, 0.0);
            if (keyFrameTimeStamps.IndexOf(timeStamp) < 0)
                keyFrameTimeStamps.Add(timeStamp);
        }

        // log the new key frame count
        keyFrameTimeStamps.SortCB([](double left, double right) { return (left < right) ? -1 : ((left > right) ? 1 : 0); });
        m_pProgressCallbacks->DisplayFormattedInformation("after collection, keyframes %u -> %u, min %.2f, max %.2f", keyFrameCount, keyFrameTimeStamps.GetSize(), keyFrameTimeStamps[0], keyFrameTimeStamps[keyFrameTimeStamps.GetSize() - 1]);

        // create key frames
        for (uint32 keyFrameIndex = 0; keyFrameIndex < keyFrameTimeStamps.GetSize(); keyFrameIndex++)
        {
            // keyframe time in ticks
            double keyFrameTimeInTicks = keyFrameTimeStamps[keyFrameIndex];

            // collect transform
            aiVector3D keyFramePosition(FindAndInterpolateBoneVector(keyFrameTimeInTicks + subtractTime, pNodeAnim->mPositionKeys, pNodeAnim->mNumPositionKeys));
            aiQuaternion keyFrameRotation(FindAndInterpolateBoneQuaternion(keyFrameTimeInTicks + subtractTime, pNodeAnim->mRotationKeys, pNodeAnim->mNumRotationKeys));
            aiVector3D keyFrameScale(FindAndInterpolateBoneVector(keyFrameTimeInTicks + subtractTime, pNodeAnim->mScalingKeys, pNodeAnim->mNumScalingKeys));

            // find the time in seconds
            double keyFrameTimeInSeconds = keyFrameTimeInTicks / animationTicksPerSecond;

            // create key frame
            boneTrack->AddKeyFrame((float)keyFrameTimeInSeconds, AssimpVector3ToFloat3(keyFramePosition), AssimpQuaternionToQuaternion(keyFrameRotation), AssimpVector3ToFloat3(keyFrameScale));
        }

        // clip the sequence
        if (clipAnimation)
        {
            // calculate clip time range
            float clipTimeStart = static_cast<float>(static_cast<double>(clipKeyFrameStart - 1) / animationTicksPerSecond);
            float clipTimeEnd = static_cast<float>(static_cast<double>(clipKeyFrameEnd - 1) / animationTicksPerSecond);
            boneTrack->ClipKeyFrames(clipTimeStart, clipTimeEnd);

            // logging
            m_pProgressCallbacks->DisplayFormattedInformation("channel '%s' clip range %.2f -> %.2f, %u -> %u keyframes", pNodeAnim->mNodeName.C_Str(), clipTimeStart, clipTimeEnd, keyFrameTimeStamps.GetSize(), boneTrack->GetKeyFrameCount());

            // discard the track if it has no frames left
            if (boneTrack->GetKeyFrameCount() == 0)
            {
                m_pProgressCallbacks->DisplayFormattedWarning("discarding channel '%s' as it has no key frames after clipping", pNodeAnim->mNodeName.C_Str());
                delete boneTrack;
                continue;
            }
        }

        // add the track
        animationGenerator.AddBoneTrack(boneTrack);
    }

    // optimize the sequence
    if (m_pOptions->OptimizeAnimation)
    {
        m_pProgressCallbacks->PushState();

        animationGenerator.Optimize(true);

        m_pProgressCallbacks->PopState();
        m_pProgressCallbacks->SetProgressValue(pAnimation->mNumChannels);
    }

    // if we didn't get any keyframes abort
    if (animationGenerator.GetBoneTrackCount() == 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("animation '%s' produced no tracks", pAnimation->mName.C_Str());
        return false;
    }

    m_pProgressCallbacks->SetStatusText("Writing animation...");

    // output filename
    PathString outputResourceFileName;
    outputResourceFileName.Format("%s.ska.xml", outputResourceName);

    ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputResourceFileName,
                                                         BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | 
                                                         BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | 
                                                         BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("could not open file '%s'", outputResourceFileName.GetCharArray());
        return false;
    }

    // save it
    if (!animationGenerator.SaveToXML(pStream))
    {
        m_pProgressCallbacks->DisplayFormattedError("failed to save xml file '%s'", outputResourceFileName.GetCharArray());
        pStream->Discard();
        pStream->Release();
        return false;
    }

    pStream->Commit();
    pStream->Release();
    return true;
}
