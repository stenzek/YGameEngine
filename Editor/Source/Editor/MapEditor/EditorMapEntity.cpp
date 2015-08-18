#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "Editor/EditorVisual.h"
#include "Editor/EditorVisualComponent.h"
#include "Editor/Editor.h"
#include "MapCompiler/MapSource.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "Engine/Entity.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(EditorMapEntity);

EditorMapEntity::EditorMapEntity(EditorMap *pMap, uint32 arrayIndex, const ObjectTemplate *pTemplate, const EditorVisualDefinition *pVisualDefinition, MapSourceEntityData *pEntityData)
    : m_pMap(pMap),
      m_arrayIndex(arrayIndex),
      m_pTemplate(pTemplate),
      m_pVisualDefinition(pVisualDefinition),
      m_pEntityData(pEntityData),
      m_pVisualInstance(nullptr),
      m_visualsCreated(false),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_position(float3::Zero),
      m_rotation(Quaternion::Identity),
      m_scale(float3::One),
      m_selected(false)
{
    
}

EditorMapEntity::~EditorMapEntity()
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        delete m_components[i].pVisual;

    delete m_pVisualInstance;
}

const bool EditorMapEntity::IsBrush() const
{
    const ObjectTemplate *pBaseTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("StaticObject");
    DebugAssert(pBaseTemplate != nullptr);

    return m_pTemplate->IsDerivedFrom(pBaseTemplate);
}

const bool EditorMapEntity::IsEntity() const
{
    const ObjectTemplate *pBaseTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("Entity");
    DebugAssert(pBaseTemplate != nullptr);

    return m_pTemplate->IsDerivedFrom(pBaseTemplate);
}

void EditorMapEntity::CreateVisual()
{
    if (m_visualsCreated)
        return;

    m_pVisualInstance = EditorVisualInstance::Create(m_arrayIndex + 1, m_pVisualDefinition, m_pEntityData->GetPropertyTable());
    if (m_pVisualInstance != nullptr)
    {
        m_pVisualInstance->AddToRenderWorld(m_pMap->GetRenderWorld());
        
        // fix up selected state
        m_pVisualInstance->OnSelectedStateChanged(m_selected);

        // fix up bounding box
        if (m_boundingBox != m_pVisualInstance->GetBoundingBox())
        {
            m_boundingBox = m_pVisualInstance->GetBoundingBox();
            m_pMap->OnEntityBoundsChanged(this);
        }
        m_boundingSphere = m_pVisualInstance->GetBoundingSphere();
    }

    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        ComponentData *data = &m_components[i];
        DebugAssert(data->pVisual == nullptr);
        InternalCreateComponentVisual(data);
    }

    m_visualsCreated = true;
}

void EditorMapEntity::DeleteVisual()
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        delete m_components[i].pVisual;

    delete m_pVisualInstance;
    m_pVisualInstance = nullptr;
    m_visualsCreated = false;
}

