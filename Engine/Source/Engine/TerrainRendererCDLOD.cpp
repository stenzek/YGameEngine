#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainRendererCDLOD.h"
#include "Engine/TerrainQuadTree.h"
#include "Engine/ResourceManager.h"
#include "Engine/Camera.h"
#include "Engine/Engine.h"
#include "Engine/EngineCVars.h"
#include "Engine/StaticMesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Shaders/OneColorShader.h"
#include "Renderer/ShaderProgram.h"
Log_SetChannel(TerrainRendererCDLOD);

static const float DEFAULT_TERRAIN_QUADTREE_MORPH_START_RATIO = 0.70f;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainRendererCDLOD_VertexFactory
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_VERTEX_FACTORY_TYPE_INFO(TerrainRendererCDLOD_VertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(TerrainRendererCDLOD_VertexFactory)
DEFINE_SHADER_COMPONENT_PARAMETER("TerrainPatchOffset", SHADER_PARAMETER_TYPE_FLOAT3)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainPatchSize", SHADER_PARAMETER_TYPE_FLOAT3)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainPatchOffsetInTextureSpace", SHADER_PARAMETER_TYPE_FLOAT2)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainPatchSizeInTextureSpace", SHADER_PARAMETER_TYPE_FLOAT2)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainHeightMapDimensions", SHADER_PARAMETER_TYPE_FLOAT4)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainGridDimensions", SHADER_PARAMETER_TYPE_FLOAT3)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainMorphConstants", SHADER_PARAMETER_TYPE_FLOAT4)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainHeightMapTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("TerrainHeightMapSampler", SHADER_PARAMETER_TYPE_SAMPLER_STATE)
END_SHADER_COMPONENT_PARAMETERS()

bool TerrainRendererCDLOD_VertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool TerrainRendererCDLOD_VertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/TerrainVertexFactory.hlsl");
    return true;
}

uint32 TerrainRendererCDLOD_VertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
{
    // create format declaration
    GPU_VERTEX_ELEMENT_DESC *pElementDesc = pElementsDesc;
    uint32 streamOffset;
    uint32 nElements = 0;
    uint32 nStreams = 0;

    // build stream 0
    {
        streamOffset = 0;

        // position
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // end of stream
        nStreams++;
    }

    // done
    return nElements;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainRendererCDLOD
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainRendererCDLOD::TerrainRendererCDLOD(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList)
    : TerrainRenderer(TERRAIN_RENDERER_TYPE_CDLOD, pParameters, pLayerList),
      m_pSettingsUpdateCallback(MakeFunctorClass(this, &TerrainRendererCDLOD::OnSettingsChangedCallback)),
      m_settingsUpdatedFlag(false),
      m_gridSize(0),
      m_pVertexBuffer(NULL),
      m_pIndexBuffer(NULL),
      m_indexFormat(GPU_INDEX_FORMAT_UINT16),
      m_indexCount(0),
      m_indexCountPerSubQuad(0),
      m_indexCountSingleQuad(0),
      m_indexEndTL(0),
      m_indexEndTR(0),
      m_indexEndBL(0),
      m_indexEndBR(0),
      m_GPUResourcesCreated(false)
{
    CVars::r_terrain_render_resolution_multiplier.AddChangeCallback(m_pSettingsUpdateCallback);
}

TerrainRendererCDLOD::~TerrainRendererCDLOD()
{
    if (Renderer::IsOnRenderThread())
    {
        TerrainRendererCDLOD::ReleaseGPUResources();
    }
    else
    {
        QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this]() {
            TerrainRendererCDLOD::ReleaseGPUResources();
        });
    }

    CVars::r_terrain_render_resolution_multiplier.RemoveChangeCallback(m_pSettingsUpdateCallback);
    delete m_pSettingsUpdateCallback;
}

TerrainSectionRenderProxy *TerrainRendererCDLOD::CreateSectionRenderProxy(uint32 entityId, const TerrainSection *pSection)
{
    // fixme properly
    TerrainSectionRenderProxyCDLOD *pRenderProxy = new TerrainSectionRenderProxyCDLOD(entityId, this, pSection);

    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([&pRenderProxy]()
    {
        if (!pRenderProxy->CreateGPUResources())
        {
            pRenderProxy->Release();
            pRenderProxy = nullptr;
        }
    });

    return pRenderProxy;
}

bool TerrainRendererCDLOD::CreateGPUResources()
{
    if (m_settingsUpdatedFlag)
    {
        ReleaseGPUResources();
        m_settingsUpdatedFlag = false;
    }

    if (m_GPUResourcesCreated)
        return true;

    bool result = true;
    if (m_pVertexBuffer == NULL || m_pIndexBuffer == NULL)
    {
        if (!CreateGridBuffers())
            result = false;
    }

    m_GPUResourcesCreated = result;
    return result;
}

void TerrainRendererCDLOD::ReleaseGPUResources()
{
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    m_indexFormat = GPU_INDEX_FORMAT_UINT16;
    m_gridSize = 0;
    m_indexCount = 0;
    m_indexCountPerSubQuad = 0;
    m_indexEndTL = 0;
    m_indexEndTR = 0;
    m_indexEndBL = 0;
    m_indexEndBR = 0;
    m_indexCountSingleQuad = 0;
    m_GPUResourcesCreated = false;
}

