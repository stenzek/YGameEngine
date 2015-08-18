#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/SkeletonGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/DataFormats.h"
#include "Core/ChunkFileWriter.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(SkeletonGenerator);

SkeletonGenerator::Bone::Bone(uint32 index, const char *name, Bone *pParent, const Transform &baseFrameTransform)
    : m_index(index),
      m_name(name),
      m_pParent(pParent),
      m_baseFrameTransform(baseFrameTransform)
{

}

SkeletonGenerator::Bone::~Bone()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SkeletonGenerator::SkeletonGenerator()
{

}

SkeletonGenerator::~SkeletonGenerator()
{
    for (uint32 i = 0; i < m_bones.GetSize(); i++)
        delete m_bones[i];
}

const SkeletonGenerator::Bone *SkeletonGenerator::GetBoneByName(const char *name) const
{
    for (uint32 i = 0; i < m_bones.GetSize(); i++)
    {
        if (m_bones[i]->GetName().Compare(name))
            return m_bones[i];
    }

    return NULL;
}

SkeletonGenerator::Bone *SkeletonGenerator::GetBoneByName(const char *name)
{
    for (uint32 i = 0; i < m_bones.GetSize(); i++)
    {
        if (m_bones[i]->GetName().Compare(name))
            return m_bones[i];
    }

    return NULL;
}

SkeletonGenerator::Bone *SkeletonGenerator::CreateBone(const char *name, Bone *pParent, const Transform &baseFrameTransform)
{
    if (GetBoneByName(name) != NULL)
        return NULL;

    Bone *pBone = new Bone(m_bones.GetSize(), name, pParent, baseFrameTransform);
    m_bones.Add(pBone);

    // add to parent's children
    if (pParent != NULL)
        pParent->m_children.Add(pBone);

    return pBone;
}

bool SkeletonGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
        return false;

    if (!xmlReader.SkipToElement("skeleton"))
    {
        xmlReader.PrintError("could not skip to skeleton element.");
        return false;
    }

    // process xml nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 skeletalMeshSelection = xmlReader.Select("properties|bones");
            if (skeletalMeshSelection < 0)
                return false;

            switch (skeletalMeshSelection)
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

                // bones
            case 1:
                {
                    for (;;)
                    {
                        if (!xmlReader.NextToken())
                            break;

                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                        {
                            int32 bonesSelection = xmlReader.Select("bone");
                            if (bonesSelection < 0)
                                return false;

                            const char *boneNameStr = xmlReader.FetchAttribute("name");
                            const char *parentStr = xmlReader.FetchAttribute("parent");
                            const char *baseFramePositionStr = xmlReader.FetchAttribute("baseframe-position");
                            const char *baseFrameRotationStr = xmlReader.FetchAttribute("baseframe-rotation");
                            const char *baseFrameScaleStr = xmlReader.FetchAttribute("baseframe-scale");

                            if (boneNameStr == NULL || baseFramePositionStr == NULL ||
                                baseFrameRotationStr == NULL || baseFrameScaleStr == NULL)
                            {
                                xmlReader.PrintError("missing bone fields");
                                return false;
                            }

                            float3 baseFramePosition = StringConverter::StringToFloat3(baseFramePositionStr);
                            Quaternion baseFrameRotation = StringConverter::StringToQuaternion(baseFrameRotationStr);
                            float3 baseFrameScale = StringConverter::StringToFloat3(baseFrameScaleStr);
                            Bone *pParentBone = NULL;
                            if (parentStr != NULL && (pParentBone = GetBoneByName(parentStr)) == NULL)
                            {
                                xmlReader.PrintError("bone '%s' has nonexistant parent '%s'", boneNameStr, parentStr);
                                return false;
                            }

                            Bone *pBone = new Bone(m_bones.GetSize(), boneNameStr, pParentBone, Transform(baseFramePosition, baseFrameRotation, baseFrameScale));
                            m_bones.Add(pBone);

                            if (pParentBone != NULL)
                                pParentBone->m_children.Add(pBone);

                            if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                return false;
                        }
                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                        {
                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "bones") == 0);
                            break;
                        }
                        else
                        {
                            xmlReader.PrintError("parse error");
                            break;
                        }
                    }
                }
                break;

            default:
                UnreachableCode();
                break;
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "skeleton") == 0);
            break;
        }
        else
        {
            xmlReader.PrintError("parse error");
            return false;
        }
    }

    return true;
}

bool SkeletonGenerator::SaveToXML(ByteStream *pStream) const
{
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("skeleton");
    {
        // write properties
        xmlWriter.StartElement("properties");
        m_properties.SaveToXML(xmlWriter);
        xmlWriter.EndElement();

        // write bones
        xmlWriter.StartElement("bones");
        {
            for (uint32 i = 0; i < m_bones.GetSize(); i++)
            {
                const Bone *pBone = m_bones[i];
                if (pBone == NULL)
                    continue;

                xmlWriter.StartElement("bone");
                {
                    xmlWriter.WriteAttribute("name", pBone->GetName());

                    if (pBone->GetParent() != NULL)
                        xmlWriter.WriteAttribute("parent", pBone->GetParent()->GetName());

                    xmlWriter.WriteAttribute("baseframe-position", StringConverter::Float3ToString(pBone->GetBaseFrameTransform().GetPosition()));
                    xmlWriter.WriteAttribute("baseframe-rotation", StringConverter::QuaternionToString(pBone->GetBaseFrameTransform().GetRotation()));
                    xmlWriter.WriteAttribute("baseframe-scale", StringConverter::Float3ToString(pBone->GetBaseFrameTransform().GetScale()));
                }
                xmlWriter.EndElement(); // </bone>
            }
        }
        xmlWriter.EndElement(); // </bones>
    }
    xmlWriter.EndElement(); // </skeleton>

    return (!xmlWriter.InErrorState() && !pStream->InErrorState());
}

