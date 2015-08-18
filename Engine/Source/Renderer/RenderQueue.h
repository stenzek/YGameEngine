#pragma once
#include "Renderer/Common.h"

class RenderProxy;
class VertexFactoryTypeInfo;
class Material;
class GPUTexture;
class GPUQuery;

enum RENDER_QUEUE_LAYER
{
    RENDER_QUEUE_LAYER_SKYBOX       = 0,            // skybox

    RENDER_QUEUE_LAYER_NONE         = 1,            // everything else

    RENDER_QUEUE_LAYER_DECALS       = 2,
};

struct RENDER_QUEUE_RENDERABLE_ENTRY
{
    uint64 SortKey;

    const RenderProxy *pRenderProxy;
    const VertexFactoryTypeInfo *pVertexFactoryTypeInfo;
    const Material *pMaterial;
    void *UserDataPointer[2];

    AABox BoundingBox;

    uint32 RenderPassMask;
    uint32 VertexFactoryFlags;
    
    float ViewDistance;
    uint32 TintColor;
    uint32 UserData[4];
    uint8 Layer;

    // predicate to apply when drawing
    GPUQuery *pPredicate;

    // align to 2 cache lines
#if defined(Y_CPU_X86)
    //byte __padding__[32];       // 128-96
#elif defined(Y_CPU_X64)
    //byte __padding__[20];       // 128-108
#endif

    // start cleared
    inline RENDER_QUEUE_RENDERABLE_ENTRY() { Y_memzero(this, sizeof(RENDER_QUEUE_RENDERABLE_ENTRY)); }
};

struct RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY
{
    float3 LightColor;
    float3 AmbientColor;
    float3 Direction;
    uint32 ShadowFlags;
    int32 ShadowMapIndex;
    bool Static;
};

struct RENDER_QUEUE_POINT_LIGHT_ENTRY
{
    float3 Position;
    float Range;
    float InverseRange;
    float3 LightColor;
    float FalloffExponent;
    uint32 ShadowFlags;
    int32 ShadowMapIndex;
    bool Static;
};

struct RENDER_QUEUE_SPOT_LIGHT_ENTRY
{
    float3 Position;
    float Range;
    float InverseRange;
    float3 LightColor;
    float Theta;
    float Phi;
    float Falloff;
    uint32 ShadowFlags;
    int32 ShadowMapIndex;
    bool Static;
};

struct RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY
{
    float3 Position;
    float Range;
    float3 LightColor;
    VOLUMETRIC_LIGHT_PRIMITIVE Primitive;
    float FalloffRate;
    float3 BoxExtents;
    float SphereRadius;
    bool Static;
};

struct RENDER_QUEUE_OCCLUDER_ENTRY
{
    const RenderProxy *pRenderProxy;
    void *UserDataPointer[2];
    uint32 UserData[4];
    bool MatchUserData;

    // todo: provide a mesh?
    AABox BoundingBox;

    // start cleared
    inline RENDER_QUEUE_OCCLUDER_ENTRY() { Y_memzero(this, sizeof(RENDER_QUEUE_OCCLUDER_ENTRY)); }
};

class RenderQueue
{
public:
    typedef MemArray<RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY> DirectionalLightArray;
    typedef MemArray<RENDER_QUEUE_POINT_LIGHT_ENTRY> PointLightArray;
    typedef MemArray<RENDER_QUEUE_SPOT_LIGHT_ENTRY> SpotLightArray;
    typedef MemArray<RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY> VolumetricLightArray;
    typedef MemArray<RENDER_QUEUE_RENDERABLE_ENTRY> RenderableArray;
    typedef MemArray<RENDER_QUEUE_OCCLUDER_ENTRY> OccluderArray;
    typedef PODArray<const RenderProxy *> DebugDrawRenderableArray;

public:
    RenderQueue();
    ~RenderQueue();

    // accepted object draw mask
    const bool IsAcceptingLights() const { return m_acceptingLights; }
    const uint32 GetAcceptingRenderPassMask() const { return m_acceptingRenderPassMask; }
    const bool IsAcceptingDebugObjects() const { return m_acceptingDebugObjects; }
    const bool IsAcceptingOccluders() const { return m_acceptingOccluders; }
    void SetAcceptingLights(bool enabled) { m_acceptingLights = enabled; }
    void SetAcceptingRenderPassMask(uint32 mask) { m_acceptingRenderPassMask = mask; }
    void SetAcceptingOccluders(bool enabled) { m_acceptingOccluders = enabled; }
    void SetAcceptingDebugObjects(bool enabled) { m_acceptingDebugObjects = enabled; }