bool TerrainRendererCDLOD::CreateGridBuffers()
{
    // calculate grid size
    uint32 gridQuads = (m_parameters.SectionSize >> (m_parameters.LODCount - 1));
    uint32 gridVertices = gridQuads + 1;
    DebugAssert(gridQuads > 0);

    // todo: render resolution multiplier
    // calculate the number of vertices, indices needed
    uint32 vertexSize = sizeof(float2);
    uint32 nVerticesRequired = gridVertices * gridVertices;
    uint32 nIndicesRequired = (gridQuads * gridQuads * 3 * 2) + 6;
    Log_DevPrintf("TerrainRendererCDLOD::CreateGridBuffers: Calculated grid size is %u vertices, %u quads", gridVertices, gridQuads);
    Log_DevPrintf("TerrainRendererCDLOD::CreateGridBuffers: %u vertices required, %u indices required", nVerticesRequired, nIndicesRequired);

    // fill vertices
    float2 *pVertices = new float2[nVerticesRequired];
    uint32 nVertices = 0;
    for (uint32 y = 0; y < gridVertices; y++)
    {
        for (uint32 x = 0; x < gridVertices; x++)
        {
            // this is deliberately quads (vertices - 1) so that (quads) == 1.0
            DebugAssert(nVertices < nVerticesRequired);
            pVertices[nVertices++].Set((float)x / (float)gridQuads,
                                        (float)y / (float)gridQuads);
        }
    }

    // fill indices
    GPU_INDEX_FORMAT indexFormat;
    uint16 *pIndices16 = NULL;
    uint32 *pIndices32 = NULL;
    uint32 nIndices = 0;
    uint32 halfGridQuads = gridQuads / 2;
    uint32 indexCountPerSubQuad = halfGridQuads * halfGridQuads * 6;
    uint32 indexCountSingleQuad = 6;
    uint32 indexEndTL;
    uint32 indexEndTR;
    uint32 indexEndBL;
    uint32 indexEndBR;

    // use 16bit?
    if (nVertices < 0xFFFF)
    {
        pIndices16 = new uint16[nIndicesRequired];
        indexFormat = GPU_INDEX_FORMAT_UINT16;

        // top-left
        for (uint32 y = 0; y < halfGridQuads; y++)
        {
            for (uint32 x = 0; x < halfGridQuads; x++)
            {
                pIndices16[nIndices++] = (uint16)(x + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndTL = nIndices;

        // Top right part
        for (uint32 y = 0; y < halfGridQuads; y++)
        {
            for (uint32 x = halfGridQuads; x < gridQuads; x++)
            {
                pIndices16[nIndices++] = (uint16)(x + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndTR = nIndices;

        // Bottom left part
        for (uint32 y = halfGridQuads; y < gridQuads; y++)
        {
            for (uint32 x = 0; x < halfGridQuads; x++)
            {
                pIndices16[nIndices++] = (uint16)(x + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndBL = nIndices;

        // Bottom right part
        for (uint32 y = halfGridQuads; y < gridQuads; y++)
        {
            for (uint32 x = halfGridQuads; x < gridQuads; x++)
            {
                pIndices16[nIndices++] = (uint16)(x + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)(x + gridVertices * (y + 1));
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * y);
                pIndices16[nIndices++] = (uint16)((x + 1) + gridVertices * (y + 1));
            }
        }

        indexEndBR = nIndices;

        // single quad
        pIndices16[nIndices++] = (uint16)0;
        pIndices16[nIndices++] = (uint16)(gridQuads);
        pIndices16[nIndices++] = (uint16)(gridVertices * gridQuads);
        pIndices16[nIndices++] = (uint16)(gridVertices * gridQuads);
        pIndices16[nIndices++] = (uint16)(gridQuads);
        pIndices16[nIndices++] = (uint16)(gridQuads + (gridVertices * gridQuads));
    }
    else
    {
        pIndices32 = new uint32[nIndicesRequired];
        indexFormat = GPU_INDEX_FORMAT_UINT32;

        // top-left
        for (uint32 y = 0; y < halfGridQuads; y++)
        {
            for (uint32 x = 0; x < halfGridQuads; x++)
            {
                pIndices32[nIndices++] = (x + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndTL = nIndices;

        // Top right part
        for (uint32 y = 0; y < halfGridQuads; y++)
        {
            for (uint32 x = halfGridQuads; x < gridQuads; x++)
            {
                pIndices32[nIndices++] = (x + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndTR = nIndices;

        // Bottom left part
        for (uint32 y = halfGridQuads; y < gridQuads; y++)
        {
            for (uint32 x = 0; x < halfGridQuads; x++)
            {
                pIndices32[nIndices++] = (x + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * (y + 1));
            }
        }
        indexEndBL = nIndices;

        // Bottom right part
        for (uint32 y = halfGridQuads; y < gridQuads; y++)
        {
            for (uint32 x = halfGridQuads; x < gridQuads; x++)
            {
                pIndices32[nIndices++] = (x + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = (x + gridVertices * (y + 1));
                pIndices32[nIndices++] = ((x + 1) + gridVertices * y);
                pIndices32[nIndices++] = ((x + 1) + gridVertices * (y + 1));
            }
        }

        indexEndBR = nIndices;

        // single quad
        pIndices32[nIndices++] = 0;
        pIndices32[nIndices++] = (gridQuads);
        pIndices32[nIndices++] = (gridVertices * gridQuads);
        pIndices32[nIndices++] = (gridVertices * gridQuads);
        pIndices32[nIndices++] = (gridQuads);
        pIndices32[nIndices++] = (gridQuads + (gridVertices * gridQuads));
    }

    // should be equal
    DebugAssert(nVertices == nVerticesRequired && nIndices == nIndicesRequired);

    // create vertex buffer
    GPUBuffer *pVertexBuffer;
    GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, vertexSize * nVertices);
    if ((pVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, pVertices)) == NULL)
    {
        Log_ErrorPrintf("TerrainRendererCDLOD::CreateGridBuffers: Could not create grid vertex buffer.");
        delete[] pVertices;
        delete[] pIndices16;
        delete[] pIndices32;
        return false;
    }

    // create index buffer
    GPUBuffer *pIndexBuffer;
    GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, ((indexFormat == GPU_INDEX_FORMAT_UINT16) ? sizeof(uint16) : sizeof(uint32)) * nIndices);
    if ((pIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, (indexFormat == GPU_INDEX_FORMAT_UINT16) ? (void *)pIndices16 : (void *)pIndices32)) == NULL)
    {
        Log_ErrorPrintf("TerrainRendererCDLOD::CreateGridBuffers: Could not create grid index buffer.");
        pVertexBuffer->Release();
        delete[] pVertices;
        delete[] pIndices16;
        delete[] pIndices32;
        return false;
    }

    // store indices
    DebugAssert(m_pVertexBuffer == NULL && m_pIndexBuffer == NULL);
    m_pVertexBuffer = pVertexBuffer;
    m_pIndexBuffer = pIndexBuffer;
    m_indexFormat = indexFormat;
    m_gridSize = gridQuads;
    m_indexCount = nIndices;
    m_indexCountPerSubQuad = indexCountPerSubQuad;
    m_indexCountSingleQuad = indexCountSingleQuad;
    m_indexEndTL = indexEndTL;
    m_indexEndTR = indexEndTR;
    m_indexEndBL = indexEndBL;
    m_indexEndBR = indexEndBR;
    delete[] pVertices;
    delete[] pIndices16;
    delete[] pIndices32;

#ifdef Y_BUILD_CONFIG_DEBUG
    // set debug names
    m_pVertexBuffer->SetDebugName("<terrain patch vertex data>");
    m_pIndexBuffer->SetDebugName("<terrain patch index data>");
#endif

    return true;
}
    
bool TerrainRendererCDLOD::BindGridBuffers(GPUContext *pGPUDevice) const
{
    if (!m_GPUResourcesCreated && !const_cast<TerrainRendererCDLOD *>(this)->CreateGPUResources())
        return false;

    uint32 vertexBufferOffset = 0;
    uint32 vertexBufferStride = sizeof(float2);
    pGPUDevice->SetVertexBuffers(0, 1, &m_pVertexBuffer, &vertexBufferOffset, &vertexBufferStride);
    pGPUDevice->SetIndexBuffer(m_pIndexBuffer, m_indexFormat, 0);
    return true;
}

void TerrainRendererCDLOD::OnSettingsChangedCallback()
{
    m_settingsUpdatedFlag = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainSectionRenderProxyCDLOD
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainSectionRenderProxyCDLOD::TerrainSectionRenderProxyCDLOD(uint32 entityID, const TerrainRendererCDLOD *pRenderer, const TerrainSection *pSection)
    : TerrainSectionRenderProxy(entityID, pSection),
      m_pRenderer(pRenderer),
      m_pHeightMapTexture(nullptr),
      m_pNormalMapTexture(nullptr),
      m_pAlphaMapTexture(nullptr),
      m_pRenderMaterial(nullptr),
      m_pDetailInstanceBuffer(nullptr),
      m_GPUResourcesCreated(false)
{
    Y_memzero(m_visibilityRanges, sizeof(m_visibilityRanges));
    Y_memzero(m_morphStart, sizeof(m_morphStart));
    Y_memzero(m_morphEnd, sizeof(m_morphEnd));
    Y_memzero(m_morphConstants, sizeof(m_morphConstants));

    // calculate bounding box
    SetBounds(pSection->GetBoundingBox(), Sphere::FromAABox(pSection->GetBoundingBox()));
}

TerrainSectionRenderProxyCDLOD::~TerrainSectionRenderProxyCDLOD()
{
    SAFE_RELEASE(m_pDetailInstanceBuffer);
    SAFE_RELEASE(m_pRenderMaterial);
    SAFE_RELEASE(m_pAlphaMapTexture);
    SAFE_RELEASE(m_pNormalMapTexture);
    SAFE_RELEASE(m_pHeightMapTexture);
}

void TerrainSectionRenderProxyCDLOD::OnLayersModified()
{
    DebugAssert(g_pRenderer->GetRenderThreadId() == Thread::GetCurrentThreadId());

    if (m_GPUResourcesCreated)
    {
        SAFE_RELEASE(m_pRenderMaterial);
        SAFE_RELEASE(m_pAlphaMapTexture);
        m_GPUResourcesCreated = false;

        if (CreateAlphaMapTexture())
        {
            if (CreateRenderMaterial())
                m_GPUResourcesCreated = true;
        }
    }
}

void TerrainSectionRenderProxyCDLOD::OnPointHeightModified(uint32 x, uint32 y)
{
    DebugAssert(g_pRenderer->GetRenderThreadId() == Thread::GetCurrentThreadId());

    if (m_pHeightMapTexture == nullptr)
        return;

    // get source pointer
    const void *pHeightMapValues;
    uint32 heightMapValueSize;
    uint32 heightMapRowPitch;
    m_pSection->GetRawHeightMapData(&pHeightMapValues, &heightMapValueSize, &heightMapRowPitch);

    // align it to the correct position
    DebugAssert(x < m_pSection->GetPointCount() && y < m_pSection->GetPointCount());
    const byte *pSourcePointer = reinterpret_cast<const byte *>(pHeightMapValues) + (y * heightMapRowPitch) + (x * heightMapValueSize);

    // write to texture
    g_pRenderer->GetGPUContext()->WriteTexture(m_pHeightMapTexture, pSourcePointer, heightMapValueSize, heightMapValueSize, 0, x, y, 1, 1);

    // update bounding box
    if (GetBoundingBox() != m_pSection->GetBoundingBox())
        SetBounds(m_pSection->GetBoundingBox(), Sphere::FromAABox(m_pSection->GetBoundingBox()));
}

void TerrainSectionRenderProxyCDLOD::OnPointLayersModified(uint32 x, uint32 y)
{
    DebugAssert(g_pRenderer->GetRenderThreadId() == Thread::GetCurrentThreadId());

    if (m_pAlphaMapTexture == nullptr)
        return;

    // for each splat map, gather the texel at these coordinates, then write it
    if (m_pAlphaMapTexture->GetTextureType() == TEXTURE_TYPE_2D_ARRAY)
    {
        Panic("Fixme");
        /*
        for (uint32 i = 0; i < textureArraySize; i++)
        {
            // gather the (up to) 4 texel values
            uint8 texelValues[4] = { 0, 0, 0, 0 };
            for (uint32 j = 0; j < 4; j++)
            {
                uint32 layerIndex = i * 4 + j;
                if (layerIndex < m_pSection->GetUsedLayerCount())
                    texelValues[j] = m_pSection->GetRawLayerWeightValues(layerIndex)[(y * m_pSection->GetPointCount()) + x];
                else
                    break;
            }

            // write to texture
            g_pRenderer->GetMainContext()->WriteTexture(static_cast<GPUTexture2DArray *>(m_pAlphaMapTexture), &texelValues, sizeof(texelValues), sizeof(texelValues), i, 0, x, y, 1, 1);
        }
        */
    }
    else
    {
        for (uint32 mapIndex = 0; mapIndex < m_pSection->GetSplatMapCount(); mapIndex++)
        {
            // get pointer
            const void *pSplatMapValues;
            uint32 splatMapValueSize;
            uint32 splatMapRowPitch;
            m_pSection->GetRawSplatMapData(mapIndex, &pSplatMapValues, &splatMapValueSize, &splatMapRowPitch);

            // align it to the correct position
            DebugAssert(x < m_pSection->GetPointCount() && y < m_pSection->GetPointCount());
            const byte *pSourcePointer = reinterpret_cast<const byte *>(pSplatMapValues)+(y * splatMapRowPitch) + (x * splatMapValueSize);

            // write to the texture
            g_pRenderer->GetGPUContext()->WriteTexture(static_cast<GPUTexture2D *>(m_pAlphaMapTexture), pSourcePointer, splatMapValueSize, splatMapValueSize, 0, x, y, 1, 1);
        }
    }
}

bool TerrainSectionRenderProxyCDLOD::CreateGPUResources()
{
    // can't render a section without any layers
    if (m_pSection->GetSplatMapCount() == 0)
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Section has no splat maps. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // create height map texture
    if (!CreateHeightMapTexture())
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Failed to create height map texture. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // create normal map texture
    if (!CreateNormalMapTexture())
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Failed to create normal map texture. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // create alpha map texture
    if (!CreateAlphaMapTexture())
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Failed to create alpha map texture. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // create render materials
    if (!CreateRenderMaterial())
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Failed to create render material. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // create detail buffer
    if (!CreateDetailInstanceBuffer())
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateGPUResources: Failed to create detail instance buffer. [section %i, %i]", m_pSection->GetSectionX(), m_pSection->GetSectionY());
        return false;
    }

    // done
    m_GPUResourcesCreated = true;
    return true;
}

bool TerrainSectionRenderProxyCDLOD::CreateHeightMapTexture()
{
    static const PIXEL_FORMAT heightStorageTextureFormats[TERRAIN_HEIGHT_STORAGE_FORMAT_COUNT] =
    {
        PIXEL_FORMAT_UNKNOWN,       // uint8
        PIXEL_FORMAT_UNKNOWN,       // uint16
        PIXEL_FORMAT_R32_FLOAT,     // float32
    };

#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Timer uploadTimer;
#endif

    // get pointers
    DebugAssert(m_pHeightMapTexture == NULL);

    // setup texture desc
    GPU_TEXTURE2D_DESC textureDesc;
    textureDesc.Width = textureDesc.Height = m_pSection->GetPointCount();
    textureDesc.Format = heightStorageTextureFormats[m_pSection->GetHeightStorageFormat()];
    textureDesc.Flags = GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_SHADER_BINDABLE;
    textureDesc.MipLevels = 1;

    // setup sampler state
    GPU_SAMPLER_STATE_DESC samplerStateDesc;
    samplerStateDesc.Filter = TEXTURE_FILTER_MIN_MAG_MIP_POINT;
    samplerStateDesc.AddressU = TEXTURE_ADDRESS_MODE_CLAMP;
    samplerStateDesc.AddressV = TEXTURE_ADDRESS_MODE_CLAMP;
    samplerStateDesc.AddressW = TEXTURE_ADDRESS_MODE_CLAMP;
    samplerStateDesc.BorderColor.SetZero();
    samplerStateDesc.LODBias = 0;
    samplerStateDesc.MinLOD = Y_INT32_MIN;
    samplerStateDesc.MaxLOD = Y_INT32_MAX;
    samplerStateDesc.MaxAnisotropy = 1;
    samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_NEVER;

    // get mip pitches
    const void *pHeightMapData;
    uint32 heightMapValueSize;
    uint32 heightMapRowPitch;
    m_pSection->GetRawHeightMapData(&pHeightMapData, &heightMapValueSize, &heightMapRowPitch);

    // create the texture object
    if ((m_pHeightMapTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, &pHeightMapData, &heightMapRowPitch)) == NULL)
    {
        Log_ErrorPrintf("TerrainSectionRendererDataCDLOD::CreateFloatHeightMapTexture: Failed to create height map texture.");
        return false;
    }

//     String str;
//     const float *pPtr = (const float *)pDataPointer;
//     for (uint32 i = 0; i < textureDesc.Height; i++)
//     {
//         str.Clear();
// 
//         const float *start = pPtr;
//         for (uint32 j = 0; j < textureDesc.Width; j++)
//             str.AppendFormattedString("%.2f ", *(pPtr++));
// 
//         DebugAssert(pPtr == ((float *)((byte *)start + dataPitch)));
//         Log_DevPrint(str.GetCharArray());
//     }

    // set debug name
#ifdef Y_BUILD_CONFIG_DEBUG
    m_pHeightMapTexture->SetDebugName(String::FromFormat("<terrain_cdlod_section_%i_%i_heightmap_lod_%u>", m_pSection->GetSectionX(), m_pSection->GetSectionY(), m_pSection->GetStorageLODLevel()));
#endif

#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Log_ProfilePrintf("PROFILE: Terrain section (%i,%i) heightmap upload took %.4fmsec (streamed LOD %u)", m_pSection->GetSectionX(), m_pSection->GetSectionY(), uploadTimer.GetTimeMilliseconds(), m_pSection->GetStorageLODLevel());
#endif

    return true;
}

bool TerrainSectionRenderProxyCDLOD::CreateNormalMapTexture()
{
#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Timer uploadTimer;
#endif

    uint32 pointCount = m_pSection->GetPointCount();
    DebugAssert(m_pNormalMapTexture == NULL);

    // while we are using a float32 heightmap, we still encode as a rgba8 texture
    const PIXEL_FORMAT pixelFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    uint32 rowPitch = PixelFormat_CalculateRowPitch(pixelFormat, pointCount);
    byte *pPixels = new byte[PixelFormat_CalculateImageSize(pixelFormat, pointCount, pointCount, 1)];
    byte *pPixelPointer = pPixels;
    
    // store normals
    for (uint32 y = 0; y < pointCount; y++)
    {
        uint32 *pRowPointer = reinterpret_cast<uint32 *>(pPixelPointer);
        for (uint32 x = 0; x < pointCount; x++)
        {
            float3 normal(m_pSection->CalculateNormalAtPoint(x, y));
            *(pRowPointer++) = PixelFormatHelpers::EncodeNormalAsRG8B8B8(normal);
        }

        pPixelPointer += rowPitch;
    }

    // create texture
    GPU_TEXTURE2D_DESC textureDesc(pointCount, pointCount, pixelFormat, GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_SHADER_BINDABLE, 1);
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 1, 16, GPU_COMPARISON_FUNC_NEVER);
    if ((m_pNormalMapTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, (const void **)&pPixels, &rowPitch)) == NULL)
    {
        Log_ErrorPrintf("TerrainSectionRendererDataCDLOD::CreateFloatNormalMapTexture: Failed to create GPU texture.");
        delete[] pPixels;
        return false;
    }

    // clear memory
    delete[] pPixels;

    // set debug name
#ifdef Y_BUILD_CONFIG_DEBUG
    m_pNormalMapTexture->SetDebugName(String::FromFormat("<terrain_cdlod_section_%i_%i_normalmap_lod_%u>", m_pSection->GetSectionX(), m_pSection->GetSectionY(), m_pSection->GetStorageLODLevel()));
#endif

#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Log_ProfilePrintf("PROFILE: Terrain section (%i,%i) normalmap upload took %.4fmsec (streamed LOD %u)", m_pSection->GetSectionX(), m_pSection->GetSectionY(), uploadTimer.GetTimeMilliseconds(), m_pSection->GetStorageLODLevel());
#endif

    return true;
}

bool TerrainSectionRenderProxyCDLOD::CreateCombinedHeightNormalMapTexture()
{
    uint32 pointCount = m_pSection->GetPointCount();
    float minHeight = (float)m_pRenderer->GetTerrainParameters()->MinHeight;
    float maxHeight = (float)m_pRenderer->GetTerrainParameters()->MaxHeight;
    float heightRange = (maxHeight - minHeight);
    float inverseHeightRange = 1.0f / heightRange;
    DebugAssert(m_pHeightMapTexture == NULL);

    const PIXEL_FORMAT pixelFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    uint32 rowPitch = PixelFormat_CalculateRowPitch(pixelFormat, pointCount);
    byte *pPixels = new byte[PixelFormat_CalculateImageSize(pixelFormat, pointCount, pointCount, 1)];
    byte *pPixelPointer = pPixels;

    // store height + normal
    for (uint32 y = 0; y < pointCount; y++)
    {
        uint32 *pRowPointer = reinterpret_cast<uint32 *>(pPixelPointer);
        for (uint32 x = 0; x < pointCount; x++)
        {
            float height = m_pSection->GetHeightMapValue(x, y);
            float3 normal(m_pSection->CalculateNormalAtPoint(x, y));

            // clamp to range
            height = Math::Clamp(height, minHeight, maxHeight);

            // split to high and low parts
            float heightFraction = height * inverseHeightRange;
            float height16BitVal = heightFraction * 65535.0f;
            float heightHighPart = height16BitVal / 256.0f;
            float heightLowPart = Y_fmodf(height16BitVal, 256.0f);

            // encode to rgba value
            union
            {
                uint32 rgbaValue;
                uint8 pRGBABytes[4];
            };
            pRGBABytes[0] = (uint8)Math::Truncate(heightHighPart);
            pRGBABytes[1] = (uint8)Math::Truncate(heightLowPart);
            pRGBABytes[2] = (uint8)Math::Truncate(normal.x * 0.5f + 0.5f);
            pRGBABytes[3] = (uint8)Math::Truncate(normal.y * 0.5f + 0.5f);

            // store value         
            *(pRowPointer++) = rgbaValue;
        }

        pPixelPointer += rowPitch;
    }

    // create texture
    GPU_TEXTURE2D_DESC textureDesc(pointCount, pointCount, pixelFormat, GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_SHADER_BINDABLE, 1);
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 1, 16, GPU_COMPARISON_FUNC_NEVER);
    if ((m_pHeightMapTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, (const void **)&pPixels, &rowPitch)) == NULL)
    {
        Log_ErrorPrintf("TerrainSectionRendererDataCDLOD::CreateFloatNormalMapTexture: Failed to create GPU texture.");
        delete[] pPixels;
        return false;
    }

    // set debug name
#ifdef Y_BUILD_CONFIG_DEBUG
    m_pHeightMapTexture->SetDebugName(String::FromFormat("<terrain_cdlod_section_%i_%i_combinedmap_lod_%u>", m_pSection->GetSectionX(), m_pSection->GetSectionY(), m_pSection->GetStorageLODLevel()));
#endif

    // also bind to normal map
    DebugAssert(m_pNormalMapTexture == NULL);
    m_pNormalMapTexture = m_pHeightMapTexture;
    m_pNormalMapTexture->AddRef();

    delete[] pPixels;
    return true;
}

bool TerrainSectionRenderProxyCDLOD::CreateAlphaMapTexture()
{
    if (m_pAlphaMapTexture != nullptr)
        return true;

#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Timer uploadTimer;
#endif

    static const PIXEL_FORMAT splatMapFormats[4 + 1] = { PIXEL_FORMAT_UNKNOWN, PIXEL_FORMAT_R8_UNORM, PIXEL_FORMAT_R8G8_UNORM, PIXEL_FORMAT_R8G8B8A8_UNORM, PIXEL_FORMAT_R8G8B8A8_UNORM };
    DebugAssert(m_pAlphaMapTexture == NULL);

    // calculate number of textures
    uint32 textureDimensions = m_pSection->GetPointCount();
    uint32 nSplatMaps = m_pSection->GetSplatMapCount();
    if (nSplatMaps == 0)
        return false;

    // setup sampler state
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_NEVER);

    // create splat maps
    for (uint32 mapIndex = 0; mapIndex < nSplatMaps; mapIndex++)
    {
        // get pointer
        const void *pDataPointer;
        uint32 valueSize;
        uint32 rowPitch;
        uint32 splatMapChannels;
        m_pSection->GetRawSplatMapData(mapIndex, &pDataPointer, &valueSize, &rowPitch);
        splatMapChannels = m_pSection->GetSplatMapChannelCount(mapIndex);
        DebugAssert(splatMapChannels < countof(splatMapFormats));

        // create the texture
        GPU_TEXTURE2D_DESC textureDesc(textureDimensions, textureDimensions, splatMapFormats[splatMapChannels], GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_SHADER_BINDABLE, 1);
        m_pAlphaMapTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, &pDataPointer, &rowPitch);

        // created?
        if (m_pAlphaMapTexture == NULL)
        {
            Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::CreateAlphaMapTexture: Failed to create alpha map texture.");
            return false;
        }

#ifdef Y_BUILD_CONFIG_DEBUG
        // set debug name
        m_pAlphaMapTexture->SetDebugName(String::FromFormat("<terrain_cdlod_section_%i_%i_alphamap_lod_%u>", m_pSection->GetSectionX(), m_pSection->GetSectionY(), m_pSection->GetStorageLODLevel()));
#endif

//         String str;
//         for (uint32 y = 0; y < textureDimensions; y++)
//         {
//             str.Clear();
//             for (uint32 x = 0; x < textureDimensions; x++)
//             {
//                 if (textureDesc.Format == PIXEL_FORMAT_R8_UNORM)
//                     str.AppendFormattedString("%u ", *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize)));
//                 else if (textureDesc.Format == PIXEL_FORMAT_R8G8_UNORM)
//                     str.AppendFormattedString("%u/%u ", *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize)), *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize) + 1));
//                 else if (textureDesc.Format == PIXEL_FORMAT_R8G8B8A8_UNORM)
//                     str.AppendFormattedString("%u/%u/%u/%u ", *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize)), *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize) + 1), *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize) + 2), *(((const byte *)pDataPointer) + (y * rowPitch) + (x * valueSize) + 3));
//             }
//             Log_DevPrintf("%u: %s", y, str.GetCharArray());
//         }

        // FIXME
        if (mapIndex == 0)
            break;
    }

#ifdef PROFILE_TEXTURE_UPLOAD_TIMES
    Log_ProfilePrintf("PROFILE: Terrain section (%i,%i) alphamap upload took %.4fmsec (streamed LOD %u)", m_pSection->GetSectionX(), m_pSection->GetSectionY(), uploadTimer.GetTimeMilliseconds(), m_pSection->GetStorageLODLevel());
#endif

    return true;
}

bool TerrainSectionRenderProxyCDLOD::CreateRenderMaterial()
{
    if (m_pRenderMaterial != nullptr)
        return true;

    // no layers == no render material
    if (m_pSection->GetSplatMapCount() == 0 || m_pAlphaMapTexture == nullptr || m_pNormalMapTexture == nullptr)
        return false;

    const TerrainLayerList *pLayerList = m_pRenderer->GetLayerList();
    uint32 nPossibleLayers = m_pSection->GetSplatMapCount() * 4;

    // collect the layer indices
    int32 *pLayerIndices = (int32 *)alloca(sizeof(int32) * nPossibleLayers);
    uint32 nLayers = 0;
    for (uint32 mapIndex = 0; mapIndex < m_pSection->GetSplatMapCount(); mapIndex++)
    {
        for (uint32 channelIndex = 0; channelIndex < m_pSection->GetSplatMapChannelCount(mapIndex); channelIndex++)
            pLayerIndices[nLayers++] = m_pSection->GetSplatMapChannel(mapIndex, channelIndex);
    }

    // create the render material
    m_pRenderMaterial = pLayerList->CreateBaseLayerRenderMaterial(pLayerIndices, nLayers, m_pNormalMapTexture, &m_pAlphaMapTexture, 1);
    if (m_pRenderMaterial == nullptr)
    {
        Log_WarningPrintf("TerrainSectionRendererDataCDLOD::CreateRenderMaterial: Could not create materials. Using default material.");
        m_pRenderMaterial = g_pResourceManager->GetDefaultMaterial();
    }

    // ok
    return true;
}

void TerrainSectionRenderProxyCDLOD::CalculateMorphConstants(float cameraNearDistance, float cameraFarDistance) const
{
    uint32 LODCount = m_pSection->GetQuadTree()->GetLODCount();

    // precalculate ratios
    float LODVisRangeDistRatios[TERRAIN_MAX_RENDER_LODS];
    {
        float LODDistanceRatio = CVars::r_terrain_lod_distance_ratio.GetFloat();
        float currentDetailBalance = 1.0f;

        for (uint32 i = 0; i < LODCount - 1; i++)
        {
            LODVisRangeDistRatios[i] = currentDetailBalance;
            if (i == 0)
                LODVisRangeDistRatios[i] *= 0.9f;

            currentDetailBalance *= LODDistanceRatio;
        }

        LODVisRangeDistRatios[LODCount - 1] = currentDetailBalance;
        for (uint32 i = 0; i < LODCount; i++)
            LODVisRangeDistRatios[i] /= currentDetailBalance;
    }

    // precalculate values
    {
        float cameraRange = cameraFarDistance - cameraNearDistance;
        float prevPos = cameraNearDistance;

        cameraRange = Min(cameraRange, CVars::r_terrain_view_distance.GetFloat());

        for (uint32 i = 0; i < LODCount; i++)
        {
            m_visibilityRanges[i] = cameraNearDistance + LODVisRangeDistRatios[i] * cameraRange;
            m_morphEnd[i] = m_visibilityRanges[i];
            prevPos = m_morphStart[i] = prevPos + (m_morphEnd[i] - prevPos) * DEFAULT_TERRAIN_QUADTREE_MORPH_START_RATIO;
        }
    }

    // calculate constants
    for (uint32 i = 0; i < LODCount; i++)
    {
        float start = m_morphStart[i];
        float end = m_morphEnd[i];
        //end = Math::Lerp(end, start, 0.01f);
        float diff = end - start;
        float invDiff = 1.0f / diff;
        m_morphConstants[i].Set(start, invDiff, end / diff, invDiff);
    }
}

void TerrainSectionRenderProxyCDLOD::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!CreateDeviceResources())
        return;

    // Store the requested render passes.
    // Terrain does not currently cast shadows. But it can receieve them.
    // Replace with AO at some point per-vertex?
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT & ~(RENDER_PASS_SHADOW_MAP);

    // early exit?
    wantedRenderPasses &= pRenderQueue->GetAcceptingRenderPassMask();
    if (wantedRenderPasses == 0)
        return;

    // calculate morph constants
    CalculateMorphConstants(pCamera->GetNearPlaneDistance(), pCamera->GetFarPlaneDistance());

    // find sections to draw
    m_pSection->GetQuadTree()->EnumerateNodesToRender(pCamera->GetPosition(), pCamera->GetFrustum(), m_visibilityRanges,
                                                      [this, pCamera, pRenderQueue, wantedRenderPasses](const TerrainQuadTreeNode *pNode, uint32 drawFlags)
    {
        // get pass mask
        uint32 renderPassMask = m_pRenderMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
        if (renderPassMask != 0)
        {
            // get center of the section, todo cache this?
            SIMDVector3f nodeMinBounds(pNode->GetBoundingBox().GetMinBounds());
            SIMDVector3f nodeMaxBounds(pNode->GetBoundingBox().GetMaxBounds());
            SIMDVector3f nodeCenter((nodeMaxBounds - nodeMinBounds) * 0.5f + nodeMinBounds);

            // get view distance
            float viewDistance = pCamera->CalculateDepthToPoint(nodeCenter);

            // create queue entry for base layer
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.pRenderProxy = this;
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(TerrainRendererCDLOD_VertexFactory);
            queueEntry.pMaterial = m_pRenderMaterial;
            queueEntry.BoundingBox = pNode->GetBoundingBox();
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.VertexFactoryFlags = 0;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.TintColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
            queueEntry.UserData[0] = 0;
            queueEntry.UserData[1] = drawFlags;
            queueEntry.UserDataPointer[0] = const_cast<void *>(reinterpret_cast<const void *>(pNode));
            queueEntry.Layer = m_pRenderMaterial->GetShader()->SelectRenderQueueLayer();
            pRenderQueue->AddRenderable(&queueEntry);
        }
    });

    // find detail batches to draw
    if (BuildDetailBatches(g_pRenderer->GetGPUContext(), pCamera->GetPosition()))
    {
        // get section center
        float3 sectionCenter(m_pSection->GetBoundingBox().GetCenter());
        float sectionViewDistance = pCamera->CalculateDepthToPoint(sectionCenter);

        // add batches
        for (uint32 detailBatchIndex = 0; detailBatchIndex < m_detailBatches.GetSize(); detailBatchIndex++)
        {
            const DetailBatch &detailBatch = m_detailBatches[detailBatchIndex];
            const TerrainSection::DetailMesh *pDetailMesh = m_pSection->GetDetailMesh(detailBatch.MeshIndex);
            const StaticMesh *pStaticMesh = pDetailMesh->pStaticMesh;

            // todo: select lod based on center of section distance
            uint32 meshLOD = 0;

            // add mesh batches
            const StaticMesh::LOD *pMeshLOD = pStaticMesh->GetLOD(meshLOD);
            for (uint32 meshBatchIndex = 0; meshBatchIndex < pMeshLOD->GetBatchCount(); meshBatchIndex++)
            {
                const StaticMesh::Batch *pMeshBatch = pMeshLOD->GetBatch(meshBatchIndex);
                const Material *pBatchMaterial = pStaticMesh->GetMaterial(pMeshBatch->MaterialIndex);

                // get pass mask
                uint32 renderPassMask = pBatchMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
                if (renderPassMask != 0)
                {
                    // create queue entry for base layer
                    RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
                    queueEntry.pRenderProxy = this;
                    queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
                    queueEntry.pMaterial = pBatchMaterial;
                    queueEntry.BoundingBox = m_pSection->GetBoundingBox();
                    queueEntry.RenderPassMask = renderPassMask;
                    queueEntry.VertexFactoryFlags = pStaticMesh->GetVertexFactoryFlags() | LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX;
                    queueEntry.ViewDistance = sectionViewDistance;
                    queueEntry.TintColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
                    queueEntry.UserData[0] = 1 + detailBatchIndex;
                    queueEntry.UserData[1] = meshLOD;
                    queueEntry.UserData[2] = meshBatchIndex;
                    queueEntry.Layer = m_pRenderMaterial->GetShader()->SelectRenderQueueLayer();
                    pRenderQueue->AddRenderable(&queueEntry);
                }
            }
        }
    }            

    // debug info
    if (pRenderQueue->IsAcceptingDebugObjects() && CVars::r_terrain_show_nodes.GetBool())
        pRenderQueue->AddDebugInfoObject(this);
}

void TerrainSectionRenderProxyCDLOD::SetupTerrainDraw(const TerrainQuadTreeNode *pNode, GPUContext *pDevice, ShaderProgram *pShaderProgram) const
{
    // precalculate these values, quadtree is always in lod0 sizes
    float inverseTextureSize = 1.0f / (float)(m_pSection->GetPointCount());
    float nodeSizeInTextureSpace = (float)(pNode->GetNodeSize()) * inverseTextureSize;
    float textureStartU = (float)pNode->GetStartQuadX() * inverseTextureSize + 0.5f * inverseTextureSize;
    float textureStartV = (float)pNode->GetStartQuadY() * inverseTextureSize + 0.5f * inverseTextureSize;

    // uniforms
    float3 worldSpaceRectOffset(pNode->GetBoundingBox().GetMinBounds().xy(), (m_pSection->GetHeightStorageFormat() == TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32) ? 0.0f : (float)m_pRenderer->GetTerrainParameters()->MinHeight);
    float3 worldSpaceRectSize(pNode->GetBoundingBox().GetMaxBounds().xy() - pNode->GetBoundingBox().GetMinBounds().xy(), (m_pSection->GetHeightStorageFormat() == TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32) ? 1.0f : (float)(m_pRenderer->GetTerrainParameters()->MaxHeight - m_pRenderer->GetTerrainParameters()->MinHeight));
    float2 textureSpaceRectOffset(textureStartU, textureStartV);
    float2 textureSpaceRectSize(nodeSizeInTextureSpace, nodeSizeInTextureSpace);
    float2 heightmapDimensions((float)m_pSection->GetPointCount(), 1.0f / (float)m_pSection->GetPointCount());
    float3 gridDimensions((float)m_pRenderer->GetGridSize(), (float)m_pRenderer->GetGridSize() * 0.5f, 2.0f / (float)m_pRenderer->GetGridSize());
    float4 morphConstants(m_morphConstants[pNode->GetLODLevel()]);

    // set uniforms
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 0, SHADER_PARAMETER_TYPE_FLOAT3, &worldSpaceRectOffset);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 1, SHADER_PARAMETER_TYPE_FLOAT3, &worldSpaceRectSize);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 2, SHADER_PARAMETER_TYPE_FLOAT2, &textureSpaceRectOffset);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 3, SHADER_PARAMETER_TYPE_FLOAT2, &textureSpaceRectSize);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 4, SHADER_PARAMETER_TYPE_FLOAT2, &heightmapDimensions);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 5, SHADER_PARAMETER_TYPE_FLOAT3, &gridDimensions);
    pShaderProgram->SetVertexFactoryParameterValue(pDevice, 6, SHADER_PARAMETER_TYPE_FLOAT4, &morphConstants);
    pShaderProgram->SetVertexFactoryParameterResource(pDevice, 7, m_pHeightMapTexture);
    pShaderProgram->SetVertexFactoryParameterResource(pDevice, 8, m_pHeightMapTexture);

    // setup draw
    m_pRenderer->BindGridBuffers(pDevice);
    pDevice->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
}

