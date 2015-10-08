#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/SpriteRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/Material.h"
#include "Engine/Texture.h"
#include "Engine/Camera.h"
#include "Engine/ResourceManager.h"
#include "Engine/Engine.h"

static const char *SPRITE_MATERIAL_TEXTURE_PARAMETER_NAME = "SpriteTexture";

SpriteRenderProxy::SpriteRenderProxy(uint32 entityId, const Texture2D *pTexture /* = nullptr */, const float3 &position /* = float3::Zero */)
    : RenderProxy(entityId),
      m_pMaterial(nullptr),
      m_visibility(true),
      m_pTexture(nullptr),
      m_position(position),
      m_width(0.0f),
      m_height(0.0f),
      m_shadowFlags(0),
      m_tintEnabled(false),
      m_tintColor(0xFFFFFFFF),
      m_bGPUResourcesCreated(false)
      //m_pVertexArray(NULL)
{
    // set texture pointer
    m_pTexture = pTexture;
    if (m_pTexture != nullptr)
        m_pTexture->AddRef();
    else
        m_pTexture = g_pResourceManager->GetDefaultTexture2D();

    // get sprite dimensions
    m_width = (float)m_pTexture->GetWidth();
    m_height = (float)m_pTexture->GetHeight();

    // setup material
    const MaterialShader *pSpriteShader = g_pResourceManager->GetMaterialShader(g_pEngine->GetSpriteMaterialShaderName());
    DebugAssert(pSpriteShader != NULL);
    m_pMaterial = new Material();
    m_pMaterial->Create("", pSpriteShader);
    m_pMaterial->SetShaderTextureParameterByName(SPRITE_MATERIAL_TEXTURE_PARAMETER_NAME, pTexture);
    pSpriteShader->Release();

    // update bounds
    UpdateBounds();
}

SpriteRenderProxy::~SpriteRenderProxy()
{
    ReleaseDeviceResources();

    m_pTexture->Release();
}

void SpriteRenderProxy::SetTexture(const Texture2D *pTexture)
{

}

void SpriteRenderProxy::RealSetTexture(const Texture2D *pTexture)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pTexture != NULL);

    if (m_pTexture == pTexture)
        return;

    ReleaseDeviceResources();

    // release mesh
    m_pTexture->Release();

    // set new mesh
    m_pTexture = pTexture;
    m_pTexture->AddRef();

    // update material
    m_pMaterial->SetShaderTextureParameterByName(SPRITE_MATERIAL_TEXTURE_PARAMETER_NAME, pTexture);

    // fix up bounds
    UpdateBounds();
}

void SpriteRenderProxy::SetPosition(const float3 &position)
{
    if (!IsInWorld())
    {
        RealSetPosition(position);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, position]() {
            pThis->RealSetPosition(position);
        });
    }
}

void SpriteRenderProxy::RealSetPosition(const float3 &position)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_position = position;
    UpdateBounds();
}

void SpriteRenderProxy::SetSizeByTextureScale(float textureScale)
{
    if (!IsInWorld())
    {
        RealSetSizeByTextureScale(textureScale);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, textureScale]() {
            pThis->RealSetSizeByTextureScale(textureScale);
        });
    }
}

void SpriteRenderProxy::RealSetSizeByTextureScale(float textureScale)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    float newWidth = (float)m_pTexture->GetWidth() * textureScale;
    float newHeight = (float)m_pTexture->GetHeight() * textureScale;

    m_width = newWidth;
    m_height = newHeight;
    UpdateBounds();
}

void SpriteRenderProxy::SetSizeByDimensions(float width, float height)
{
    if (!IsInWorld())
    {
        RealSetSizeByDimensions(width, height);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, width, height]() {
            pThis->RealSetSizeByDimensions(width, height);
        });
    }
}

void SpriteRenderProxy::RealSetSizeByDimensions(float width, float height)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_width = width;
    m_height = height;
    UpdateBounds();
}

void SpriteRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    if (!IsInWorld())
    {
        RealSetShadowFlags(shadowFlags);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, shadowFlags]() {
            pThis->RealSetShadowFlags(shadowFlags);
        });
    }
}

void SpriteRenderProxy::RealSetShadowFlags(uint32 shadowFlags)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_shadowFlags = shadowFlags;
    UpdateBounds();
}

void SpriteRenderProxy::SetTintColor(bool enabled, uint32 color /* = 0 */)
{
    if (!IsInWorld())
    {
        RealSetTintColor(enabled, color);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, enabled, color]() {
            pThis->RealSetTintColor(enabled, color);
        });
    }
}

void SpriteRenderProxy::RealSetTintColor(bool enabled, uint32 color /*= 0*/)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_tintEnabled = enabled;
    m_tintColor = (enabled) ? color : 0xFFFFFFFF;
    UpdateBounds();
}

void SpriteRenderProxy::SetVisibility(bool visible)
{
    if (!IsInWorld())
    {
        RealSetVisibility(visible);
    }
    else
    {
        ReferenceCountedHolder<SpriteRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, visible]() {
            pThis->RealSetVisibility(visible);
        });
    }
}

