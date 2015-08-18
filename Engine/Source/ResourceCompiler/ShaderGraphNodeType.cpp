#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphNodeType.h"

ShaderGraphNodeTypeInfo::ShaderGraphNodeTypeInfo(const char *Name, const ObjectTypeInfo *pParentType,
                                                 const PROPERTY_DECLARATION *pPropertyDeclarations,
                                                 const SHADER_GRAPH_NODE_INPUT *pInputs, const SHADER_GRAPH_NODE_OUTPUT *pOutputs,
                                                 const char *ShortName, const char *Description, ObjectFactory *pFactory)
    : ObjectTypeInfo(Name, pParentType, pPropertyDeclarations, pFactory),
      m_szShortName(ShortName),
      m_szDescription(Description),
      m_nInputs(0),
      m_pInputs(pInputs),
      m_nOutputs(0),
      m_pOutputs(pOutputs)
{
    if (m_pInputs != NULL)
    {
        for (; m_pInputs[m_nInputs].Name != NULL; m_nInputs++);
    }
    if (m_pOutputs != NULL)
    {
        for (; m_pOutputs[m_nOutputs].Name != NULL; m_nOutputs++);
    }
}

ShaderGraphNodeTypeInfo::~ShaderGraphNodeTypeInfo()
{
    //DebugAssert(m_iTypeIndex == INVALID_TYPE_INDEX);
}