EditorMapEntity *EditorMapEntity::Create(EditorMap *pMap, uint32 arrayIndex, MapSourceEntityData *pEntityData)
{
    // find object template
    const ObjectTemplate *pTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pEntityData->GetTypeName());
    if (pTemplate == nullptr)
    {
        Log_WarningPrintf("No object template found for type '%s' referenced by entity '%s'. No visual will be shown, and the entity will not be editable.", pEntityData->GetTypeName().GetCharArray(), pEntityData->GetEntityName().GetCharArray());
        return nullptr;
    }

    // find definition for this entity type
    const EditorVisualDefinition *pVisualDefinition = g_pEditor->GetVisualDefinitionForObjectTemplate(pTemplate);
    if (pVisualDefinition == nullptr)
    {
        Log_WarningPrintf("No visual definition found for type '%s' referenced by entity '%s'. No visual will be shown.", pEntityData->GetTypeName().GetCharArray(), pEntityData->GetEntityName().GetCharArray());
        return nullptr;
    }

    // create object
    EditorMapEntity *pMapEntity = new EditorMapEntity(pMap, arrayIndex, pTemplate, pVisualDefinition, pEntityData);

    // import transform
    {
        const String *propertyValue;

        // position
        propertyValue = pEntityData->GetPropertyValuePointer("Position");
        if (propertyValue != nullptr)
            pMapEntity->m_position = StringConverter::StringToFloat3(*propertyValue);

        // rotation
        propertyValue = pEntityData->GetPropertyValuePointer("Rotation");
        if (propertyValue != nullptr)
            pMapEntity->m_rotation = StringConverter::StringToQuaternion(*propertyValue);

        // scale
        propertyValue = pEntityData->GetPropertyValuePointer("Scale");
        if (propertyValue != nullptr)
            pMapEntity->m_scale = StringConverter::StringToFloat3(*propertyValue);
    }

    // set inital bounding box
    pMapEntity->m_boundingBox.SetBounds(pMapEntity->m_position, pMapEntity->m_position);
    pMapEntity->m_boundingSphere.SetCenter(pMapEntity->m_position);

    // create components
    for (uint32 componentIndex = 0; componentIndex < pEntityData->GetComponentCount(); componentIndex++)
    {
        MapSourceEntityComponent *pComponentData = pEntityData->GetComponentByIndex(componentIndex);

        // get template
        const ObjectTemplate *pComponentTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pComponentData->GetTypeName());
        if (pComponentTemplate == nullptr)
        {
            Log_WarningPrintf("No object template found for component type '%s' referenced by entity '%s'. No visual will be shown, and the component will not be editable.", pComponentData->GetTypeName().GetCharArray(), pEntityData->GetEntityName().GetCharArray());
            continue;
        }

        // create internal copy
        pMapEntity->InternalCreateComponent(pComponentTemplate, pComponentData);
    }

    // all done
    return pMapEntity;
}

void EditorMapEntity::OnPropertyModified(const char *propertyName, const char *propertyValue)
{
    // pass to visual
    if (m_pVisualInstance != nullptr)
    {
        m_pVisualInstance->OnPropertyModified(propertyName, propertyValue);
        if (m_boundingBox != m_pVisualInstance->GetBoundingBox())
        {
            m_boundingBox = m_pVisualInstance->GetBoundingBox();
            m_pMap->OnEntityBoundsChanged(this);
        }

        m_boundingSphere = m_pVisualInstance->GetBoundingSphere();
    }

    // handle base position + rotation
    if (Y_stricmp(propertyName, "Position") == 0)
    {
        m_position = StringConverter::StringToFloat3(propertyValue);

        // handle psuedo-properties for components
        for (uint32 i = 0; i < m_components.GetSize(); i++)
        {
            ComponentData *data = &m_components[i];
            if (data->pVisual != nullptr)
                data->pVisual->OnPropertyModified("Position", StringConverter::Float3ToString(m_position + data->pData->GetPropertyTable()->GetPropertyValueDefaultFloat3("LocalPosition", float3::Zero)));
        }
    }
    else if (Y_stricmp(propertyName, "Rotation") == 0)
    {
        m_rotation = StringConverter::StringToQuaternion(propertyValue);

        // handle psuedo-properties for components
        for (uint32 i = 0; i < m_components.GetSize(); i++)
        {
            ComponentData *data = &m_components[i];
            if (data->pVisual != nullptr)
                data->pVisual->OnPropertyModified("Rotation", StringConverter::QuaternionToString((m_rotation * data->pData->GetPropertyTable()->GetPropertyValueDefaultQuaternion("LocalRotation", Quaternion::Identity)).Normalize()));
        }
    }
    else if (Y_stricmp(propertyName, "Scale") == 0)
    {
        m_scale = StringConverter::StringToFloat3(propertyValue);

        // handle psuedo-properties for components
        for (uint32 i = 0; i < m_components.GetSize(); i++)
        {
            ComponentData *data = &m_components[i];
            if (data->pVisual != nullptr)
                data->pVisual->OnPropertyModified("Scale", StringConverter::Float3ToString(m_scale * data->pData->GetPropertyTable()->GetPropertyValueDefaultFloat3("LocalScale", float3::One)));
        }
    }

    // notify map
    m_pMap->OnEntityPropertyChanged(this, propertyName, propertyValue);
}