void SpriteRenderProxy::RealSetVisibility(bool visible)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_visibility = visible;
}

void SpriteRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_visibility)
        return;

    // Store the requested render passes.
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOW_MAP;
    if (m_tintEnabled)
        wantedRenderPasses |= RENDER_PASS_TINT;

    // skip for no passes
    if ((pRenderQueue->GetAcceptingRenderPassMask() & wantedRenderPasses) == 0)
        return;

    // Determine render passes.
    uint32 renderPassMask = m_pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
    if (renderPassMask != 0)
    {
        // Calculate view distance
        float viewDistance = pCamera->CalculateDepthToPoint(m_position);

        // Create queue entry
        RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
        queueEntry.RenderPassMask = renderPassMask;
        queueEntry.pRenderProxy = this;
        queueEntry.BoundingBox = GetBoundingBox();
        queueEntry.pVertexFactoryTypeInfo = OBJECT_TYPEINFO(PlainVertexFactory);
        queueEntry.VertexFactoryFlags = PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD;
        queueEntry.pMaterial = m_pMaterial;
        queueEntry.ViewDistance = viewDistance;
        queueEntry.TintColor = m_tintColor;
        queueEntry.Layer = m_pMaterial->GetShader()->SelectRenderQueueLayer();
        pRenderQueue->AddRenderable(&queueEntry);
    }
}

void SpriteRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const
{
    if (!m_bGPUResourcesCreated && !CreateDeviceResources())
        return;

    uint32 i;
    PlainVertexFactory::Vertex vertices[4];

    float hw = m_width * 0.5f;
    float hh = m_height * 0.5f;

    // these coordinates have to be in y-up coordinate system, as we are working with the view matrix
    vertices[0].SetUV(-hw, hh, 0.0f, 0.0f, 0.0f);
    vertices[1].SetUV(-hw, -hh, 0.0f, 0.0f, 1.0f);
    vertices[2].SetUV(hw, hh, 0.0f, 1.0f, 0.0f);
    vertices[3].SetUV(hw, -hh, 0.0f, 1.0f, 1.0f);
    
    // extracting the rotation component will make it an orthogonal matrix, so we can gain some perf by transposing instead of inverting
    float4x4 tempMatrix(pCamera->GetViewMatrix());
    tempMatrix.SetColumn(3, float4::UnitW);
    tempMatrix.TransposeInPlace();

    // translate matrix
    tempMatrix.SetColumn(3, float4(m_position, 1.0f));

    // pretransform the vertices on the cpu, avoiding a gpu stall
    for (i = 0; i < 4; i++)
        vertices[i].Position = tempMatrix.TransformPoint(vertices[i].Position);

    pCommandList->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);
    

    //const float4x4 &viewMatrix = pGPUDevice->GetGPUConstants()->GetCameraViewMatrix();
    //Vector3 cameraUp(viewMatrix.GetColumn(1));
    //Vector3 camUp(pContext->GetCamera()->GetUpDirection());
    /*Vector3 camUp(Vector3::UnitZ);
    Vector3 yAxis((Vector3(pContext->GetCamera()->GetEyePosition() - Vector3(m_Position))).Normalize());
    Vector3 xAxis(camUp.Cross(yAxis));
    Vector3 zAxis(yAxis.Cross(xAxis));
    Matrix4 billboardMatrix;
    billboardMatrix.SetRow(0, Vector4(xAxis, m_Position.x));
    billboardMatrix.SetRow(1, Vector4(yAxis, m_Position.y));
    billboardMatrix.SetRow(2, Vector4(zAxis, m_Position.z));
    billboardMatrix.SetRow(3, Vector4::UnitW);

    //pGPUDevice->GetGPUConstants()->SetLocalToWorldMatrix(billboardMatrix, true);

    for (i = 0; i < 4; i++)
        vertices[i].Position = billboardMatrix.TransformPoint(vertices[i].Position);

    pGPUDevice->GetGPUConstants()->SetLocalToWorldMatrix(Matrix4::Identity, true);
    */
    pCommandList->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_STRIP);
    pCommandList->DrawUserPointer(vertices, sizeof(vertices[0]), countof(vertices));
}

bool SpriteRenderProxy::CreateDeviceResources() const
{
    if (m_bGPUResourcesCreated)
        return true;

    // material will automatically create texture resources
    if (!m_pMaterial->CreateDeviceResources())
        return false;

    m_bGPUResourcesCreated = true;
    return true;
}

void SpriteRenderProxy::ReleaseDeviceResources() const
{
    //SAFE_RELEASE(m_pVertexArray);
    m_bGPUResourcesCreated = false;
}

void SpriteRenderProxy::UpdateBounds()
{
    // depending on the camera angle, the z could stretch up to the width of the sprite.
    SIMDVector3f halfWorldSize(SIMDVector3f(m_width, m_height, m_width) * 0.5f);
    SIMDVector3f position(m_position);
    AABox newBounds(position - halfWorldSize, position + halfWorldSize);
    SetBounds(newBounds, Sphere::FromAABox(newBounds));
}
