#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"

class Material;
class RenderWorld;
class GPUBuffer;
class StaticDecalRenderProxy;

class StaticDecal : private RenderProxy
{
    friend class DecalManager;

private:
    StaticDecal(const AABox &boundingBox, const Sphere &boundingSphere, const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance, float lifetime, uint32 entityID);
    StaticDecal(const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance, float lifetime, uint32 entityID);

public:
    ~StaticDecal();

    const float3 &GetPosition() const { return m_position; }
    const float3 &GetNormal() const { return m_normal; }
    const float2 &GetSize() const { return m_size; }
    const Material *GetMaterial() const { return m_pMaterial; }
    const float GetDrawDistance() const { return m_drawDistance; }
    const float GetLifeRemaining() const { return m_lifeRemaining; }

private:
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;

    void Rebuild(const float3 &position, const float3 &normal, const float2 &size);

private:
    float3 m_position;
    float3 m_normal;
    float2 m_size;
    const Material *m_pMaterial;
    float m_drawDistance;
    float m_lifeRemaining;

    typedef MemArray<LocalVertexFactory::Vertex> VertexArray;
    VertexArray m_vertices;
    GPUBuffer *m_pVertexBuffer;
};

class DecalManager
{
public:

public:
    DecalManager(RenderWorld *pRenderWorld);
    ~DecalManager();

    // static decals
    StaticDecal *CreateStaticDecal(const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance = Y_FLT_MAX, float lifetime = Y_FLT_INFINITE);
    void MoveStaticDecal(StaticDecal *pDecal, const float3 &position, const float3 &normal);
    void ResizeStaticDecal(StaticDecal *pDecal, const float3 &size);
    void RemoveStaticDecal(StaticDecal *pDecal);

    // static decal helper function
    StaticDecal *ProjectStaticDecal(const Ray &ray, const float2 &size, const Material *pMaterial, float drawDistance = Y_FLT_MAX, float lifetime = Y_FLT_INFINITE);

    // update
    void Tick(const float timeDifference);

private:
    RenderWorld *m_pRenderWorld;
    
    typedef PODArray<StaticDecal *> StaticDecalArray;
    StaticDecalArray m_staticDecals;
};

class DecalMeshGenerator
{
public:
    struct Triangle
    {
        struct Vertex
        {
            float3 Position;
            float2 TextureCoordinate;
        };
        Vertex Vertices[3];
        float3 Normal;
    };

    struct Polygon
    {
        struct Vertex
        {
            float3 Position;
            float2 TextureCoordinate;
        };
        MemArray<Vertex> Vertices;
        float3 Normal;
    };

public:
    DecalMeshGenerator(const float3 &position, const float3 &normal, const float width, const float height);
    ~DecalMeshGenerator();

    const AABox &GetDecalBox() const { return m_decalBox; }
    const MemArray<Triangle> &GetDecalTriangles() const { return m_decalTriangles; }
    const AABox &GetBoundingBox() const { return m_boundingBox; }

    uint32 GenerateDecalTriangles(const RenderProxy::IntersectingTriangleArray &inputTriangles);
    
private:
    void CalculateDecalFrame();
    void CalculateDecalBox();
    void CalculateBoundingBox();
    void CalculateClippingPlanes();
    void GenerateClippedTriangles(const RenderProxy::IntersectingTriangleArray &inputTriangles);

    float3 m_position;
    float3 m_normal;
    float m_width;
    float m_height;
    float m_depth;
    float3 m_rightVector;
    float3 m_upVector;
    AABox m_decalBox;
    Plane m_clippingPlanes[6];
    MemArray<Polygon> m_clippedPolygons;
    MemArray<Triangle> m_decalTriangles;
    AABox m_boundingBox;
};