void TerrainSectionRenderProxyCDLOD::InvokeTerrainDraw(GPUContext *pDevice, uint32 drawFlags) const
{
#if 1
    if (drawFlags == TerrainQuadTreeQuery::DrawFlagSingleQuad)
    {
        pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndBR(), m_pRenderer->GetGridIndexCountSingleQuad(), 0);
    }
    else if (drawFlags == TerrainQuadTreeQuery::DrawFlagAll)
    {
        pDevice->DrawIndexed(0, m_pRenderer->GetGridIndexCountPerSubQuad() * 4, 0);
    }
    else
    {
        const uint32 gridIndexCountPerSubQuad = m_pRenderer->GetGridIndexCountPerSubQuad();

        if (drawFlags & TerrainQuadTreeQuery::DrawFlagTopLeft)
        {
            if (drawFlags & TerrainQuadTreeQuery::DrawFlagTopRight)
            {
                if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomLeft)
                {
                    if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomRight)
                    {
                        // TL + TR + BL + BR
                        pDevice->DrawIndexed(0, gridIndexCountPerSubQuad * 4, 0);
                        drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopLeft | TerrainQuadTreeQuery::DrawFlagTopRight | TerrainQuadTreeQuery::DrawFlagBottomLeft | TerrainQuadTreeQuery::DrawFlagBottomRight);
                    }
                    else
                    {
                        // TL + TR + BL
                        pDevice->DrawIndexed(0, gridIndexCountPerSubQuad * 3, 0);
                        drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopLeft | TerrainQuadTreeQuery::DrawFlagTopRight | TerrainQuadTreeQuery::DrawFlagBottomLeft);
                    }
                }
                else
                {
                    // TL + TR
                    pDevice->DrawIndexed(0, gridIndexCountPerSubQuad * 2, 0);
                    drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopLeft | TerrainQuadTreeQuery::DrawFlagTopRight);
                }
            }
            else
            {
                // TL
                pDevice->DrawIndexed(0, gridIndexCountPerSubQuad, 0);
                drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopLeft);
            }
        }

        if (drawFlags & TerrainQuadTreeQuery::DrawFlagTopRight)
        {
            if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomLeft)
            {
                if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomRight)
                {
                    // TR + BL + BR
                    pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTL(), gridIndexCountPerSubQuad * 3, 0);
                    drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopRight | TerrainQuadTreeQuery::DrawFlagBottomLeft | TerrainQuadTreeQuery::DrawFlagBottomRight);
                }
                else
                {
                    // TR + BL
                    pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTL(), gridIndexCountPerSubQuad * 2, 0);
                    drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopRight | TerrainQuadTreeQuery::DrawFlagBottomLeft);
                }
            }
            else
            {
                // TR
                pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTL(), gridIndexCountPerSubQuad, 0);
                drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagTopRight);
            }
        }

        if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomLeft)
        {
            if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomRight)
            {
                // BL + BR
                pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTR(), gridIndexCountPerSubQuad * 2, 0);
                drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagBottomLeft | TerrainQuadTreeQuery::DrawFlagBottomRight);
            }
            else
            {
                // BL
                pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTR(), gridIndexCountPerSubQuad, 0);
                drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagBottomLeft);
            }
        }

        if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomRight)
        {
            // BR
            pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndBL(), gridIndexCountPerSubQuad, 0);
            drawFlags &= ~(TerrainQuadTreeQuery::DrawFlagBottomRight);
        }
    }

