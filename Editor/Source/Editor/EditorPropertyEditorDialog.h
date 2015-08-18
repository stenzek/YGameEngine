#pragma once
#include "Editor/Common.h"
#include "Editor/EditorPropertyEditorWidget.h"

class EditorMap;
class EditorMapEntity;
class PropertyTemplate;

class EditorPropertyEditorDialog : public QDialog
{
    Q_OBJECT

public:
    EditorPropertyEditorDialog(QWidget *pParent);
    virtual ~EditorPropertyEditorDialog();

    const EditorPropertyEditorWidget *GetPropertyEditor() const { return m_pPropertyEditor; }
    EditorPropertyEditorWidget *GetPropertyEditor() { return m_pPropertyEditor; }

    const EditorMap *GetAttachedMap() const { return m_pAttachedMap; }
    const EditorMapEntity *GetAttachedEntity() const { return m_pAttachedEntity; }

    void PopulateForEntity(EditorMap *pMap, EditorMapEntity *pEntity);
    void PopulateForTemplate(const PropertyTemplate *pTemplate);
    void PopulateForTable(const PropertyTemplate *pTemplate, PropertyTable *pTable);

Q_SIGNALS:
    void OnPropertyChanged(const char *propertyName, const char *propertyValue);

private:
    EditorPropertyEditorWidget *m_pPropertyEditor;

    EditorMap *m_pAttachedMap;
    EditorMapEntity *m_pAttachedEntity;

    const PropertyTemplate *m_pAttachedTemplate;
    PropertyTable *m_pAttachedTable;
};

