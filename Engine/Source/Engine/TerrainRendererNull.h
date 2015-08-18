#pragma once
#include "Engine/TerrainRenderer.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/VertexFactory.h"

class TerrainRendererNull;
class TerrainSectionRenderProxyNull;

class TerrainRendererNull : public TerrainRenderer
{
public:
    TerrainRendererNull(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList);
    virtual ~TerrainRendererNull();

    // create a render proxy for this terrain
    virtual TerrainSectionRenderProxy *CreateSectionRenderProxy(uint32 entityId, const TerrainSection *pSection) override final;

    // gpu resources
    virtual bool CreateGPUResources() override final;
    virtual void ReleaseGPUResources() override final;
};

class TerrainSectionRenderProxyNull : public TerrainSectionRenderProxy
{
public:
    TerrainSectionRenderProxyNull(uint32 entityId, const TerrainRendererNull *pRenderer, const TerrainSection *pSection);
    ~TerrainSectionRenderProxyNull();

    virtual void OnLayersModified() override final;
    virtual void OnPointHeightModified(uint32 x, uint32 y) override final;
    virtual void OnPointLayersModified(uint32 x, uint32 y) override final;

private:
    const TerrainRendererNull *m_pRenderer;
};


