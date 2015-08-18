#pragma once
#include "Editor/Common.h"
#include "Core/PropertyTable.h"
#include "Core/PropertyTemplate.h"

class RenderWorld;
class ObjectTemplate;
class EditorVisualComponentDefinition;
class EditorVisualComponentInstance;
class ByteStream;

class EditorVisualDefinition
{
public:
    EditorVisualDefinition();
    ~EditorVisualDefinition();

    const String &GetName() const { return m_strName; }

    bool CreateFromName(const char *name);
    bool CreateFromTemplate(const ObjectTemplate *pTemplate);

    const EditorVisualComponentDefinition *GetVisualDefinition(uint32 i) const { DebugAssert(i < m_visualComponentDefinitions.GetSize()); return m_visualComponentDefinitions[i]; }
    const uint32 GetVisualDefinitionCount() const { return m_visualComponentDefinitions.GetSize(); }

private:
    bool LoadComponentsFromStream(const char *FileName, ByteStream *pStream);

    String m_strName;

    typedef PODArray<EditorVisualComponentDefinition *> VisualDefinitionArray;
    VisualDefinitionArray m_visualComponentDefinitions;
};

class EditorVisualInstance
{
public:
    ~EditorVisualInstance();

    static EditorVisualInstance *Create(uint32 pickingID, const EditorVisualDefinition *pDefinition, const PropertyTable *pInitialProperties = &PropertyTable::EmptyPropertyList);

    // accessors
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }
    RenderWorld *GetRenderWorld() const { return m_pRenderWorld; }

    // world manipulation
    void AddToRenderWorld(RenderWorld *pWorld);
    void RemoveFromRenderWorld(RenderWorld *pWorld);

    // events
    void OnPropertyModified(const char *propertyName, const char *propertyValue);
    void OnSelectedStateChanged(bool selected);

private:
    EditorVisualInstance(const EditorVisualDefinition *pDefinition);

    void UpdateBounds();

    const EditorVisualDefinition *m_pDefinition;
    RenderWorld *m_pRenderWorld;

    typedef PODArray<EditorVisualComponentInstance *> VisualComponentInstanceArray;
    VisualComponentInstanceArray m_visualInstances;

    // bounding box of all components
    AABox m_boundingBox;
    Sphere m_boundingSphere;
};

