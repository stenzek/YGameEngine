#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/SkeletalMesh.h"

struct ResourceCompilerCallbacks;

namespace Physics { class CollisionShapeGenerator; }

class SkeletalMeshGenerator
{
public:
    struct Bone 
    {
        uint32 Index;
        String Name;
        Transform LocalToBoneTransform;

        Bone(uint32 index, const char *name, const Transform &localToBoneTransform) : Index(index), Name(name), LocalToBoneTransform(localToBoneTransform) {}
    };

    struct Vertex
    {
        float3 Position;
        float3 Tangent;
        float3 Binormal;
        float3 Normal;
        float2 TextureCoordinates;
        uint32 Color;
        int32 BoneIndices[SKELETAL_MESH_MAX_BONES_PER_VERTEX];
        float BoneWeights[SKELETAL_MESH_MAX_BONES_PER_VERTEX];
    };

    struct Triangle
    {
        uint32 MaterialIndex;
        uint64 SmoothingGroups;
        uint32 VertexIndices[3];
    };

//     struct Batch
//     {
//         uint32 MaterialIndex;
//         uint32 WeightCount;
//         uint32 FirstTriangle;
//         uint32 TriangleCount;
//     };

public:
    SkeletalMeshGenerator();
    ~SkeletalMeshGenerator();

    // skeleton
    const String &GetSkeletonName() const { return m_skeletonName; }
    void SetSkeletonName(const char *skeletonName) { m_skeletonName = skeletonName; }

    // flags
    bool GetProvideVertexTextureCoordinates() const { return m_provideVertexTextureCoordinates; }
    void SetProvideVertexTextureCoordinates(bool on) { m_provideVertexTextureCoordinates = on; }
    bool GetProvideVertexColors() const { return m_provideVertexTextureCoordinates; }
    void SetProvideVertexColors(bool on) { m_provideVertexTextureCoordinates = on; }

    // bones
    const uint32 GetBoneNameCount() const { return m_bones.GetSize(); }
    const Bone *GetBoneByIndex(uint32 i) const { return &m_bones[i]; }
    const int32 FindBoneIndexByName(const char *boneName) const;
    Bone *GetBoneByIndex(uint32 i) { return &m_bones[i]; }
    Bone *AddBone(const char *boneName, const Transform &localToBoneTransform);

    // materials
    const uint32 GetMaterialNameCount() const { return m_materialNames.GetSize(); }
    const String &GetMaterialNameByIndex(uint32 i) const { return m_materialNames[i]; }
    const int32 FindMaterialNameIndex(const char *materialName) const;
    const uint32 AddMaterialName(const char *materialName);

    // add already-complete vertex
    uint32 AddVertex(const Vertex &vertex);

    // add a vertex without any bones yet
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates);
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &normal);
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &tangent, const float3 &binormal, const float3 &normal);

    // coordinates are assumed to be in local space
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates, const int32 *pBoneIndices, const float *pBoneWeights);
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &normal, const int32 *pBoneIndices, const float *pBoneWeights);
    uint32 AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &tangent, const float3 &binormal, const float3 &normal, const int32 *pBoneIndices, const float *pBoneWeights);

    // triangles
    uint32 AddTriangle(uint32 materialNameIndex, uint64 smoothingGroups, uint32 v0, uint32 v1, uint32 v2);
    uint32 AddTriangle(uint32 materialNameIndex, uint64 smoothingGroups, const uint32 *pIndices);

    // tangent space building
    void CalculateTangentVectors();
    void CalculateTangentVectorsAndNormals();

    // Loading interface (from XML)
    bool LoadFromXML(const char *FileName, ByteStream *pStream);

    // Output interface
    bool SaveToXML(ByteStream *pStream) const;
    bool Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const;

    // Collision shapes
    bool BuildBoxCollisionShape();
    bool BuildSphereCollisionShape();
    bool BuildTriangleMeshCollisionShape();
    bool BuildConvexHullCollisionShape();
    void SetCollisionShape(Physics::CollisionShapeGenerator *pGenerator);
    void RemoveCollisionShape();

private:
    bool m_provideVertexTextureCoordinates;
    bool m_provideVertexColors;
    String m_skeletonName;

    Array<Bone> m_bones;
    Array<String> m_materialNames;

    MemArray<Vertex> m_vertices;
    MemArray<Triangle> m_triangles;

    Physics::CollisionShapeGenerator *m_pCollisionShapeGenerator;
};
