#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorPropertyEditorDialog.h"
#include "Editor/Editor.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "Editor/EditorHelpers.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "MapCompiler/MapSource.h"

EditorPropertyEditorDialog::EditorPropertyEditorDialog(QWidget *pParent)
    : QDialog(pParent, Qt::WindowStaysOnTopHint | Qt::Tool),
      m_pPropertyEditor(new EditorPropertyEditorWidget(this)),
      m_pAttachedMap(nullptr),
      m_pAttachedEntity(nullptr),
      m_pAttachedTemplate(nullptr),
      m_pAttachedTable(nullptr)
{
    m_pPropertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pPropertyEditor->setMinimumSize(300, 500);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(2, 2, 2, 2);
    verticalLayout->addWidget(m_pPropertyEditor, 1);
    setLayout(verticalLayout);
}

EditorPropertyEditorDialog::~EditorPropertyEditorDialog()
{

}

void EditorPropertyEditorDialog::PopulateForEntity(EditorMap *pMap, EditorMapEntity *pEntity)
{
    m_pAttachedMap = pMap;
    m_pAttachedEntity = pEntity;

    // build the title text
    LargeString titleText;
    titleText.AppendFormattedString("<strong>%s</strong>&nbsp;(%s)<br>", pEntity->GetEntityData()->GetEntityName().GetCharArray(), pEntity->GetTemplate()->GetTypeName().GetCharArray());
    titleText.AppendString(pEntity->GetTemplate()->GetDescription());
    //m_pPropertyEditor->SetTitleText(titleText);

    // add properties and values
    const ObjectTemplate *pTemplate = pEntity->GetTemplate();
    for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
    {
        const PropertyTemplateProperty *pProperty = pTemplate->GetPropertyDefinition(i);
        const String *propertyValue = pEntity->GetEntityData()->GetPropertyValuePointer(pProperty->GetName());
        if (propertyValue == nullptr)
            propertyValue = &pProperty->GetDefaultValue();

        m_pPropertyEditor->AddProperty(pProperty, *propertyValue);
    }
}

void EditorPropertyEditorDialog::PopulateForTemplate(const PropertyTemplate *pTemplate)
{

}

void EditorPropertyEditorDialog::PopulateForTable(const PropertyTemplate *pTemplate, PropertyTable *pTable)
{
    for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
    {
        const PropertyTemplateProperty *pProperty = pTemplate->GetPropertyDefinition(i);
        m_pPropertyEditor->AddProperty(pProperty, pTable->GetPropertyValueDefaultString(pProperty->GetName(), pProperty->GetDefaultValue()));
    }

    m_pAttachedTemplate = pTemplate;
    m_pAttachedTable = pTable;
}

