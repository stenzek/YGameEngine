#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphCompiler.h"

ShaderGraphCompiler::ShaderGraphCompiler(const ShaderGraph *pShaderGraph)
    : m_pShaderGraph(pShaderGraph)
{

}

ShaderGraphCompiler::~ShaderGraphCompiler()
{

}

const ShaderGraphCompiler::ExternalParameter *ShaderGraphCompiler::GetExternalUniformParameter(const char *uniformName) const
{
    for (ExternalParameterList::ConstIterator itr = m_ExternalUniforms.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Name.Compare(uniformName))
            return &(*itr);
    }

    return NULL;
}

const ShaderGraphCompiler::ExternalParameter *ShaderGraphCompiler::GetExternalTextureParameter(const char *textureName) const
{
    for (ExternalParameterList::ConstIterator itr = m_ExternalTextures.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Name.Compare(textureName))
            return &(*itr);
    }

    return NULL;
}

const bool ShaderGraphCompiler::GetExternalStaticSwitchParameter(const char *staticSwitchName) const
{
    for (StaticSwitchList::ConstIterator itr = m_ExternalStaticSwitches.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Left.Compare(staticSwitchName))
            return itr->Right;
    }

    return false;
}

void ShaderGraphCompiler::AddExternalUniformParameter(const char *uniformName, SHADER_PARAMETER_TYPE uniformType, const char *bindingName)
{
    for (ExternalParameterList::Iterator itr = m_ExternalUniforms.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Name.Compare(uniformName))
        {
            DebugAssert(itr->Type == uniformType);
            return;
        }
    }

    ExternalParameter ep;
    ep.Name = uniformName;
    ep.Type = uniformType;
    ep.BindingName = bindingName;
    m_ExternalUniforms.PushBack(ep);
}

void ShaderGraphCompiler::AddExternalTextureParameter(const char *textureName, TEXTURE_TYPE textureType, const char *bindingName)
{
    SHADER_PARAMETER_TYPE mappedType = SHADER_PARAMETER_TYPE_COUNT;
    switch (textureType)
    {
    case TEXTURE_TYPE_1D:                   mappedType = SHADER_PARAMETER_TYPE_TEXTURE1D;           break;
    case TEXTURE_TYPE_1D_ARRAY:             mappedType = SHADER_PARAMETER_TYPE_TEXTURE1DARRAY;      break;
    case TEXTURE_TYPE_2D:                   mappedType = SHADER_PARAMETER_TYPE_TEXTURE2D;           break;
    case TEXTURE_TYPE_2D_ARRAY:             mappedType = SHADER_PARAMETER_TYPE_TEXTURE2DARRAY;      break;
    case TEXTURE_TYPE_3D:                   mappedType = SHADER_PARAMETER_TYPE_TEXTURE3D;           break;
    case TEXTURE_TYPE_CUBE:                 mappedType = SHADER_PARAMETER_TYPE_TEXTURECUBE;         break;
    case TEXTURE_TYPE_CUBE_ARRAY:           mappedType = SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY;    break;
    default:                                UnreachableCode();                                      break;
    }

    for (ExternalParameterList::Iterator itr = m_ExternalTextures.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Name.Compare(textureName))
        {
            DebugAssert(itr->Type == mappedType);
            return;
        }
    }

    ExternalParameter ep;
    ep.Name = textureName;
    ep.Type = mappedType;
    ep.BindingName = bindingName;
    m_ExternalTextures.PushBack(ep);
}

void ShaderGraphCompiler::AddExternalStaticSwitchParameter(const char *staticSwitchName, bool enabled)
{
    for (StaticSwitchList::Iterator itr = m_ExternalStaticSwitches.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Left.Compare(staticSwitchName))
        {
            itr->Right = enabled;
            return;
        }
    }

    m_ExternalStaticSwitches.PushBack(Pair<String, bool>(staticSwitchName, enabled));
}