// we use recursion-based transform concatenation to preserve as much floating-point accuracy as possible
// static Transform CalculateAbsoluteBoneBaseFrameTransform(const SkeletonGenerator::Bone *pBone)
// {
//     if (pBone->GetParent() == nullptr)
//         return pBone->GetBaseFrameTransform();
// 
//     return Transform::ConcatenateTransforms(CalculateAbsoluteBoneBaseFrameTransform(pBone->GetParent()), pBone->GetBaseFrameTransform());
// }

static float4x4 CalculateAbsoluteBoneBaseFrameTransform(const SkeletonGenerator::Bone *pBone)
{
    if (pBone->GetParent() == nullptr)
        return pBone->GetBaseFrameTransform().GetTransformMatrix4x4();
    else
        return CalculateAbsoluteBoneBaseFrameTransform(pBone->GetParent()) * pBone->GetBaseFrameTransform().GetTransformMatrix4x4();
}

bool SkeletonGenerator::Compile(ByteStream *pStream) const
{
    // build header
    DF_SKELETON_HEADER fileHeader;
    fileHeader.Magic = DF_SKELETON_HEADER_MAGIC;
    fileHeader.HeaderSize = sizeof(fileHeader);
    fileHeader.BoneCount = 0;
    fileHeader.BonesOffset = 0;

    // write header
    uint64 headerOffset = pStream->GetPosition();
    pStream->Write2(&fileHeader, sizeof(fileHeader));

    // write bones
    {
        // initialize an array for storing child bones
        uint32 *pChildBones = (uint32 *)alloca(sizeof(uint32) * m_bones.GetSize());
        uint32 nChildBones = 0;

        // set start offset
        fileHeader.BonesOffset = (uint32)(pStream->GetPosition() - headerOffset);
        
        for (uint32 boneIndex = 0; boneIndex < m_bones.GetSize(); boneIndex++)
        {
            const Bone *pBone = m_bones[boneIndex];

            // find child bones
            nChildBones = 0;
            for (uint32 subBoneIndex = 0; subBoneIndex < m_bones.GetSize(); subBoneIndex++)
            {
                if (m_bones[subBoneIndex]->GetParent() == pBone)
                    pChildBones[nChildBones++] = subBoneIndex;
            }

            DF_SKELETON_BONE fileBone;
            fileBone.BoneSize = sizeof(fileBone) + pBone->GetName().GetLength() + (sizeof(uint32) * nChildBones);
            fileBone.BoneNameLength = pBone->GetName().GetLength();
            fileBone.ParentBoneIndex = (pBone->GetParent() != nullptr) ? pBone->GetParent()->GetIndex() : 0xFFFFFFFF;
            fileBone.ChildBoneCount = nChildBones;
            
            // store relative transform
            const Transform &relativeBaseFrameTransform = pBone->GetBaseFrameTransform();
            relativeBaseFrameTransform.GetPosition().Store(fileBone.RelativeBaseFrameTransformPosition);
            relativeBaseFrameTransform.GetRotation().Store(fileBone.RelativeBaseFrameTransformRotation);
            relativeBaseFrameTransform.GetScale().Store(fileBone.RelativeBaseFrameTransformScale);

//             // calculate absolute transform
//             Transform absoluteBaseFrameTransform(CalculateAbsoluteBoneBaseFrameTransform(pBone));
//             absoluteBaseFrameTransform.GetPosition().Store(fileBone.AbsoluteBaseFrameTransformPosition);
//             absoluteBaseFrameTransform.GetRotation().Store(fileBone.AbsoluteBaseFrameTransformRotation);
//             absoluteBaseFrameTransform.GetScale().Store(fileBone.AbsoluteBaseFrameTransformScale);

            // calculate absolute transform
            float4x4 absoluteBaseFrameTransformMatrix(CalculateAbsoluteBoneBaseFrameTransform(pBone));
            Transform absoluteBaseFrameTransform(absoluteBaseFrameTransformMatrix);
            absoluteBaseFrameTransform.GetPosition().Store(fileBone.AbsoluteBaseFrameTransformPosition);
            absoluteBaseFrameTransform.GetRotation().Store(fileBone.AbsoluteBaseFrameTransformRotation);
            absoluteBaseFrameTransform.GetScale().Store(fileBone.AbsoluteBaseFrameTransformScale);

            // write bone header, and bone name
            if (!pStream->Write2(&fileBone, sizeof(fileBone)) || !pStream->Write2(pBone->GetName().GetCharArray(), pBone->GetName().GetLength()))
                return false;

            // write child bones
            if (nChildBones > 0 && !pStream->Write2(pChildBones, sizeof(uint32) * nChildBones))
                return false;

            // increment bone count
            fileHeader.BoneCount++;
        }
    }

    // rewrite the file header
    uint64 endOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(headerOffset) || !pStream->Write2(&fileHeader, sizeof(fileHeader)) || !pStream->SeekAbsolute(endOffset))
        return false;

    // ok
    return true;
}

// Interface
BinaryBlob *ResourceCompiler::CompileSkeleton(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.skl.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileSkeleton: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    SkeletonGenerator *pGenerator = new SkeletonGenerator();
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
    if (!pGenerator->Compile(pOutputStream))
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
