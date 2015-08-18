#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainRenderer.h"
#include "Engine/TerrainRendererNull.h"
#include "Engine/TerrainRendererCDLOD.h"
#include "Engine/EngineCVars.h"
#include "Renderer/Renderer.h"
#include "Core/MeshUtilties.h"
Log_SetChannel(TerrainRenderer);

TerrainRenderer::TerrainRenderer(TERRAIN_RENDERER_TYPE type, const TerrainParameters *pParameters, const TerrainLayerList *pLayerList)
    : m_type(type),
      m_parameters(*pParameters),
      m_pLayerList(pLayerList)
{
    m_pLayerList->AddRef();
}

TerrainRenderer::~TerrainRenderer()
{

}

TerrainRenderer *TerrainRenderer::CreateTerrainRenderer(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList)
{
    // determine renderer
    TERRAIN_RENDERER_TYPE rendererType = TERRAIN_RENDERER_TYPE_NULL;
    if (g_pRenderer != NULL)
    {
        if (!NameTable_TranslateType(NameTables::TerrainRendererType, CVars::r_terrain_renderer.GetString(), &rendererType, true))
        {
            Log_WarningPrintf("TerrainRenderer::CreateTerrainRenderer: Unknown terrain renderer type '%s'. Using CDLOD.", CVars::r_terrain_renderer.GetString().GetCharArray());
            rendererType = TERRAIN_RENDERER_TYPE_CDLOD;
        }
    }

    switch (rendererType)
    {
    case TERRAIN_RENDERER_TYPE_NULL:        return new TerrainRendererNull(pParameters, pLayerList);
    case TERRAIN_RENDERER_TYPE_CDLOD:       return new TerrainRendererCDLOD(pParameters, pLayerList);
    }

    Panic("Unrecognized terrain renderer type");
    return NULL;
}

// bool TerrainManager::IsSectionInVisibleRange(int32 sectionX, int32 sectionY) const
// {
//     AABox sectionBounds = CalculateSectionBoundingBox(sectionX, sectionY);
//     return (sectionBounds.GetCenter() - Vector3(m_lastCameraPosition)).SquaredLength() <= Math::Square(m_loadDistance);
// }

TerrainSectionRenderProxy::TerrainSectionRenderProxy(uint32 entityId, const TerrainSection *pSection)
    : RenderProxy(entityId),
      m_pSection(pSection),
      m_sectionX(pSection->GetSectionX()),
      m_sectionY(pSection->GetSectionY())
{
    m_pSection->AddRef();
}

TerrainSectionRenderProxy::~TerrainSectionRenderProxy()
{
    m_pSection->Release();
}

// void TerrainRenderProxy::UpdateBounds()
// {
//     AABox boundingBox(m_pTerrainRenderer->GetTerrainManager()->CalculateTerrainBoundingBox());
// 
//     if (GetBoundingBox() != boundingBox)
//         SetBounds(boundingBox, Sphere::FromAABox(boundingBox));
// }

bool TerrainSectionRenderProxy::RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection) const
{
    return m_pSection->RayCast(ray, contactNormal, contactPoint, exitAtFirstIntersection);
}

uint32 TerrainSectionRenderProxy::GetIntersectingTriangles(const AABox &searchBox, IntersectingTriangleArray &intersectingTriangles) const
{
    uint32 numAdded = 0;

    // optimize me later! enumerate triangles in box
    m_pSection->EnumerateTrianglesIntersectingBox(searchBox, [searchBox, &intersectingTriangles, &numAdded](const float3 vertices[3])
    {
        // calculate the normal for these three vertices
        float3 normal(MeshUtilites::CalculateFaceNormal(vertices[0], vertices[1], vertices[2]));

        // add to list
        intersectingTriangles.Add(IntersectingTriangle(vertices[0], vertices[1], vertices[2], normal));
        numAdded++;
    });

    return numAdded;
}
