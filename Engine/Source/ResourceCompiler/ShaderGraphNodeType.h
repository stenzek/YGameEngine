#pragma once
#include "ResourceCompiler/Common.h"
#include "Renderer/RendererTypes.h"

class ShaderGraphNode;

typedef uint32 SHADER_GRAPH_VALUE_SWIZZLE;

// common types
#define SHADER_GRAPH_VALUE_SWIZZLE_NONE (((uint32)255 << 24) | ((uint32)255 << 16) | ((uint32)255 << 8) | ((uint32)255))
#define SHADER_GRAPH_VALUE_SWIZZLE_RGBA (((uint32)0 << 24) | ((uint32)1 << 16) | ((uint32)2 << 8) | ((uint32)3))
#define SHADER_GRAPH_VALUE_SWIZZLE_RGB (((uint32)0 << 24) | ((uint32)1 << 16) | ((uint32)2 << 8) | ((uint32)255))
#define SHADER_GRAPH_VALUE_SWIZZLE_RG (((uint32)0 << 24) | ((uint32)1 << 16) | ((uint32)255 << 8) | ((uint32)255))
#define SHADER_GRAPH_VALUE_SWIZZLE_R (((uint32)0 << 24) | ((uint32)255 << 16) | ((uint32)255 << 8) | ((uint32)255))

struct SHADER_GRAPH_NODE_INPUT
{
    const char *Name;
    SHADER_PARAMETER_TYPE Type;
    const char *DefaultValue;
    bool Optional;
};

struct SHADER_GRAPH_NODE_OUTPUT
{
    const char *Name;
    SHADER_PARAMETER_TYPE Type; 
};

class ShaderGraphNodeTypeInfo : public ObjectTypeInfo
{
public:
    ShaderGraphNodeTypeInfo(const char *Name, const ObjectTypeInfo *pParentType,
                            const PROPERTY_DECLARATION *pPropertyDeclarations,
                            const SHADER_GRAPH_NODE_INPUT *pInputs, const SHADER_GRAPH_NODE_OUTPUT *pOutputs,
                            const char *ShortName, const char *Description, ObjectFactory *pFactory);

    ~ShaderGraphNodeTypeInfo();

    const char *GetShortName() const { return m_szShortName; }
    const char *GetDescription() const { return m_szDescription; }
    
    uint32 GetInputCount() const { return m_nInputs; }
    const SHADER_GRAPH_NODE_INPUT *GetInput(uint32 i) const { DebugAssert(i < m_nInputs); return &m_pInputs[i]; }
    uint32 GetOutputCount() const { return m_nOutputs; }
    const SHADER_GRAPH_NODE_OUTPUT *GetOutput(uint32 i) const { DebugAssert(i < m_nOutputs); return &m_pOutputs[i]; }

protected:
    const char *m_szShortName;
    const char *m_szDescription;

    uint32 m_nInputs;
    const SHADER_GRAPH_NODE_INPUT *m_pInputs;
    uint32 m_nOutputs;
    const SHADER_GRAPH_NODE_OUTPUT *m_pOutputs;
};

#define DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(Type, ParentType) \
        private: \
            static ShaderGraphNodeTypeInfo s_typeInfo; \
            static const PROPERTY_DECLARATION s_propertyDeclarations[]; \
            static const SHADER_GRAPH_NODE_INPUT s_pInputs[]; \
            static const SHADER_GRAPH_NODE_OUTPUT s_pOutputs[]; \
        public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const ShaderGraphNodeTypeInfo *StaticTypeInfo() { return &s_typeInfo; } \
            static ShaderGraphNodeTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }

#define DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(Type) DECLARE_OBJECT_GENERIC_FACTORY(Type)

#define DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(Type) DECLARE_OBJECT_NO_FACTORY(Type)

#define DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(Type, ParentType, ShortName, Description) \
        ShaderGraphNodeTypeInfo Type::s_typeInfo = ShaderGraphNodeTypeInfo(#Type, ParentType::StaticTypeInfo(), Type::s_propertyDeclarations, s_pInputs, s_pOutputs, ShortName, Description, Type::StaticFactory());

#define DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(Type) DEFINE_OBJECT_GENERIC_FACTORY(Type)

#define BEGIN_SHADER_GRAPH_NODE_PROPERTIES(Type) \
        const PROPERTY_DECLARATION Type::s_propertyDeclarations[] = {

#define END_SHADER_GRAPH_NODE_PROPERTIES() \
            PROPERTY_TABLE_MEMBER(NULL, PROPERTY_TYPE_COUNT, 0, NULL, NULL, NULL, NULL, NULL, NULL) \
        };

#define DEFINE_SHADER_GRAPH_NODE_INPUTS(Type) \
        const SHADER_GRAPH_NODE_INPUT Type::s_pInputs[] = {

#define DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY(Name, Type, DefaultValue, Optional) \
            { Name, Type, DefaultValue, Optional },

#define END_SHADER_GRAPH_NODE_INPUTS() \
            { NULL, SHADER_PARAMETER_TYPE_COUNT, NULL, true } \
        };

#define DEFINE_SHADER_GRAPH_NODE_OUTPUTS(Type) \
        const SHADER_GRAPH_NODE_OUTPUT Type::s_pOutputs[] = {

#define DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY(Name, Type) \
            { Name, Type },

#define END_SHADER_GRAPH_NODE_OUTPUTS() \
            { NULL, SHADER_PARAMETER_TYPE_COUNT } \
        };

#define SHADER_GRAPH_NODE_TYPEINFO(Type) Type::StaticTypeInfo()
#define SHADER_GRAPH_NODE_TYPEINFO_PTR(Ptr) Ptr->StaticTypeInfo()

#define SHADER_GRAPH_NODE_MUTABLE_TYPEINFO(Type) Type::StaticMutableTypeInfo()
#define SHADER_GRAPH_NODE_MUTABLE_TYPEINFO_PTR(Type) Type->StaticMutableTypeInfo()

