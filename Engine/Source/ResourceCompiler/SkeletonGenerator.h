#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/SkeletalMesh.h"
#include "Core/PropertyTable.h"

class SkeletonGenerator
{
public:
    class Bone
    {
        friend class SkeletonGenerator;

    public:
        Bone(uint32 index, const char *name, Bone *pParent, const Transform &baseFrameTransform);
        ~Bone();

        const uint32 GetIndex() const { return m_index; }
        const String &GetName() const { return m_name; }
        const Bone *GetParent() const { return m_pParent; }
        const Transform &GetBaseFrameTransform() const { return m_baseFrameTransform; }

        void SetName(const char *name) { m_name = name; }
        void SetParent(Bone *pParent) { m_pParent = pParent; }
        void SetBaseFrameTransform(const Transform &transform) { m_baseFrameTransform = transform; }

        Bone *GetChild(uint32 i) { return m_children[i]; }
        uint32 GetChildCount() const { return m_children.GetSize(); }
        void AddChild(Bone *pBone) { m_children.Add(pBone); }

    private:
        uint32 m_index;
        String m_name;
        Bone *m_pParent;
        Transform m_baseFrameTransform;

        PODArray<Bone *> m_children;

        DeclareNonCopyable(Bone);
    };

public:
    SkeletonGenerator();
    ~SkeletonGenerator();

    // properties
    const PropertyTable *GetPropertyTable() const { return &m_properties; }
    
    // bones
    const uint32 GetBoneCount() const { return m_bones.GetSize(); }
    const Bone *GetBoneByIndex(uint32 i) const { return m_bones[i]; }
    const Bone *GetBoneByName(const char *name) const;
    Bone *GetBoneByIndex(uint32 i) { return m_bones[i]; }
    Bone *GetBoneByName(const char *name);
    Bone *CreateBone(const char *name, Bone *pParent, const Transform &baseFrameTransform);

    // Loading interface (from XML)
    bool LoadFromXML(const char *FileName, ByteStream *pStream);

    // Output interface
    bool SaveToXML(ByteStream *pStream) const;
    bool Compile(ByteStream *pStream) const;

private:
    PropertyTable m_properties;
    PODArray<Bone *> m_bones;
};

