#pragma once
#include "Engine/Common.h"

class Skeleton : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(Skeleton, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(Skeleton);

public:
    class Bone
    {
        friend class Skeleton;

    public:
        Bone() : m_index(0xFFFFFFFF), m_pParentBone(nullptr) {}

        const uint32 GetIndex() const { return m_index; }
        const String &GetName() const { return m_name; }
        const Bone *GetParentBone() const { return m_pParentBone; }
        const Transform &GetRelativeBaseFrameTransform() const { return m_relativeBaseFrameTransform; }
        const Transform &GetAbsoluteBaseFrameTransform() const { return m_absoluteBaseFrameTransform; }
        const Bone *GetChildBone(uint32 childIndex) const { return m_childBones[childIndex]; }
        const uint32 GetChildBoneCount() const { return m_childBones.GetSize(); }

    private:
        uint32 m_index;
        const Bone *m_pParentBone;
        String m_name;
        Transform m_relativeBaseFrameTransform;
        Transform m_absoluteBaseFrameTransform;
        PODArray<const Bone *> m_childBones;
    };

public:
    Skeleton(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    virtual ~Skeleton();

    // bone info
    const uint32 GetBoneCount() const { return m_bones.GetSize(); }
    const Bone *GetRootBone() const { return &m_bones[0]; }
    const Bone *GetBoneByIndex(uint32 boneIndex) const { return &m_bones[boneIndex]; }
    const Bone *GetBoneByName(const char *boneName) const;

    // initialization
    bool LoadFromStream(const char *name, ByteStream *pStream);

private:
    Array<Bone> m_bones;
};

