#pragma once
#include "Engine/Common.h"
#include "Engine/Skeleton.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/VertexFactories/SkeletalMeshVertexFactory.h"

class Material;
class Skeleton;

namespace Physics { class CollisionShape; }

#define SKELETAL_MESH_MAX_BONES_PER_VERTEX (4)

class SkeletalMesh : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(SkeletalMesh, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(SkeletalMesh);

public:
    struct Bone
    {
        uint32 SkeletonBoneIndex;
        Transform LocalToBoneTransform;
    };

    struct Batch
    {
        uint32 MaterialIndex;
        uint32 WeightCount;
        uint32 BaseBoneRef;
        uint32 BoneRefCount;
        uint32 BaseVertex;
        uint32 VertexCount;
        uint32 FirstIndex;
        uint32 IndexCount;
    };

public:
    SkeletalMesh(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    virtual ~SkeletalMesh();

    // skeleton
    const Skeleton *GetSkeleton() const { return m_pSkeleton; }

    // coordinates are for the base frame
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    // materials
    const uint32 GetMaterialCount() const { return m_materials.GetSize(); }
    const Material *GetMaterial(uint32 i) const { return m_materials[i]; }

    // bones
    const uint32 GetBoneCount() const { return m_bones.GetSize(); }
    const Bone *GetBone(uint32 i) const { return &m_bones[i]; }

    // bone refs
    const uint32 GetBoneRefCount() const { return m_boneRefs.GetSize(); }
    const uint16 GetBoneRef(uint32 i) const { return m_boneRefs[i]; }

    // vertices
    const uint32 GetVertexCount() const { return m_vertices.GetSize(); }
    const SkeletalMeshVertexFactory::Vertex *GetVertex(uint32 i) const { return &m_vertices[i]; }

    // indices
    const uint32 GetIndexCount() const { return m_indices.GetSize(); }
    const uint16 GetIndex(uint32 i) const { return m_indices[i]; }

    // batches
    const uint32 GetBatchCount() const { return m_batches.GetSize(); }
    const Batch *GetBatch(uint32 i) const { return &m_batches[i]; }

    // collision shape
    const Physics::CollisionShape *GetCollisionShape() const { return m_pCollisionShape; }

    // initialization
    bool LoadFromStream(const char *name, ByteStream *pStream);

    // gpu resources
    const VertexBufferBindingArray *GetVertexBuffers() const { return &m_vertexBuffers; }
    const uint32 GetBaseVertexFactoryFlags() const { return m_baseVertexFactoryFlags; }
    const uint32 GetBufferVertexFactoryFlags() const { return m_bufferVertexFactoryFlags; }
    GPUBuffer *GetIndexBuffer() const { return m_pIndexBuffer; }
    bool CreateGPUResources() const;
    void ReleaseGPUResources() const;
    bool CheckGPUResources() const;
    
private:
    const Skeleton *m_pSkeleton;

    AABox m_boundingBox;
    Sphere m_boundingSphere;

    MemArray<Bone> m_bones;

    PODArray<uint16> m_boneRefs;

    PODArray<const Material *> m_materials;

    MemArray<SkeletalMeshVertexFactory::Vertex> m_vertices;

    MemArray<uint16> m_indices;

    uint32 m_baseVertexFactoryFlags;
    MemArray<Batch> m_batches;

    Physics::CollisionShape *m_pCollisionShape;

    // gpu data
    mutable uint32 m_bufferVertexFactoryFlags;
    mutable VertexBufferBindingArray m_vertexBuffers;
    mutable GPUBuffer *m_pIndexBuffer;
    mutable bool m_GPUResourcesCreated;   
    DeclareNonCopyable(SkeletalMesh);
};

/*
class SkeletalMeshInstanceBoneData
{
public:
    SkeletalMeshInstanceBoneData(const SkeletalMesh *pSkeletalMesh);
    ~SkeletalMeshInstanceBoneData();

    const Transform &GetTransform(uint32 i) const { return m_boneTransforms[i]; }
    void SetTransform(uint32 i, const Transform &transform) { m_boneTransforms[i] = transform; }

    void InitializeForMesh(const SkeletalMesh *pSkeletalMesh);
    void ResetToBaseFrame(const SkeletalMesh *pSkeletalMesh);

private:
    MemArray<Transform> m_boneTransforms;
};
*/
