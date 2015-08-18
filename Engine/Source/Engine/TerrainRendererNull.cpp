#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainRendererNull.h"
#include "Engine/ResourceManager.h"
#include "Engine/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderMap.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(TerrainRendererNull);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainRendererNull
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainRendererNull::TerrainRendererNull(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList)
    : TerrainRenderer(TERRAIN_RENDERER_TYPE_CDLOD, pParameters, pLayerList)
{

}

TerrainRendererNull::~TerrainRendererNull()
{
    TerrainRendererNull::ReleaseGPUResources();
}

TerrainSectionRenderProxy *TerrainRendererNull::CreateSectionRenderProxy(uint32 entityId, const TerrainSection *pSection)
{
    return new TerrainSectionRenderProxyNull(entityId, this, pSection);
}

bool TerrainRendererNull::CreateGPUResources()
{
    return true;
}

void TerrainRendererNull::ReleaseGPUResources()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainRenderProxyNull
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainSectionRenderProxyNull::TerrainSectionRenderProxyNull(uint32 entityId, const TerrainRendererNull *pRenderer, const TerrainSection *pSection)
    : TerrainSectionRenderProxy(entityId, pSection),
      m_pRenderer(pRenderer)
{

}

TerrainSectionRenderProxyNull::~TerrainSectionRenderProxyNull()
{

}

void TerrainSectionRenderProxyNull::OnLayersModified()
{

}

void TerrainSectionRenderProxyNull::OnPointHeightModified(uint32 x, uint32 y)
{

}

void TerrainSectionRenderProxyNull::OnPointLayersModified(uint32 x, uint32 y)
{

}