#elif 0

    static const uint32 drawFlagsConst[4] = { TerrainQuadTreeQuery::DrawFlagTopLeft, TerrainQuadTreeQuery::DrawFlagTopRight, TerrainQuadTreeQuery::DrawFlagBottomLeft, TerrainQuadTreeQuery::DrawFlagBottomRight };
    const uint32 gridIndexCountPerSubQuad = m_pRenderer->GetGridIndexCountPerSubQuad();

    for (uint32 i = 0; i < 4; i++)
    {
        if (drawFlags & drawFlagsConst[i])
        {
            uint32 batchStart = 0;
            switch (i)
            {
            case 1: batchStart = m_pRenderer->GetGridIndexEndTL();  break;
            case 2: batchStart = m_pRenderer->GetGridIndexEndTR();  break;
            case 3: batchStart = m_pRenderer->GetGridIndexEndBL();  break;
            }

            uint32 batchCount = gridIndexCountPerSubQuad;
            for (uint32 j = i + 1; j < 4; j++)
            {
                if (drawFlags & drawFlagsConst[j])
                {
                    batchCount += gridIndexCountPerSubQuad;
                    drawFlags &= ~drawFlagsConst[j];
                }
            }

            pDevice->DrawIndexed(batchStart, batchCount, 0);
        }
    }

