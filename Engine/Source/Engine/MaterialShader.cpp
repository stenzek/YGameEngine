#include "Engine/PrecompiledHeader.h"
#include "Engine/MaterialShader.h"
#include "Engine/Material.h"
#include "Engine/Texture.h"
#include "Engine/DataFormats.h"
#include "Renderer/VertexFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderQueue.h"
//Log_SetChannel(MaterialShader);

DEFINE_RESOURCE_TYPE_INFO(MaterialShader);
DEFINE_RESOURCE_GENERIC_FACTORY(MaterialShader);

MaterialShader::MaterialShader(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pResourceTypeInfo)
{
    m_blendingMode = MATERIAL_BLENDING_MODE_NONE;
    m_lightingType = MATERIAL_LIGHTING_TYPE_REFLECTIVE;
    m_lightingModel = MATERIAL_LIGHTING_MODEL_PHONG;
    m_lightingNormalSpace = MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE;
    m_renderMode = MATERIAL_RENDER_MODE_NORMAL;
    m_renderLayer = MATERIAL_RENDER_LAYER_NORMAL;
    m_twoSided = false;
    m_depthClamping = false;
    m_depthTests = true;
    m_depthWrites = true;
    m_castShadows = true;
    m_receiveShadows = true;
}

MaterialShader::~MaterialShader()
{
    ReleaseDeviceResources();
}

bool MaterialShader::Load(const char *resourceName, ByteStream *pStream)
{
    BinaryReader binaryReader(pStream);

    // set name
    m_strName = resourceName;

    // read header
    DF_MATERIAL_SHADER_HEADER header;
    if (!binaryReader.SafeReadBytes(&header, sizeof(header)) || header.Magic != DF_MATERIAL_SHADER_HEADER_MAGIC || header.HeaderSize != sizeof(header))
        return false;

    // copy properties
    m_blendingMode = (MATERIAL_BLENDING_MODE)header.BlendingMode;
    m_lightingType = (MATERIAL_LIGHTING_TYPE)header.LightingType;
    m_lightingModel = (MATERIAL_LIGHTING_MODEL)header.LightingModel;
    m_lightingNormalSpace = (MATERIAL_LIGHTING_NORMAL_SPACE)header.LightingNormalSpace;
    m_renderMode = (MATERIAL_RENDER_MODE)header.RenderMode;
    m_renderLayer = (MATERIAL_RENDER_LAYER)header.RenderLayer;
    m_twoSided = header.TwoSided;
    m_depthClamping = header.DepthClamping;
    m_depthTests = header.DepthTests;
    m_depthWrites = header.DepthWrites;
    m_castShadows = header.CastShadows;
    m_receiveShadows = header.ReceiveShadows;
    m_sourceCRC = header.SourceCRC;

    // copy uniforms
    m_UniformParameters.Resize(header.UniformParameterCount);
    for (uint32 i = 0; i < m_UniformParameters.GetSize(); i++)
    {
        DF_MATERIAL_SHADER_UNIFORM_PARAMETER uniformHeader;
        if (!binaryReader.SafeReadBytes(&uniformHeader, sizeof(uniformHeader)))
            return false;

        UniformParameter *pDestinationUniformParameter = &m_UniformParameters[i];
        pDestinationUniformParameter->Index = i;
        pDestinationUniformParameter->Type = (SHADER_PARAMETER_TYPE)uniformHeader.Type;
        Y_memcpy(&pDestinationUniformParameter->DefaultValue, &uniformHeader.DefaultValue, sizeof(pDestinationUniformParameter->DefaultValue));
        if (!binaryReader.SafeReadFixedString(uniformHeader.NameLength, &pDestinationUniformParameter->Name))
            return false;
    }

    // copy textures
    m_TextureParameters.Resize(header.TextureParameterCount);
    for (uint32 i = 0; i < m_TextureParameters.GetSize(); i++)
    {
        DF_MATERIAL_SHADER_TEXTURE_PARAMETER textureHeader;
        if (!binaryReader.SafeReadBytes(&textureHeader, sizeof(textureHeader)))
            return false;

        TextureParameter *pDestinationTextureParameter = &m_TextureParameters[i];
        pDestinationTextureParameter->Index = i;
        pDestinationTextureParameter->Type = (TEXTURE_TYPE)textureHeader.Type;
        if (!binaryReader.SafeReadFixedString(textureHeader.NameLength, &pDestinationTextureParameter->Name) ||
            !binaryReader.SafeReadFixedString(textureHeader.DefaultValueLength, &pDestinationTextureParameter->DefaultValue))
        {
            return false;
        }
    }

    // copy static switches
    m_StaticSwitchParameters.Resize(header.StaticSwitchParameterCount);
    for (uint32 i = 0; i < m_StaticSwitchParameters.GetSize(); i++)
    {
        DF_MATERIAL_SHADER_STATIC_SWITCH_PARAMETER staticSwitchHeader;
        if (!binaryReader.SafeReadBytes(&staticSwitchHeader, sizeof(staticSwitchHeader)))
            return false;

        StaticSwitchParameter *pDestinationStaticSwitchParameter = &m_StaticSwitchParameters[i];
        pDestinationStaticSwitchParameter->Index = i;
        pDestinationStaticSwitchParameter->Mask = staticSwitchHeader.Mask;
        pDestinationStaticSwitchParameter->DefaultValue = staticSwitchHeader.DefaultValue;
        if (!binaryReader.SafeReadFixedString(staticSwitchHeader.NameLength, &pDestinationStaticSwitchParameter->Name))
            return false;
    }

    // ok
    return true;
}