void EditorMapEntity::SetPosition(const float3 &position)
{
    SetEntityPropertyValue("Position", StringConverter::Float3ToString(position));
}

void EditorMapEntity::SetRotation(const Quaternion &rotation)
{
    SetEntityPropertyValue("Rotation", StringConverter::QuaternionToString(rotation));
}

void EditorMapEntity::SetScale(const float3 &scale)
{
    SetEntityPropertyValue("Scale", StringConverter::Float3ToString(scale));
}

void EditorMapEntity::OnSelectedStateChanged(bool selected)
{
    if (m_visualsCreated)
    {
        if (m_pVisualInstance != nullptr)
            m_pVisualInstance->OnSelectedStateChanged(selected);

        for (uint32 i = 0; i < m_components.GetSize(); i++)
        {
            ComponentData *data = &m_components[i];
            if (data->pVisual != nullptr)
                data->pVisual->OnSelectedStateChanged(selected);
        }
    }

    m_selected = selected;
}

void EditorMapEntity::OnComponentSelectedStateChanged(uint32 index, bool selected)
{
    DebugAssert(index < m_components.GetSize());
    
    ComponentData *data = &m_components[index];
    if (data->pVisual != nullptr)
        data->pVisual->OnSelectedStateChanged(selected);
}

String EditorMapEntity::GetEntityPropertyValue(const char *propertyName) const
{
    return m_pEntityData->GetPropertyValueString(propertyName);
}

bool EditorMapEntity::SetEntityPropertyValue(const char *propertyName, const char *propertyValue)
{
    if (m_pTemplate->GetPropertyDefinitionByName(propertyName) == nullptr)
    {
        Log_WarningPrintf("Attempting to set non-existant property '%s' on entity '%s' to '%s'.", propertyName, m_pEntityData->GetEntityName().GetCharArray(), propertyValue);
        return false;
    }

    String oldValue = m_pEntityData->GetPropertyValueString(propertyName);
    if (oldValue == propertyValue)
        return true;

    m_pEntityData->SetPropertyValue(propertyName, propertyValue);

    // notify the visuals
    OnPropertyModified(propertyName, propertyValue);
    return true;
}

const ObjectTemplate *EditorMapEntity::GetComponentTemplate(uint32 index) const
{
    return m_components[index].pTemplate;
}

const MapSourceEntityComponent *EditorMapEntity::GetComponentData(uint32 index) const
{
    return m_components[index].pData;
}

String EditorMapEntity::GetComponentPropertyValue(uint32 index, const char *propertyName) const
{
    return m_components[index].pData->GetPropertyValueString(propertyName);
}

void EditorMapEntity::SetComponentPropertyValue(uint32 index, const char *propertyName, const char *propertyValue)
{
    ComponentData *data = &m_components[index];

    data->pData->SetPropertyValue(propertyName, propertyValue);

    // handle positional
    if (data->pVisual != nullptr)
    {
        data->pVisual->OnPropertyModified(propertyName, propertyValue);

        // handle psuedo-properties
        if (Y_stricmp(propertyName, "LocalPosition") == 0)
            data->pVisual->OnPropertyModified("Position", StringConverter::Float3ToString(m_position + StringConverter::StringToFloat3(propertyValue)));
        else if (Y_stricmp(propertyName, "LocalRotation") == 0)
            data->pVisual->OnPropertyModified("Rotation", StringConverter::QuaternionToString((m_rotation * StringConverter::StringToQuaternion(propertyValue)).Normalize()));
        else if (Y_stricmp(propertyName, "LocalScale") == 0)
            data->pVisual->OnPropertyModified("Scale", StringConverter::Float3ToString(m_scale * StringConverter::StringToFloat3(propertyValue)));
    }
}