#else

    if (drawFlags & TerrainQuadTreeQuery::DrawFlagTopLeft)
        pDevice->DrawIndexed(0, m_pRenderer->GetGridIndexCountPerSubQuad(), 0);
    if (drawFlags & TerrainQuadTreeQuery::DrawFlagTopRight)
        pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTL(), m_pRenderer->GetGridIndexCountPerSubQuad(), 0);
    if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomLeft)
        pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndTR(), m_pRenderer->GetGridIndexCountPerSubQuad(), 0);
    if (drawFlags & TerrainQuadTreeQuery::DrawFlagBottomRight)
        pDevice->DrawIndexed(m_pRenderer->GetGridIndexEndBL(), m_pRenderer->GetGridIndexCountPerSubQuad(), 0);

#endif
}

void TerrainSectionRenderProxyCDLOD::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    // height layer?
    if (pQueueEntry->UserData[0] == 0)
    {
        const TerrainQuadTreeNode *pNode = reinterpret_cast<const TerrainQuadTreeNode *>(pQueueEntry->UserDataPointer[0]);
        SetupTerrainDraw(pNode, pGPUContext, pShaderProgram);
    }
    else
    {
        SetupDetailBatchDraw(pQueueEntry->UserData[0] - 1, pQueueEntry->UserData[1], pQueueEntry->UserData[2], pGPUContext, pShaderProgram);
    }
}

