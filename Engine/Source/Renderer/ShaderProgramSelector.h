#pragma once
#include "Renderer/RendererTypes.h"

class ShaderProgram;
class GPUContext;
class GPUResource;
class GPUSamplerState;
class ShaderComponentTypeInfo;
class VertexFactoryTypeInfo;
class GPUShaderProgram;
class MaterialShader;
class Material;
struct RENDER_QUEUE_RENDERABLE_ENTRY;

class ShaderProgramSelector
{
public:
    ShaderProgramSelector(uint32 baseGlobalFlags);

    // state switchers
    void SetGlobalFlags(uint32 globalFlags);
    void SetBaseShader(const ShaderComponentTypeInfo *pBaseShaderType, uint32 baseShaderFlags);
    void SetVertexFactory(const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags);
    void SetMaterial(const Material *pMaterial);

    // state switcher from render queue entry
    void SetQueueEntry(const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);

    // shader accessors
    ShaderProgram *MakeActive(GPUCommandList *pCommandList);

private:
    enum DIRTY_FLAGS
    {
        DirtyGlobalFlags = (1 << 0),
        DirtyBaseShader = (1 << 1),
        DirtyVertexFactory = (1 << 2),
        DirtyMaterialShader = (1 << 3),
        DirtyMaterial = (1 << 4)
    };

    const ShaderComponentTypeInfo *m_pBaseShaderType;
    const VertexFactoryTypeInfo *m_pVertexFactoryType;
    const Material *m_pMaterial;
    const MaterialShader *m_pMaterialShader;
    ShaderProgram *m_pCurrentProgram;
    uint32 m_baseGlobalFlags;
    uint32 m_globalShaderFlags;
    uint32 m_baseShaderFlags;
    uint32 m_vertexFactoryFlags;
    uint32 m_materialStaticSwitchMask;
    uint32 m_dirtyFlags;
};