int32 EditorMapEntity::CreateComponent(const char *componentTypeName, const char *componentName /*= nullptr*/)
{
    const ObjectTemplate *pTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(componentTypeName);
    if (pTemplate == nullptr)
        return -1;

    return CreateComponent(pTemplate, componentName);
}

int32 EditorMapEntity::CreateComponent(const ObjectTemplate *pTemplate, const char *componentName /*= nullptr*/)
{
    // forward through
    MapSourceEntityComponent *pComponentData = m_pEntityData->CreateComponent(pTemplate, componentName);
    if (pComponentData == nullptr)
        return -1;

    return (int32)InternalCreateComponent(pTemplate, pComponentData);
}

uint32 EditorMapEntity::InternalCreateComponent(const ObjectTemplate *pTemplate, MapSourceEntityComponent *pComponentData)
{
    // find definition for this entity type
    const EditorVisualDefinition *pVisualDefinition = g_pEditor->GetVisualDefinitionForObjectTemplate(pTemplate);
    if (pVisualDefinition == nullptr)
        Log_WarningPrintf("No visual definition found for component type '%s' referenced by entity '%s'. No visual will be shown.", pComponentData->GetTypeName().GetCharArray(), m_pEntityData->GetEntityName().GetCharArray());

    // create our structure
    ComponentData entry;
    entry.pTemplate = pTemplate;
    entry.pVisualDefinition = pVisualDefinition;
    entry.pData = pComponentData;
    entry.pVisual = nullptr;

    // create visual if the entity's visuals are created
    if (m_visualsCreated)
        InternalCreateComponentVisual(&entry);

    // add to list
    uint32 returnValue = m_components.GetSize();
    m_components.Add(entry);
    return returnValue;
}

void EditorMapEntity::InternalCreateComponentVisual(ComponentData *data)
{
    if (data->pVisualDefinition == nullptr)
        return;

    data->pVisual = EditorVisualInstance::Create(m_arrayIndex + 1, data->pVisualDefinition, data->pData->GetPropertyTable());
    if (data->pVisual != nullptr)
    {
        // create psuedo-properties
        data->pVisual->OnPropertyModified("Position", StringConverter::Float3ToString(m_position + data->pData->GetPropertyTable()->GetPropertyValueDefaultFloat3("LocalPosition", float3::Zero)));
        data->pVisual->OnPropertyModified("Rotation", StringConverter::QuaternionToString((m_rotation * data->pData->GetPropertyTable()->GetPropertyValueDefaultQuaternion("LocalRotation", Quaternion::Identity)).Normalize()));
        data->pVisual->OnPropertyModified("Scale", StringConverter::Float3ToString(m_scale * data->pData->GetPropertyTable()->GetPropertyValueDefaultFloat3("LocalScale", float3::One)));

        // add to world
        data->pVisual->AddToRenderWorld(m_pMap->GetRenderWorld());

        // fix up selected state
        data->pVisual->OnSelectedStateChanged(m_selected);

        // fix up bounding box
        AABox newBoundingBox(AABox::Merge(m_boundingBox, data->pVisual->GetBoundingBox()));
        if (newBoundingBox != m_boundingBox)
        {
            m_boundingBox = newBoundingBox;
            m_pMap->OnEntityBoundsChanged(this);
        }

        // update bounding sphere
        m_boundingSphere = Sphere::Merge(m_boundingSphere, data->pVisual->GetBoundingSphere());
    }
}

void EditorMapEntity::RemoveComponent(uint32 index)
{
    DebugAssert(index < m_components.GetSize());

    ComponentData *data = &m_components[index];

    m_pEntityData->RemoveComponent(data->pData);
    delete data->pVisual;

    m_components.OrderedRemove(index);
}