void TerrainSectionRenderProxyCDLOD::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    // base layer?
    if (pQueueEntry->UserData[0] == 0)
    {
        uint32 nodeDrawFlags = pQueueEntry->UserData[1];
        InvokeTerrainDraw(pGPUContext, nodeDrawFlags);
    }
    else
    {
        InvokeDetailBatchDraw(pQueueEntry->UserData[0] - 1, pQueueEntry->UserData[1], pQueueEntry->UserData[2], pGPUContext);
    }
}

void TerrainSectionRenderProxyCDLOD::DrawDebugInfo(const Camera *pCamera, GPUContext *pGPUContext, MiniGUIContext *pGUIContext) const
{
    if (CVars::r_terrain_show_nodes.GetBool())
    {
        pGUIContext->PushManualFlush();
        pGUIContext->SetAlphaBlendingEnabled(false);

        /*
        SmallString text;
        text.Format("flags = %u node from %u,%u to %u,%u [lod %u] @ section (%i, %i)", drawFlags, pNode->GetStartQuadX(), pNode->GetStartQuadY(), pNode->GetStartQuadX() + pNode->GetNodeSize(), pNode->GetStartQuadY() + pNode->GetNodeSize(), pNode->GetLODLevel(), m_pSection->GetSectionX(), m_pSection->GetSectionY());
        pGUIContext->DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), 12, 0, nodeCount * 13 + 32, text, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), false, MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
        nodeCount++;
        */

        // find sections to draw
        m_pSection->GetQuadTree()->EnumerateNodesToRender(pCamera->GetPosition(), pCamera->GetFrustum(), m_visibilityRanges,
                                                          [this, pGUIContext](const TerrainQuadTreeNode *pNode, uint32 drawFlags)
        {
            float3 startLODColor(1.0f, 0.0f, 0.0f);
            float3 endLODColor(0.0f, 1.0f, 0.0f);

            float3 minBounds(pNode->GetBoundingBox().GetMinBounds());
            float3 maxBounds(pNode->GetBoundingBox().GetMaxBounds());
            float3 color(startLODColor.Lerp(endLODColor, (float)pNode->GetLODLevel() / (float)(m_pSection->GetQuadTree()->GetLODCount() - 1)));

            pGUIContext->Draw3DWireBox(minBounds, maxBounds, MAKE_COLOR_R8G8B8A8_UNORM(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, 255));
        });

        pGUIContext->PopManualFlush();
    }
}

