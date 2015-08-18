#pragma once
#include "Editor/Common.h"

class RenderWorld;
class MapSourceEntityData;
class MapSourceEntityComponent;
class ObjectTemplate;
class EditorMap;
class EditorVisualDefinition;
class EditorVisualInstance;

class EditorMapEntity
{
public:
    ~EditorMapEntity();

    const uint32 GetArrayIndex() const { return m_arrayIndex; }
    const ObjectTemplate *GetTemplate() const { return m_pTemplate; }
    const EditorVisualDefinition *GetVisualDefinition() const { return m_pVisualDefinition; }
    const MapSourceEntityData *GetEntityData() const { return m_pEntityData; }
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }
    const float3 &GetPosition() const { return m_position; }
    const Quaternion &GetRotation() const { return m_rotation; }
    const float3 &GetScale() const { return m_scale; }
    const bool IsSelected() const { return m_selected; }

    // helpers
    const bool IsBrush() const;
    const bool IsEntity() const;

    // creation
    static EditorMapEntity *Create(EditorMap *pMap, uint32 arrayIndex, MapSourceEntityData *pEntityData);

    // visual creation/deletion
    const EditorVisualInstance *GetVisualInstance() const { return m_pVisualInstance; }
    EditorVisualInstance *GetVisualInstance() { return m_pVisualInstance; }
    const bool IsVisualCreated() const { return m_visualsCreated; }
    void CreateVisual();
    void DeleteVisual();

    // property access
    String GetEntityPropertyValue(const char *propertyName) const;
    bool SetEntityPropertyValue(const char *propertyName, const char *propertyValue);

    // transform access
    void SetPosition(const float3 &position);
    void SetRotation(const Quaternion &rotation);
    void SetScale(const float3 &scale);

    // events
    void OnPropertyModified(const char *propertyName, const char *propertyValue);
    void OnSelectedStateChanged(bool selected);
    void OnComponentSelectedStateChanged(uint32 index, bool selected);

    // components
    const ObjectTemplate *GetComponentTemplate(uint32 index) const;
    const MapSourceEntityComponent *GetComponentData(uint32 index) const;
    const uint32 GetComponentCount() const { return m_components.GetSize(); }
    String GetComponentPropertyValue(uint32 index, const char *propertyName) const;
    void SetComponentPropertyValue(uint32 index, const char *propertyName, const char *propertyValue);
    int32 CreateComponent(const char *componentTypeName, const char *componentName = nullptr);
    int32 CreateComponent(const ObjectTemplate *pTemplate, const char *componentName = nullptr);
    void RemoveComponent(uint32 index);

protected:
    struct ComponentData
    {
        const ObjectTemplate *pTemplate;
        const EditorVisualDefinition *pVisualDefinition;
        MapSourceEntityComponent *pData;
        EditorVisualInstance *pVisual;
    };

    EditorMapEntity(EditorMap *pMap, uint32 arrayIndex, const ObjectTemplate *pTemplate, const EditorVisualDefinition *pVisualDefinition, MapSourceEntityData *pEntityData);
    void InternalCreateComponentVisual(ComponentData *data);
    uint32 InternalCreateComponent(const ObjectTemplate *pTemplate, MapSourceEntityComponent *pComponentData);

    EditorMap *m_pMap;
    uint32 m_arrayIndex;
    const ObjectTemplate *m_pTemplate;
    const EditorVisualDefinition *m_pVisualDefinition;
    MapSourceEntityData *m_pEntityData;

    EditorVisualInstance *m_pVisualInstance;
    bool m_visualsCreated;

    // bounding box of all visuals
    AABox m_boundingBox;
    Sphere m_boundingSphere;

    // we track position, rotation, scale for the purpose of the widget
    float3 m_position;
    Quaternion m_rotation;
    float3 m_scale;
    bool m_selected;

    // component data
    MemArray<ComponentData> m_components;
};

