#pragma once
#include "Engine/Common.h"
#include "Engine/TerrainSection.h"
#include "Renderer/RenderProxy.h"

class TerrainSectionRendererData;
class TerrainSectionRenderProxy;

class TerrainRenderer
{
    friend class TerrainManager;

public:
    TerrainRenderer(TERRAIN_RENDERER_TYPE type, const TerrainParameters *pParameters, const TerrainLayerList *pLayerList);
    virtual ~TerrainRenderer();

    // accessors
    const TERRAIN_RENDERER_TYPE GetType() const { return m_type; }
    const TerrainParameters *GetTerrainParameters() const { return &m_parameters; }
    const TerrainLayerList *GetLayerList() const { return m_pLayerList; }

    // create a render proxy for a section
    virtual TerrainSectionRenderProxy *CreateSectionRenderProxy(uint32 entityId, const TerrainSection *pSection) = 0;

    // gpu resources
    virtual bool CreateGPUResources() = 0;
    virtual void ReleaseGPUResources() = 0;

    //////////////////////////////////////////////////////////////////////////

    // create renderer
    static TerrainRenderer *CreateTerrainRenderer(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList);

protected:
    // vars
    TERRAIN_RENDERER_TYPE m_type;
    TerrainParameters m_parameters;
    const TerrainLayerList *m_pLayerList;
};

class TerrainSectionRenderProxy : public RenderProxy
{
public:
    TerrainSectionRenderProxy(uint32 entityId, const TerrainSection *pSection);
    virtual ~TerrainSectionRenderProxy();

    // section access
    const TerrainSection *GetSection() const { return m_pSection; }
    const int32 GetSectionX() const { return m_sectionX; }
    const int32 GetSectionY() const { return m_sectionY; }

    // when a layer is added to a section
    virtual void OnLayersModified() = 0;

    // when a height value is modified
    virtual void OnPointHeightModified(uint32 x, uint32 y) = 0;

    // when a layer weight is modified
    virtual void OnPointLayersModified(uint32 x, uint32 y) = 0;

    // we implement the raycasting and intersections in the base class, they are common between all renderer types
    virtual bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection) const override;
    virtual uint32 GetIntersectingTriangles(const AABox &searchBox, IntersectingTriangleArray &intersectingTriangles) const override;

protected:
    const TerrainSection *m_pSection;
    int32 m_sectionX, m_sectionY;
};

// keep section in memory
//bool IsSectionInVisibleRange(int32 sectionX, int32 sectionY) const;