int32 MaterialShader::FindUniformParameter(const char *Name) const
{
    uint32 i;
    for (i = 0; i < m_UniformParameters.GetSize(); i++)
    {
        if (m_UniformParameters[i].Name.Compare(Name))
            return (int32)i;
    }

    return -1;
}

int32 MaterialShader::FindTextureParameter(const char *Name) const
{
    uint32 i;
    for (i = 0; i < m_TextureParameters.GetSize(); i++)
    {
        if (m_TextureParameters[i].Name.Compare(Name))
            return (int32)i;
    }

    return -1;
}

int32 MaterialShader::FindStaticSwitchParameter(const char *Name) const
{
    uint32 i;
    for (i = 0; i < m_StaticSwitchParameters.GetSize(); i++)
    {
        if (m_StaticSwitchParameters[i].Name.Compare(Name))
            return (int32)i;
    }

    return -1;
}

bool MaterialShader::CreateDeviceResources() const
{
    return true;
}

void MaterialShader::ReleaseDeviceResources() const
{
    m_ShaderMap.ReleaseGPUResources();
}

uint32 MaterialShader::SelectRenderPassMask(uint32 wantedPassMask) const
{
    uint32 newPassMask = wantedPassMask;

//     // for blended objects, we need to disable the base pass, as we don't write depth values
//     switch (m_eBlendingMode)
//     {
//     case MATERIAL_BLENDING_MODE_STRAIGHT:
//     case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
//         newPassMask &= ~RENDER_PASS_Z_PREPASS;
//         break;
//     }

    switch (m_lightingType)
    {
    case MATERIAL_LIGHTING_TYPE_EMISSIVE:
        newPassMask &= ~(RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING);
        break;

    case MATERIAL_LIGHTING_TYPE_REFLECTIVE:
    case MATERIAL_LIGHTING_TYPE_REFLECTIVE_TWO_SIDED:
        newPassMask &= ~(RENDER_PASS_EMISSIVE);
        break;

    case MATERIAL_LIGHTING_TYPE_REFLECTIVE_EMISSIVE:
        break;
    }

    // custom lighting cannot use lightmaps
    if (m_lightingModel == MATERIAL_LIGHTING_MODEL_CUSTOM)
        newPassMask &= ~(RENDER_PASS_LIGHTMAP);

    if (!m_castShadows)
        newPassMask &= ~(RENDER_PASS_SHADOW_MAP);

    if (!m_receiveShadows)
        newPassMask &= ~(RENDER_PASS_SHADOWED_LIGHTING);

    return newPassMask;
}

uint8 MaterialShader::SelectRenderQueueLayer() const
{
    switch (m_renderLayer)
    {
    case MATERIAL_RENDER_LAYER_SKYBOX:
        return RENDER_QUEUE_LAYER_SKYBOX;
    }

    return RENDER_QUEUE_LAYER_NONE;
}

GPUBlendState *MaterialShader::SelectBlendState() const
{
    switch (m_blendingMode)
    {
    case MATERIAL_BLENDING_MODE_ADDITIVE:
        return g_pRenderer->GetFixedResources()->GetBlendStateAdditive();

    case MATERIAL_BLENDING_MODE_STRAIGHT:
        return g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending();

    case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
        return g_pRenderer->GetFixedResources()->GetBlendStatePremultipliedAlpha();

    case MATERIAL_BLENDING_MODE_SOFTMASKED:
        return g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending();

    //case MATERIAL_BLENDING_MODE_NONE:
    //case MATERIAL_BLENDING_MODE_MASKED: <-- alphakill will take care of this
    default:
        return g_pRenderer->GetFixedResources()->GetBlendStateNoBlending();
    }
}

GPURasterizerState *MaterialShader::SelectRasterizerState(RENDERER_FILL_MODE fillMode /* = RENDERER_FILL_SOLID */, RENDERER_CULL_MODE cullMode /* = RENDERER_CULL_BACK */, bool depthBias /* = false */, bool scissorTest /* = false */) const
{
    return g_pRenderer->GetFixedResources()->GetRasterizerState((m_renderMode == MATERIAL_RENDER_MODE_WIREFRAME) ? RENDERER_FILL_WIREFRAME : fillMode, 
                                                                (m_twoSided) ? RENDERER_CULL_NONE : cullMode,
                                                                depthBias,
                                                                m_depthClamping,
                                                                scissorTest);
}

GPUDepthStencilState *MaterialShader::SelectDepthStencilState(bool depthTests /*= true*/, bool depthWrites /*= true*/, GPU_COMPARISON_FUNC comparisonFunc /*= GPU_COMPARISON_FUNC_LESS*/) const
{
    return g_pRenderer->GetFixedResources()->GetDepthStencilState(depthTests & m_depthTests, 
                                                                  depthWrites & m_depthWrites,
                                                                  comparisonFunc);
}