bool TerrainSectionRenderProxyCDLOD::CreateDetailInstanceBuffer()
{
    DebugAssert(m_pDetailInstanceBuffer == nullptr);

    // count the number of total instances
    uint32 instanceCount = m_pSection->GetDetailMeshInstanceCount();
    if (instanceCount == 0)
        return true;

    // allocate a buffer with enough space to fill these
    const uint32 INSTANCE_SIZE = sizeof(float3x4);
    GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, INSTANCE_SIZE * instanceCount);
    m_pDetailInstanceBuffer = g_pRenderer->CreateBuffer(&bufferDesc, nullptr);
    if (m_pDetailInstanceBuffer == nullptr)
    {
        Log_ErrorPrint("TerrainSectionRenderProxyCDLOD::CreateDetailInstanceBuffer: Failed to allocate GPU buffer");
        return false;
    }

    // reserve enough batches
    m_detailBatches.Reserve(m_pSection->GetDetailMeshCount());
    Log_DevPrintf("allocate detail buffer size %s", StringConverter::SizeToHumanReadableString(bufferDesc.Size).GetCharArray());
    return true;
}

bool TerrainSectionRenderProxyCDLOD::BuildDetailBatches(GPUContext *pGPUContext, const float3 &eyePosition) const
{
    // any detail instances?
    if (m_pDetailInstanceBuffer == nullptr)
        return false;

    // map the buffer
    byte *pBufferPtr;
    if (!pGPUContext->MapBuffer(m_pDetailInstanceBuffer, GPU_MAP_TYPE_WRITE_DISCARD, reinterpret_cast<void **>(&pBufferPtr)))
    {
        Log_ErrorPrintf("TerrainSectionRenderProxyCDLOD::BuildDetailBatches: Failed to map GPU buffer");
        return false;
    }

    // batch info
    uint32 lastMesh = Y_UINT32_MAX;
    float cullDistanceSq = Y_FLT_INFINITE;
    DetailBatch detailBatch;
    Y_memzero(&detailBatch, sizeof(detailBatch));
    m_detailBatches.Clear();

    // loop through instances, grab those in range and add them to the buffer
    byte *pCurrentBufferPtr = pBufferPtr;
    for (uint32 meshInstanceIndex = 0; meshInstanceIndex < m_pSection->GetDetailMeshInstanceCount(); meshInstanceIndex++)
    {
        const TerrainSection::DetailMeshInstance *meshInstance = m_pSection->GetDetailMeshInstance(meshInstanceIndex);

        // new mesh index?
        if (meshInstance->MeshIndex != lastMesh)
        {
            if (detailBatch.InstanceCount > 0)
                m_detailBatches.Add(detailBatch);

            // get info for new mesh
            detailBatch.MeshIndex = meshInstance->MeshIndex;
            detailBatch.InstanceBufferOffset = static_cast<uint32>(pCurrentBufferPtr - pBufferPtr);
            detailBatch.InstanceCount = 0;
            lastMesh = meshInstance->MeshIndex;
            cullDistanceSq = Math::Square(m_pSection->GetDetailMesh(lastMesh)->DrawDistance);
        }

        // test distance
        float instanceDistanceSq = eyePosition.SquaredDistance(meshInstance->Position);
        if (instanceDistanceSq > cullDistanceSq)
            continue;
        
        // add it
        Y_memcpy(pCurrentBufferPtr, &meshInstance->TransformMatrix, sizeof(float3x4));
        pCurrentBufferPtr += sizeof(float3x4);
        detailBatch.InstanceCount++;
    }

    // last batch
    if (detailBatch.InstanceCount > 0)
        m_detailBatches.Add(detailBatch);

    // unmap buffer
    pGPUContext->Unmapbuffer(m_pDetailInstanceBuffer, pBufferPtr);
    return (m_detailBatches.GetSize() > 0);
}

