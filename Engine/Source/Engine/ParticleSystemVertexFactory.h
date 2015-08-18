#pragma once
#include "Renderer/VertexFactory.h"
#include "Engine/ParticleSystemCommon.h"

class Camera;

class ParticleSystemSpriteVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(ParticleSystemSpriteVertexFactory, VertexFactory);

    enum Flags
    {
        // rendering with per-frame data in dynamic buffer
        Flag_RenderBasic = (1 << 0),

        // rendering with position, attributes in vertex buffer and system value to determine edges
        Flag_RenderInstancedQuads = (1 << 1),

        // enable world transform
        Flag_UseWorldTransform = (1 << 1),
    };

public:
    static uint32 GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static uint32 GetVerticesPerSprite(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static DRAW_TOPOLOGY GetDrawTopology(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static bool FillVertexBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Camera *pCamera, const ParticleData *pParticles, uint32 nParticles, void *pBuffer, uint32 bufferSize);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);
};
