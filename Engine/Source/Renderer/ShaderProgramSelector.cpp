#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderQueue.h"
#include "Engine/MaterialShader.h"
#include "Engine/Material.h"
Log_SetChannel(ShaderProgramSelector);

ShaderProgramSelector::ShaderProgramSelector(uint32 baseGlobalFlags)
    : m_pBaseShaderType(nullptr),
      m_pVertexFactoryType(nullptr),
      m_pMaterial(nullptr),
      m_pMaterialShader(nullptr),
      m_pCurrentProgram(nullptr),
      m_baseGlobalFlags(baseGlobalFlags),
      m_globalShaderFlags(0),
      m_baseShaderFlags(0),
      m_vertexFactoryFlags(0),
      m_materialStaticSwitchMask(0),
      m_dirtyFlags(DirtyGlobalFlags | DirtyBaseShader | DirtyVertexFactory | DirtyMaterialShader | DirtyMaterial)
{

}

void ShaderProgramSelector::SetGlobalFlags(uint32 globalFlags)
{
    uint32 newGlobalFlags = m_baseGlobalFlags | globalFlags;
    if (newGlobalFlags != m_globalShaderFlags)
    {
        m_globalShaderFlags = newGlobalFlags;
        m_dirtyFlags = DirtyGlobalFlags;
    }
}

void ShaderProgramSelector::SetBaseShader(const ShaderComponentTypeInfo *pBaseShaderType, uint32 baseShaderFlags)
{
    if (m_pBaseShaderType != pBaseShaderType || m_baseShaderFlags != baseShaderFlags)
    {
        m_pBaseShaderType = pBaseShaderType;
        m_baseShaderFlags = baseShaderFlags;
        m_dirtyFlags |= DirtyBaseShader;
    }
}

void ShaderProgramSelector::SetVertexFactory(const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags)
{
    if (m_pVertexFactoryType != pVertexFactoryTypeInfo || m_vertexFactoryFlags != vertexFactoryFlags)
    {
        m_pVertexFactoryType = pVertexFactoryTypeInfo;
        m_vertexFactoryFlags = vertexFactoryFlags;
        m_dirtyFlags |= DirtyVertexFactory;
    }
}

void ShaderProgramSelector::SetMaterial(const Material *pMaterial)
{
    if (m_pMaterial != pMaterial)
    {
        m_pMaterial = pMaterial;
        if (pMaterial != nullptr)
        {
            if (pMaterial->GetShader() != m_pMaterialShader)
            {
                m_pMaterialShader = pMaterial->GetShader();
                m_materialStaticSwitchMask = pMaterial->GetShaderStaticSwitchMask();
                m_dirtyFlags |= DirtyMaterialShader | DirtyMaterial;
            }
            else if (pMaterial->GetShaderStaticSwitchMask() != m_materialStaticSwitchMask)
            {
                m_materialStaticSwitchMask = pMaterial->GetShaderStaticSwitchMask();
                m_dirtyFlags |= DirtyMaterialShader | DirtyMaterial;
            }
            else
            {
                m_dirtyFlags |= DirtyMaterial;
            }
        }
        else
        {
            m_pMaterialShader = nullptr;
            m_materialStaticSwitchMask = 0;
            m_dirtyFlags |= DirtyMaterialShader | DirtyMaterial;
        }
    }
}

void ShaderProgramSelector::SetQueueEntry(const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    uint32 shaderGlobalFlags = 0;
    if (pQueueEntry->RenderPassMask & RENDER_PASS_TINT)
        shaderGlobalFlags |= SHADER_GLOBAL_FLAG_MATERIAL_TINT;

    SetGlobalFlags(shaderGlobalFlags);
    SetMaterial(pQueueEntry->pMaterial);
    SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
}

ShaderProgram *ShaderProgramSelector::MakeActive(GPUContext *pContext)
{
    if (m_dirtyFlags == 0)
        return m_pCurrentProgram;

    uint32 dirtyFlags = m_dirtyFlags;
    m_dirtyFlags = 0;

    if (dirtyFlags & (DirtyGlobalFlags | DirtyBaseShader | DirtyVertexFactory | DirtyMaterialShader))
    {
        m_pCurrentProgram = g_pRenderer->GetShaderProgram(m_globalShaderFlags, m_pBaseShaderType, m_baseShaderFlags, m_pVertexFactoryType, m_vertexFactoryFlags, m_pMaterialShader, m_materialStaticSwitchMask);
        if (m_pCurrentProgram == nullptr)
            return nullptr;

        // bind shader
        pContext->SetShaderProgram(m_pCurrentProgram->GetGPUProgram());
    }

    // bind materials
    if ((dirtyFlags & DirtyMaterial) && m_pMaterial != nullptr)
    {
        if (m_pCurrentProgram == nullptr || !m_pMaterial->BindDeviceResources(pContext, m_pCurrentProgram))
            return nullptr;
    }

    return m_pCurrentProgram;
}
