#include "Core/PrecompiledHeader.h"
#include "Core/ObjectTypeInfo.h"

ObjectTypeInfo::RegistryType &ObjectTypeInfo::GetRegistry()
{
    static RegistryType s_Registry;
    return s_Registry;
}

ObjectTypeInfo::ObjectTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory)
    : m_iTypeIndex(INVALID_OBJECT_TYPE_INDEX),
      m_iInheritanceDepth(0),
      m_strTypeName(TypeName),
      m_pParentType(pParentTypeInfo),
      m_pFactory(pFactory),
      m_pSourcePropertyDeclarations(pPropertyDeclarations),
      m_ppPropertyDeclarations(NULL),
      m_nPropertyDeclarations(0)
{

}

ObjectTypeInfo::~ObjectTypeInfo()
{
    //DebugAssert(m_iTypeIndex == INVALID_TYPE_INDEX);
}

bool ObjectTypeInfo::IsDerived(const ObjectTypeInfo *pTypeInfo) const
{
    const ObjectTypeInfo *pCur = this;
    do
    {
        if (pCur == pTypeInfo)
            return true;

        pCur = pCur->m_pParentType;
    }
    while (pCur != NULL);

    return false;
}

const PROPERTY_DECLARATION *ObjectTypeInfo::GetPropertyDeclarationByName(const char *PropertyName) const
{
    for (uint32 i = 0; i < m_nPropertyDeclarations; i++)
    {
        if (!Y_stricmp(m_ppPropertyDeclarations[i]->Name, PropertyName))
            return m_ppPropertyDeclarations[i];
    }

    return nullptr;
}

void ObjectTypeInfo::RegisterType()
{
    if (m_iTypeIndex != INVALID_OBJECT_TYPE_INDEX)
        return;

    // our stuff
    const ObjectTypeInfo *pCurrentTypeInfo;
    const PROPERTY_DECLARATION *pPropertyDeclaration;
    uint32 i;

    // get property count
    pCurrentTypeInfo = this;
    m_nPropertyDeclarations = 0;
    m_iInheritanceDepth = 0;
    while (pCurrentTypeInfo != nullptr)
    {
        if (pCurrentTypeInfo->m_pSourcePropertyDeclarations != nullptr)
        {
            pPropertyDeclaration = pCurrentTypeInfo->m_pSourcePropertyDeclarations;
            while (pPropertyDeclaration->Name != nullptr)
            {
                m_nPropertyDeclarations++;
                pPropertyDeclaration++;
            }
        }

        pCurrentTypeInfo = pCurrentTypeInfo->GetParentType();
        m_iInheritanceDepth++;
    }

    if (m_nPropertyDeclarations > 0)
    {
        m_ppPropertyDeclarations = new const PROPERTY_DECLARATION *[m_nPropertyDeclarations];
        pCurrentTypeInfo = this;
        i = 0;
        while (pCurrentTypeInfo != nullptr)
        {
            if (pCurrentTypeInfo->m_pSourcePropertyDeclarations != nullptr)
            {
                pPropertyDeclaration = pCurrentTypeInfo->m_pSourcePropertyDeclarations;
                while (pPropertyDeclaration->Name != nullptr)
                {
                    DebugAssert(i < m_nPropertyDeclarations);
                    m_ppPropertyDeclarations[i++] = pPropertyDeclaration++;
                }
            }

            pCurrentTypeInfo = pCurrentTypeInfo->GetParentType();
        }
    }

    m_iTypeIndex = GetRegistry().RegisterTypeInfo(this, m_strTypeName, m_iInheritanceDepth);
}

void ObjectTypeInfo::UnregisterType()
{
    if (m_iTypeIndex == INVALID_OBJECT_TYPE_INDEX)
        return;

    delete[] m_ppPropertyDeclarations;
    m_ppPropertyDeclarations = nullptr;
    m_nPropertyDeclarations = 0;

    m_iTypeIndex = INVALID_OBJECT_TYPE_INDEX;
    GetRegistry().UnregisterTypeInfo(this);
}