    // Adds a batch to the draw queue.
    // Does not perform sorting.
    void AddLight(const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLightEntry);
    void AddLight(const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLightEntry);
    void AddLight(const RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLightEntry);
    void AddLight(const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLightEntry);
    void AddRenderable(const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);
    void AddOccluder(const RENDER_QUEUE_OCCLUDER_ENTRY *pOccluderEntry);
    void AddOccluder(const RenderProxy *pRenderProxy, const AABox &boundingBox);
    void AddOccluder(const RenderProxy *pRenderProxy, const AABox &boundingBox, const uint32 userData[], const void *const userDataPointer[]);
    void AddDebugInfoObject(const RenderProxy *pRenderProxy);

    // remove a render proxy from the queue
    void InvalidateOpaqueRenderProxy(const RenderProxy *pRenderProxy);
    void InvalidateOpaqueRenderProxy(const RenderProxy *pRenderProxy, const uint32 userData[], const void *const userDataPointer[]);

    // mark queue entries with a query result
    void MarkRenderProxyWithPredicate(const RenderProxy *pRenderProxy, GPUQuery *pPredicate);
    void MarkRenderProxyWithPredicate(const RenderProxy *pRenderProxy, const uint32 userData[], const void *const userDataPointer[], GPUQuery *pPredicate);

    // Re-sorts the render queue for optimal performance.
    void Sort();

    // Clears the render queue.
    void Clear();

    // External Access
    const uint32 GetQueueSize() const { return m_queueSize; }
    const uint32 GetNumObjectsInvalidatedByOcclusion() const { return m_numObjectsInvalidatedByOcclusion; }

    // lights
    DirectionalLightArray &GetDirectionalLightArray() { return m_directionalLightArray; }
    const uint32 GetDirectionalLightCount() const { return m_directionalLightArray.GetSize(); }
    PointLightArray &GetPointLightArray() { return m_pointLightArray; }
    const uint32 GetPointLightCount() const { return m_pointLightArray.GetSize(); }
    SpotLightArray &GetSpotLightArray() { return m_spotLightArray; }
    const uint32 GetSpotLightCount() const { return m_spotLightArray.GetSize(); }
    VolumetricLightArray &GetVolumetricLightArray() { return m_volumetricLightArray; }
    const uint32 GetVolumetricLightCount() const { return m_volumetricLightArray.GetSize(); }

    // opaque objects (both unlit and lit)
    RenderableArray &GetOpaqueRenderables() { return m_opaqueRenderables; }
    const uint32 GetOpaqueRenderableCount() const { return m_opaqueRenderables.GetSize(); }

    // translucent objects (both unlit and lit)
    RenderableArray &GetTranslucentRenderables() { return m_translucentRenderables; }
    const uint32 GetTranslucentRenderableCount() const { return m_translucentRenderables.GetSize(); }

    // post process renderables
    RenderableArray &GetPostProcessRenderables() { return m_postProcessRenderables; }
    const uint32 GetPostProcessRenderableCount() const { return m_postProcessRenderables.GetSize(); }

    // occlusion culling occluders
    OccluderArray &GetOccluders() { return m_occluders; }

    // debug draw objects
    DebugDrawRenderableArray &GetDebugObjects() { return m_debugDrawObjects; }

private:
    // accepting lights?
    bool m_acceptingLights;

    // accepting draw mask?
    uint32 m_acceptingRenderPassMask;

    // accepting occluders?
    bool m_acceptingOccluders;

    // accepting debug info?
    bool m_acceptingDebugObjects;

    // total objects added
    uint32 m_queueSize;
    uint32 m_numObjectsInvalidatedByOcclusion;

    // list of lights
    DirectionalLightArray m_directionalLightArray;
    PointLightArray m_pointLightArray;
    SpotLightArray m_spotLightArray;
    VolumetricLightArray m_volumetricLightArray;

    // opaque objects with light interactions
    // todo in the future: split this into z prepass, lightmap/emissive, lit buckets and examine impact on perf
    RenderableArray m_opaqueRenderables;

    // unlit translucent objects, also those with light interactions, since this is sorted by distance
    RenderableArray m_translucentRenderables;

    // post process renderables
    RenderableArray m_postProcessRenderables;

    // occlusion culling occluders
    OccluderArray m_occluders;

    // objects with debug draw callbacks
    DebugDrawRenderableArray m_debugDrawObjects;
};

