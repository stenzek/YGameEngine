#include "Renderer/PrecompiledHeader.h"
#include "Renderer/DecalManager.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
Log_SetChannel(DecalManager);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StaticDecal::StaticDecal(const AABox &boundingBox, const Sphere &boundingSphere, const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance, float lifetime, uint32 entityID)
    : RenderProxy(entityID),
      m_position(position),
      m_normal(normal),
      m_size(size),
      m_pMaterial(pMaterial),
      m_drawDistance(drawDistance),
      m_lifeRemaining(lifetime),
      m_pVertexBuffer(NULL)
{
    SetBounds(boundingBox, boundingSphere);
    m_pMaterial->AddRef();
}

StaticDecal::StaticDecal(const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance, float lifetime, uint32 entityID)
    : RenderProxy(entityID),
      m_position(position),
      m_normal(normal),
      m_size(size),
      m_pMaterial(pMaterial),
      m_drawDistance(drawDistance),
      m_lifeRemaining(lifetime),
      m_pVertexBuffer(NULL)
{
    m_pMaterial->AddRef();
}

StaticDecal::~StaticDecal()
{
    m_pMaterial->Release();

    if (m_pVertexBuffer != NULL)
        m_pVertexBuffer->Release();
}

void StaticDecal::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // available mask for decals
    //const uint32 availableRenderPasses = RENDER_PASSES_DEFAULT;

    // get distance
    float distance = pCamera->CalculateDepthToPoint(m_position);

    // out of range?
    if (distance > m_drawDistance)
        return;

    // queue it
    //RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
    //queueEntry.BoundingBox = 
}

void StaticDecal::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const
{

}

void StaticDecal::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const
{

}

void StaticDecal::Rebuild(const float3 &position, const float3 &normal, const float2 &size)
{
    // generate decal mesh
    DecalMeshGenerator decalMeshGenerator(position, normal, size.x, size.y);

    // collect vertices from objects
    RenderProxy::IntersectingTriangleArray intersectingTriangles;
    GetRenderWorld()->EnumerateRenderablesInAABox(decalMeshGenerator.GetDecalBox(), [&decalMeshGenerator, &intersectingTriangles](const RenderProxy *pRenderProxy) {
        pRenderProxy->GetIntersectingTriangles(decalMeshGenerator.GetDecalBox(), intersectingTriangles);
    });

    // convert format
    Log_DevPrintf("%u intersecting triangles", intersectingTriangles.GetSize());

    // generate triangles
    //uint32 nTriangles = decalMeshGenerator.GenerateDecalTriangles(intersectingTriangles);
    
    // calculate bounding box from vertices
    AABox boundingBox(decalMeshGenerator.GetBoundingBox());
    Sphere boundingSphere(Sphere::FromAABox(boundingBox));

    // update bounding box/sphere
    SetBounds(boundingBox, boundingSphere);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DecalManager::DecalManager(RenderWorld *pRenderWorld)
    : m_pRenderWorld(pRenderWorld)
{

}

DecalManager::~DecalManager()
{
    for (uint32 i = 0; i < m_staticDecals.GetSize(); i++)
    {
        m_pRenderWorld->RemoveRenderable(m_staticDecals[i]);
        m_staticDecals[i]->Release();
    }
}

StaticDecal *DecalManager::CreateStaticDecal(const float3 &position, const float3 &normal, const float2 &size, const Material *pMaterial, float drawDistance /*= Y_FLT_MAX*/, float lifetime /*= Y_FLT_INFINITE*/)
{
    return NULL;
}

void DecalManager::MoveStaticDecal(StaticDecal *pDecal, const float3 &position, const float3 &normal)
{

}

void DecalManager::ResizeStaticDecal(StaticDecal *pDecal, const float3 &size)
{

}

void DecalManager::RemoveStaticDecal(StaticDecal *pDecal)
{

}

StaticDecal *DecalManager::ProjectStaticDecal(const Ray &ray, const float2 &size, const Material *pMaterial, float drawDistance /*= Y_FLT_MAX*/, float lifetime /*= Y_FLT_INFINITE*/)
{
    return nullptr;
}

void DecalManager::Tick(const float timeDifference)
{

}

DecalMeshGenerator::DecalMeshGenerator(const float3 &position, const float3 &normal, const float width, const float height)
    : m_position(position),
      m_normal(normal),
      m_width(width),
      m_height(height),
      m_depth(Max(width, height)),
      m_rightVector(float3::UnitX),
      m_upVector(float3::UnitZ),
      m_decalBox(AABox::Zero),
      m_boundingBox(AABox::Zero)
{
    CalculateDecalFrame();
    CalculateDecalBox();
}

DecalMeshGenerator::~DecalMeshGenerator()
{

}

uint32 DecalMeshGenerator::GenerateDecalTriangles(const RenderProxy::IntersectingTriangleArray &inputTriangles)
{
    return 0;
}

void DecalMeshGenerator::CalculateDecalFrame()
{
    if (m_normal == float3::UnitZ)
        m_rightVector = float3::UnitX;
    else if (m_normal == float3::NegativeUnitZ)
        m_rightVector = float3::NegativeUnitX;
    else
        m_rightVector = (-m_normal).Cross(float3::UnitZ).Normalize();

    m_upVector = m_rightVector.Cross(-m_normal).Normalize();
}

void DecalMeshGenerator::CalculateDecalBox()
{
    // get the maximum dimension
    float halfDepth = m_depth * 0.5f;
    float halfWidth = m_width * 0.5f;
    float halfHeight = m_height * 0.5f;

    // width
    float3 leftSide(m_position + (-m_rightVector) * halfWidth);
    float3 rightSide(m_position + (m_rightVector)* halfWidth);
    float3 topSide(m_position + (m_upVector)* halfHeight);
    float3 bottomSide(m_position + (-m_upVector) * halfHeight);
    float3 frontSide(m_position + (m_normal)* halfDepth);
    float3 backSide(m_position + (-m_normal) * halfDepth);

    AABox decalBox(m_position, m_position);
    decalBox.Merge(leftSide);
    decalBox.Merge(rightSide);
    decalBox.Merge(topSide);
    decalBox.Merge(bottomSide);
    decalBox.Merge(frontSide);
    decalBox.Merge(backSide);

    m_decalBox = decalBox;
}

void DecalMeshGenerator::CalculateBoundingBox()
{

}

void DecalMeshGenerator::CalculateClippingPlanes()
{
    float3 planeNormal;
    float3 planeRefPoint;

    planeNormal = m_normal;
    planeRefPoint = m_position + (planeNormal * m_depth);
    m_clippingPlanes[0] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));

    planeNormal = -m_normal;
    planeRefPoint = m_position + (planeNormal * m_depth);
    m_clippingPlanes[1] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));

    planeNormal = m_rightVector;
    planeRefPoint = m_position + (planeNormal * m_width);
    m_clippingPlanes[2] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));

    planeNormal = -m_rightVector;
    planeRefPoint = m_position + (planeNormal * m_width);
    m_clippingPlanes[3] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));

    planeNormal = m_upVector;
    planeRefPoint = m_position + (planeNormal * m_height);
    m_clippingPlanes[4] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));

    planeNormal = -m_upVector;
    planeRefPoint = m_position + (planeNormal * m_height);
    m_clippingPlanes[5] = Plane(planeNormal, planeRefPoint.Dot(planeNormal));
}

void DecalMeshGenerator::GenerateClippedTriangles(const RenderProxy::IntersectingTriangleArray &inputTriangles)
{

}