void TerrainSectionRenderProxyCDLOD::SetupDetailBatchDraw(uint32 detailBatchIndex, uint32 meshLODIndex, uint32 meshBatchIndex, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    const DetailBatch &detailBatch = m_detailBatches[detailBatchIndex];
    const TerrainSection::DetailMesh *pDetailMesh = m_pSection->GetDetailMesh(detailBatchIndex);
    const StaticMesh::LOD *pLOD = pDetailMesh->pStaticMesh->GetLOD(meshLODIndex);

    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    pGPUContext->SetVertexBuffer(0, pLOD->GetVertexBuffers()->GetBuffer(0), pLOD->GetVertexBuffers()->GetBufferOffset(0), pLOD->GetVertexBuffers()->GetBufferStride(0));
    pGPUContext->SetVertexBuffer(1, m_pDetailInstanceBuffer, detailBatch.InstanceBufferOffset, sizeof(LocalVertexFactory::InstanceTransform));
    pGPUContext->SetIndexBuffer(pLOD->GetIndexBuffer(), pLOD->GetIndexFormat(), 0);
}

void TerrainSectionRenderProxyCDLOD::InvokeDetailBatchDraw(uint32 detailBatchIndex, uint32 meshLODIndex, uint32 meshBatchIndex, GPUContext *pGPUContext) const
{
    const DetailBatch &detailBatch = m_detailBatches[detailBatchIndex];
    const TerrainSection::DetailMesh *pDetailMesh = m_pSection->GetDetailMesh(detailBatchIndex);
    const StaticMesh::Batch *pBatch = pDetailMesh->pStaticMesh->GetLOD(meshLODIndex)->GetBatch(meshBatchIndex);

    pGPUContext->DrawIndexedInstanced(pBatch->StartIndex, pBatch->NumIndices, 0, detailBatch.InstanceCount);
}
