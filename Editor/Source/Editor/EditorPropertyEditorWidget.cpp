#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorPropertyEditorWidget.h"
#include "Editor/EditorResourceSelectionWidget.h"
#include "Editor/EditorResourceSelectionDialog.h"
#include "Editor/EditorVectorEditWidget.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "Editor/MapEditor/EditorEntityEditMode.h"
#include "Editor/EditorHelpers.h"
#include "MapCompiler/MapSource.h"
Log_SetChannel(EditorPropertyEditorWidget);

EditorPropertyEditorWidget::EditorPropertyEditorWidget(QWidget *pParent)
    : QWidget(pParent),
      m_supressPropertyUpdates(true)
{
    CreateUI();
    ConnectEvents();
}

EditorPropertyEditorWidget::~EditorPropertyEditorWidget()
{
    ClearProperties();
}

void EditorPropertyEditorWidget::CreateUI()
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setMargin(0);

//     QGroupBox *titleFrame = new QGroupBox(this);
//     QHBoxLayout *titleLayout = new QHBoxLayout(titleFrame);
//     titleLayout->setContentsMargins(4, 0, 4, 0);
//     titleLayout->setAlignment(Qt::AlignTop);
// 
//     QLabel *titleIcon = new QLabel(titleFrame);
//     titleIcon->setPixmap(QPixmap(QStringLiteral(":/editor/icons/EditCodeHS.png")));
//     titleIcon->setFixedSize(32, 32);
//     titleIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//     titleLayout->addWidget(titleIcon);
// 
//     m_pTitleText = new QLabel(titleFrame);
//     m_pTitleText->setTextFormat(Qt::RichText);
//     //m_pTitleText->setText("<strong>hi</strong><br>Fooooo");
//     titleLayout->addWidget(m_pTitleText);
// 
//     titleFrame->setMinimumSize(1, 48);
//     titleFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//     titleFrame->setLayout(titleLayout);
//     verticalLayout->addWidget(titleFrame);

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);

    m_pTreePropertyBrowser = new QtTreePropertyBrowser(splitter);
    m_pTreePropertyBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pTreePropertyBrowser->setMinimumSize(1, 100);
    splitter->addWidget(m_pTreePropertyBrowser);

    m_pHelpText = new QTextEdit(splitter);
    m_pHelpText->setMinimumSize(1, 80);
    m_pHelpText->setAcceptRichText(true);
    m_pHelpText->setEnabled(false);
    m_pHelpText->setFrameShape(QFrame::Panel);
    m_pHelpText->setFrameShadow(QFrame::Sunken);
    splitter->addWidget(m_pHelpText);

    verticalLayout->addWidget(splitter, 1);
    setLayout(verticalLayout);

    m_pGroupPropertyManager = new QtGroupPropertyManager(m_pTreePropertyBrowser);
    m_pStringPropertyManager = new QtStringPropertyManager(m_pTreePropertyBrowser);
    m_pBoolPropertyManager = new QtBoolPropertyManager(m_pTreePropertyBrowser);
    m_pIntSpinBoxPropertyManager = new QtIntPropertyManager(m_pTreePropertyBrowser);
    m_pIntSliderPropertyManager = new QtIntPropertyManager(m_pTreePropertyBrowser);
    m_pDoubleSpinBoxPropertyManager = new QtDoublePropertyManager(m_pTreePropertyBrowser);
    m_pEnumPropertyManager = new QtEnumPropertyManager(m_pTreePropertyBrowser);
    m_pFlagPropertyManager = new QtFlagPropertyManager(m_pTreePropertyBrowser);
    m_pResourcePropertyManager = new EditorPropertyEditorResourcePropertyManager(m_pTreePropertyBrowser);
    m_pVectorPropertyManager = new EditorPropertyEditorVectorPropertyManager(m_pTreePropertyBrowser);

    // set default factories for the managers
    m_pTreePropertyBrowser->setFactoryForManager(m_pStringPropertyManager, new QtLineEditFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pBoolPropertyManager, new QtCheckBoxFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pIntSpinBoxPropertyManager, new QtSpinBoxFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pIntSliderPropertyManager, new QtSliderFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pDoubleSpinBoxPropertyManager, new QtDoubleSpinBoxFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pEnumPropertyManager, new QtEnumEditorFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pFlagPropertyManager->subBoolPropertyManager(), new QtCheckBoxFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pResourcePropertyManager, new EditorPropertyEditorResourceEditorFactory(m_pTreePropertyBrowser));
    m_pTreePropertyBrowser->setFactoryForManager(m_pVectorPropertyManager, new EditorPropertyEditorVectorEditFactory(m_pTreePropertyBrowser));
}

void EditorPropertyEditorWidget::ConnectEvents()
{
    connect(m_pTreePropertyBrowser, SIGNAL(currentItemChanged(QtBrowserItem *)), this, SLOT(OnTreeBrowserCurrentItemChanged(QtBrowserItem *)));

    connect(m_pGroupPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pStringPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pBoolPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pIntSpinBoxPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pIntSliderPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pDoubleSpinBoxPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pEnumPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pFlagPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pResourcePropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
    connect(m_pVectorPropertyManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(OnPropertyManagerPropertyChanged(QtProperty *)));
}

// void EditorPropertyEditorWidget::SetTitleText(const char *titleText)
// {
//     m_pTitleText->setText(titleText);
// }

bool EditorPropertyEditorWidget::AddProperty(const PropertyTemplateProperty *pPropertyDefinition, const char *currentValue)
{
    QString propertyName(ConvertStringToQString(pPropertyDefinition->GetLabel()));
    PROPERTY_TYPE propertyType = pPropertyDefinition->GetType();
    QtProperty *pProperty = NULL;
    QtAbstractPropertyManager *pPropertyManager = NULL;
    bool ownsPropertyManager = false;

    // suppress updates
    m_supressPropertyUpdates = true;

    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE selectorType = (pPropertyDefinition->GetSelector() != NULL) ? pPropertyDefinition->GetSelector()->GetType() : PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_COUNT;
    switch (selectorType)    
    {
    case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_RESOURCE:
        {
            const PropertyTemplateValueSelector_Resource *pResourceDefinition = static_cast<const PropertyTemplateValueSelector_Resource *>(pPropertyDefinition->GetSelector());
            const ObjectTypeInfo *pObjectTypeInfo = ResourceTypeInfo::GetRegistry().GetTypeInfoByName(pResourceDefinition->GetResourceTypeName());
            const ResourceTypeInfo *pResourceTypeInfo;
            if (pObjectTypeInfo != NULL && pObjectTypeInfo->IsDerived(OBJECT_TYPEINFO(Resource)))
                pResourceTypeInfo = static_cast<const ResourceTypeInfo *>(pObjectTypeInfo);
            else
                pResourceTypeInfo = NULL;

            pPropertyManager = m_pResourcePropertyManager;
            pProperty = m_pResourcePropertyManager->addProperty(propertyName);
            m_pResourcePropertyManager->setResourceType(pProperty, pResourceTypeInfo);
        }
        break;

    case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SPINNER:
        {
            const PropertyTemplateValueSelector_Spinner *pSpinnerDefinition = static_cast<const PropertyTemplateValueSelector_Spinner *>(pPropertyDefinition->GetSelector());
            switch (propertyType)
            {
            case PROPERTY_TYPE_INT:
            case PROPERTY_TYPE_UINT:
                {
                    pPropertyManager = m_pIntSpinBoxPropertyManager;
                    pProperty = m_pIntSpinBoxPropertyManager->addProperty(propertyName);
                    m_pIntSpinBoxPropertyManager->setRange(pProperty, (int)pSpinnerDefinition->GetMinValue(), (int)pSpinnerDefinition->GetMaxValue());
                    m_pIntSpinBoxPropertyManager->setSingleStep(pProperty, (int)pSpinnerDefinition->GetIncrement());
                }
                break;
                        
            case PROPERTY_TYPE_FLOAT:
                {
                    pPropertyManager = m_pDoubleSpinBoxPropertyManager;
                    pProperty = m_pDoubleSpinBoxPropertyManager->addProperty(propertyName);
                    m_pDoubleSpinBoxPropertyManager->setRange(pProperty, (double)pSpinnerDefinition->GetMinValue(), (double)pSpinnerDefinition->GetMaxValue());
                    m_pDoubleSpinBoxPropertyManager->setSingleStep(pProperty, (double)pSpinnerDefinition->GetIncrement());
                }
                break;
            }
        }
        break;

    case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SLIDER:
        {
            const PropertyTemplateValueSelector_Slider *pSliderDefinition = static_cast<const PropertyTemplateValueSelector_Slider *>(pPropertyDefinition->GetSelector());
            switch (propertyType)
            {
            case PROPERTY_TYPE_INT:
            case PROPERTY_TYPE_UINT:
                {
                    pPropertyManager = m_pIntSliderPropertyManager;
                    pProperty = m_pIntSliderPropertyManager->addProperty(propertyName);
                    m_pIntSliderPropertyManager->setRange(pProperty, (int)pSliderDefinition->GetMinValue(), (int)pSliderDefinition->GetMaxValue());
                }
                break;
            }
        }
        break;

    case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_CHOICE:
        {
            const PropertyTemplateValueSelector_Choice *pChoiceDefinition = static_cast<const PropertyTemplateValueSelector_Choice *>(pPropertyDefinition->GetSelector());

            pPropertyManager = m_pEnumPropertyManager;
            pProperty = m_pEnumPropertyManager->addProperty(propertyName);

            QStringList enumNames;
            for (uint32 i = 0; i < pChoiceDefinition->GetChoiceCount(); i++)
                enumNames.append(ConvertStringToQString(pChoiceDefinition->GetChoice(i).Right));

            m_pEnumPropertyManager->setEnumNames(pProperty, enumNames);
        }
        break;

    case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_FLAGS:
        {
            const PropertyTemplateValueSelector_Flags *pFlagsDefinition = static_cast<const PropertyTemplateValueSelector_Flags *>(pPropertyDefinition->GetSelector());

            pPropertyManager = m_pFlagPropertyManager;
            pProperty = m_pFlagPropertyManager->addProperty(propertyName);

            QStringList flagNames;
            for (uint32 i = 0; i < pFlagsDefinition->GetFlagCount(); i++)
                flagNames.append(ConvertStringToQString(pFlagsDefinition->GetFlag(i).Label));

            m_pFlagPropertyManager->setFlagNames(pProperty, flagNames);
        }
        break;

        // default selectors
    default:
        {
            switch (propertyType)
            {
            case PROPERTY_TYPE_BOOL:
                {
                    pPropertyManager = m_pBoolPropertyManager;
                    pProperty = m_pBoolPropertyManager->addProperty(propertyName);
                }
                break;

            case PROPERTY_TYPE_STRING:
                {
                    pPropertyManager = m_pStringPropertyManager;
                    pProperty = m_pStringPropertyManager->addProperty(propertyName);
                }
                break;   

            case PROPERTY_TYPE_FLOAT2:
            case PROPERTY_TYPE_FLOAT3:
            case PROPERTY_TYPE_FLOAT4:
                {
                    pPropertyManager = m_pVectorPropertyManager;
                    pProperty = m_pVectorPropertyManager->addProperty(propertyName);

                    switch (propertyType)
                    {
                    case PROPERTY_TYPE_FLOAT2:  m_pVectorPropertyManager->setNumComponents(pProperty, 2);   break;
                    case PROPERTY_TYPE_FLOAT3:  m_pVectorPropertyManager->setNumComponents(pProperty, 3);   break;
                    case PROPERTY_TYPE_FLOAT4:  m_pVectorPropertyManager->setNumComponents(pProperty, 4);   break;
                    }                    
                }
                break;

            case PROPERTY_TYPE_UINT:
            case PROPERTY_TYPE_INT:
            case PROPERTY_TYPE_FLOAT:
            case PROPERTY_TYPE_INT2:
            case PROPERTY_TYPE_INT3:
            case PROPERTY_TYPE_INT4:
            case PROPERTY_TYPE_QUATERNION:
            case PROPERTY_TYPE_TRANSFORM:
                {
                    pPropertyManager = m_pStringPropertyManager;
                    pProperty = m_pStringPropertyManager->addProperty(propertyName);
                }
                break;                         
            }
        }
        break;
    }

    if (pProperty == NULL)
    {
        // create a string property
        pProperty = m_pStringPropertyManager->addProperty(propertyName);
        pPropertyManager = m_pStringPropertyManager;
    }

    // add to our tracking list
    PropertyEntry propertyEntry;
    propertyEntry.PropertyName = pPropertyDefinition->GetName();
    propertyEntry.pPropertyDefinition = pPropertyDefinition;
    propertyEntry.pProperty = pProperty;
    propertyEntry.pPropertyManager = pPropertyManager;
    propertyEntry.OwnsPropertyManager = ownsPropertyManager;
    m_properties.Add(propertyEntry);

    // does this property belong to a category?
    if (pPropertyDefinition->GetCategory().IsEmpty())
    {
        // nope, so just add it to the root
        m_pTreePropertyBrowser->addProperty(pProperty);
    }
    else
    {
        // find the matching property header
        QString propertyCategory(ConvertStringToQString(pPropertyDefinition->GetCategory()));
        QtProperty *pCategoryProperty = nullptr;
        for (uint32 categoryIndex = 0; categoryIndex < m_categories.GetSize(); categoryIndex++)
        {
            if (m_categories[categoryIndex]->propertyName() == propertyCategory)
            {
                pCategoryProperty = m_categories[categoryIndex];
                break;
            }
        }

        // found it?
        if (pCategoryProperty == nullptr)
        {
            // create it
            pCategoryProperty = m_pGroupPropertyManager->addProperty(propertyCategory);
            m_pTreePropertyBrowser->addProperty(pCategoryProperty);
            m_categories.Add(pCategoryProperty);
        }

        // add to this category
        pCategoryProperty->addSubProperty(pProperty);
    }

    // has a value?
    if (currentValue != nullptr)
        UpdatePropertyValue(pPropertyDefinition->GetName(), currentValue);

    // allow updates again
    m_pTreePropertyBrowser->setEnabled(true);
    m_supressPropertyUpdates = false;

    return true;
}

void EditorPropertyEditorWidget::AddPropertiesFromTemplate(const PropertyTemplate *pTemplate)
{
    for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
    {
        const PropertyTemplateProperty *pPropertyDefinition = pTemplate->GetPropertyDefinition(i);

        // skip hidden properties
        if (pPropertyDefinition->GetFlags() & PROPERTY_TEMPLATE_PROPERTY_FLAG_HIDDEN)
            continue;

        // add it
        AddProperty(pPropertyDefinition, pPropertyDefinition->GetDefaultValue());
    }
}

void EditorPropertyEditorWidget::ClearProperties()
{
    m_supressPropertyUpdates = true;
    m_pTreePropertyBrowser->clear();
    m_pTreePropertyBrowser->setEnabled(false);
    m_pHelpText->clear();

    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        if (m_properties[i].OwnsPropertyManager)
        {
            m_pTreePropertyBrowser->unsetFactoryForManager(m_properties[i].pPropertyManager);
            delete m_properties[i].pPropertyManager;
        }
    }
    m_properties.Clear();
    m_categories.Clear();
    m_supressPropertyUpdates = false;

    m_pStringPropertyManager->clear();
    m_pBoolPropertyManager->clear();
    m_pIntSpinBoxPropertyManager->clear();
    m_pIntSliderPropertyManager->clear();
    m_pDoubleSpinBoxPropertyManager->clear();
    m_pGroupPropertyManager->clear();
}

void EditorPropertyEditorWidget::SetDestinationPropertyTable(PropertyTable *pPropertyTable)
{
    m_pDestinationPropertyTable = pPropertyTable;
}

void EditorPropertyEditorWidget::UpdatePropertyValue(const char *propertyName, const char *propertyValue)
{
    // find the property
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        PropertyEntry *pPropertyEntry = &m_properties[i];
        if (pPropertyEntry->PropertyName.Compare(propertyName))
        {
            const PropertyTemplateProperty *pPropertyDefinition = pPropertyEntry->pPropertyDefinition;
            QtProperty *pProperty = pPropertyEntry->pProperty;

            // found it
            // update the property manager
            PROPERTY_TYPE propertyType = pPropertyDefinition->GetType();
            PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE selectorType = (pPropertyDefinition->GetSelector() != NULL) ? pPropertyDefinition->GetSelector()->GetType() : PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_COUNT;
            switch (selectorType)
            {
            case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_RESOURCE:
                {
                    m_supressPropertyUpdates = true;
                    m_pResourcePropertyManager->setValue(pProperty, propertyValue);
                    m_supressPropertyUpdates = false;
                    return;
                }
                break;

            case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SPINNER:
                {
                    switch (propertyType)
                    {
                    case PROPERTY_TYPE_INT:
                    case PROPERTY_TYPE_UINT:
                        {
                            int32 intValue = StringConverter::StringToInt32(propertyValue);

                            m_supressPropertyUpdates = true;
                            m_pIntSpinBoxPropertyManager->setValue(pProperty, intValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;

                    case PROPERTY_TYPE_FLOAT:
                        {
                            float floatValue = StringConverter::StringToFloat(propertyValue);

                            m_supressPropertyUpdates = true;
                            m_pDoubleSpinBoxPropertyManager->setValue(pProperty, (double)floatValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;
                    }
                }
                break;

            case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SLIDER:
                {
                    switch (propertyType)
                    {
                    case PROPERTY_TYPE_INT:
                    case PROPERTY_TYPE_UINT:
                        {
                            int32 intValue = StringConverter::StringToInt32(propertyValue);

                            m_supressPropertyUpdates = true;
                            m_pIntSliderPropertyManager->setValue(pProperty, intValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;
                    }
                }
                break;

            case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_CHOICE:
                {
                    const PropertyTemplateValueSelector_Choice *pChoiceDefinition = static_cast<const PropertyTemplateValueSelector_Choice *>(pPropertyDefinition->GetSelector());

                    // map from our choice to the choice index
                    uint32 choiceIndex = 0;
                    for (uint32 j = 0; j < pChoiceDefinition->GetChoiceCount(); j++)
                    {
                        if (pChoiceDefinition->GetChoice(j).Left.Compare(propertyValue))
                        {
                            choiceIndex = j;
                            break;
                        }
                    }

                    // alter in enum manager
                    Log_DevPrintf("prop update %s: map choice '%s' to %u", pPropertyDefinition->GetName().GetCharArray(), propertyValue, choiceIndex);
                    m_supressPropertyUpdates = true;
                    m_pEnumPropertyManager->setValue(pProperty, (int)choiceIndex);
                    m_supressPropertyUpdates = false;
                    return;
                }
                break;

            case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_FLAGS:
                {
                    const PropertyTemplateValueSelector_Flags *pFlagsDefinition = static_cast<const PropertyTemplateValueSelector_Flags *>(pPropertyDefinition->GetSelector());

                    // map from our flags to the sequential flags
                    uint32 ourMask = StringConverter::StringToUInt32(propertyValue);
                    uint32 qtMask = 0;
                    for (uint32 j = 0; j < pFlagsDefinition->GetFlagCount(); j++)
                    {
                        if (ourMask & pFlagsDefinition->GetFlag(j).Value)
                            qtMask |= (1 << j);
                    }

                    // alter in enum manager
                    Log_DevPrintf("prop update %s: map flags 0x%x to 0x%x", pPropertyDefinition->GetName().GetCharArray(), ourMask, qtMask);
                    m_supressPropertyUpdates = true;
                    m_pFlagPropertyManager->setValue(pProperty, ourMask);
                    m_supressPropertyUpdates = false;
                    return;
                }
                break;

                // default selectors
            default:
                {
                    switch (propertyType)
                    {
                    case PROPERTY_TYPE_BOOL:
                        {
                            bool boolValue = StringConverter::StringToBool(propertyValue);

                            m_supressPropertyUpdates = true;
                            m_pBoolPropertyManager->setValue(pProperty, boolValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;

                    case PROPERTY_TYPE_STRING:
                        {
                            m_supressPropertyUpdates = true;
                            m_pStringPropertyManager->setValue(pProperty, propertyValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;   

                    case PROPERTY_TYPE_FLOAT2:
                    case PROPERTY_TYPE_FLOAT3:
                    case PROPERTY_TYPE_FLOAT4:
                        {
                            m_supressPropertyUpdates = true;
                            m_pVectorPropertyManager->setValueString(pProperty, propertyValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;

                    case PROPERTY_TYPE_UINT:
                    case PROPERTY_TYPE_INT:
                    case PROPERTY_TYPE_FLOAT:
                    case PROPERTY_TYPE_INT2:
                    case PROPERTY_TYPE_INT3:
                    case PROPERTY_TYPE_INT4:
                    case PROPERTY_TYPE_QUATERNION:
                    case PROPERTY_TYPE_TRANSFORM:
                        {
                            m_supressPropertyUpdates = true;
                            m_pStringPropertyManager->setValue(pProperty, propertyValue);
                            m_supressPropertyUpdates = false;
                            return;
                        }
                        break;                         
                    }
                }
                break;
            }

            // if we get here, we were an invalid combination of selector/type. so assume string.
            DebugAssert(pPropertyEntry->pPropertyManager == m_pStringPropertyManager);
            m_supressPropertyUpdates = true;
            m_pStringPropertyManager->setValue(pProperty, propertyValue);
            m_supressPropertyUpdates = false;
            return;
        }
    }
}

void EditorPropertyEditorWidget::UpdatePropertyValueFromTable(const char *propertyName)
{
    // retreive the value from the table, and update it
    DebugAssert(m_pDestinationPropertyTable != nullptr);
    UpdatePropertyValue(propertyName, m_pDestinationPropertyTable->GetPropertyValueDefaultString(propertyName));
}

void EditorPropertyEditorWidget::OnTreeBrowserCurrentItemChanged(QtBrowserItem *pBrowserItem)
{
    m_pHelpText->clear();

    if (pBrowserItem == NULL)
        return;

    QtProperty *pProperty = pBrowserItem->property();

    // find the property entry for the selection
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        PropertyEntry *pEntry = &m_properties[i];
        if (pEntry->pProperty == pProperty)
        {
            LargeString helpText;
            const PropertyTemplateProperty *pDefinition = pEntry->pPropertyDefinition;

            // add the title
            helpText.AppendFormattedString("<strong>%s</strong> (%s)<br>", pDefinition->GetName().GetCharArray(), NameTable_GetNameString(NameTables::PropertyType, pEntry->pPropertyDefinition->GetType()));

            // add the description
            helpText.AppendString(pDefinition->GetDescription());
            helpText.AppendString("<br><br>");

            // add the default value
            helpText.AppendFormattedString("<em>Default Value: %s</em>", pDefinition->GetDefaultValue().GetCharArray());

            // set it
            m_pHelpText->setHtml(ConvertStringToQString(helpText));
            return;
        }
    }

    // if we're here, it means it may be a category property, so just set it with that text
    QString categoryHelp;
    categoryHelp.append("<strong>");
    categoryHelp.append(pProperty->propertyName());
    categoryHelp.append("</strong>");
    m_pHelpText->setHtml(categoryHelp);
}

void EditorPropertyEditorWidget::OnPropertyManagerPropertyChanged(QtProperty *pProperty)
{
    if (m_supressPropertyUpdates || !pProperty->hasValue())
        return;

    // try to get the real (final) value
    String finalName;
    SmallString finalValue;
    if (GetFinalPropertyValue(pProperty, &finalName, &finalValue))
    {
        // signal the event
        OnPropertyValueChange(finalName, finalValue);
    }
}

bool EditorPropertyEditorWidget::GetFinalPropertyValue(const QtProperty *pProperty, String *pPropertyName, String *pPropertyValue)
{
    // search for the property in the list
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        const PropertyEntry &propertyEntry = m_properties[i];
        if (propertyEntry.pProperty == pProperty)
        {
            // fix up property name
            pPropertyName->Assign(propertyEntry.pPropertyDefinition->GetName());

            // some cases have to be changed
            if (propertyEntry.pPropertyDefinition->GetSelector() != nullptr)
            {
                PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE selectorType = propertyEntry.pPropertyDefinition->GetSelector()->GetType();
                switch (selectorType)
                {
                case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_CHOICE:
                    {
                        // map the enum index to the choice index
                        const PropertyTemplateValueSelector_Choice *pChoiceSelector = reinterpret_cast<const PropertyTemplateValueSelector_Choice *>(propertyEntry.pPropertyDefinition->GetSelector());
                        uint32 choiceIndex = (uint32)m_pEnumPropertyManager->value(pProperty);
                        DebugAssert(choiceIndex < pChoiceSelector->GetChoiceCount());
                        pPropertyValue->Assign(pChoiceSelector->GetChoice(choiceIndex).Left);
                        Log_DevPrintf("prop change %s :: map choice %u -> %s", propertyEntry.pPropertyDefinition->GetName().GetCharArray(), choiceIndex, pPropertyValue->GetCharArray());
                        return true;
                    }
                    break;

                case PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_FLAGS:
                    {
                        // map the flags in qt (starting at 1, 2) to our selector
                        const PropertyTemplateValueSelector_Flags *pFlagsSelector = reinterpret_cast<const PropertyTemplateValueSelector_Flags *>(propertyEntry.pPropertyDefinition->GetSelector());
                        uint32 mask = (uint32)m_pFlagPropertyManager->value(pProperty);

                        // start at the first one, iterate through
                        uint32 finalMask = 0;
                        for (uint32 j = 0; j < pFlagsSelector->GetFlagCount(); j++)
                        {
                            if (mask & (1 << j))
                                finalMask |= pFlagsSelector->GetFlag(j).Value;
                        }

                        // spit it out
                        Log_DevPrintf("prop change %s :: map flags 0x%x -> 0x%x", propertyEntry.pPropertyDefinition->GetName().GetCharArray(), mask, finalMask);
                        pPropertyValue->Assign(StringConverter::UInt32ToString(finalMask));
                        return true;
                    }
                    break;
                }
            }

            // no special treatment, just pass through as-is
            String propertyValueString(ConvertQStringToString(pProperty->valueText()));
            Log_DevPrintf("prop change %s -> %s", propertyEntry.pPropertyDefinition->GetName().GetCharArray(), propertyValueString.GetCharArray());
            pPropertyValue->Assign(propertyValueString);
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////

EditorPropertyEditorResourcePropertyManager::EditorPropertyEditorResourcePropertyManager(QObject *pParent /*= NULL*/)
    : QtAbstractPropertyManager(pParent)
{

}

EditorPropertyEditorResourcePropertyManager::~EditorPropertyEditorResourcePropertyManager()
{

}

QString EditorPropertyEditorResourcePropertyManager::value(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.constEnd())
        return QString();

    return itr->val;
}

const ResourceTypeInfo *EditorPropertyEditorResourcePropertyManager::resourceType(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.constEnd())
        return NULL;

    return itr->pResourceTypeInfo;
}

void EditorPropertyEditorResourcePropertyManager::setValue(QtProperty *pProperty, const QString &value)
{
    PropertyValueMap::iterator itr = m_values.find(pProperty);
    if (itr == m_values.end())
        return;

    if (itr->val == value)
        return;

    itr->val = value;

    propertyChanged(pProperty);
    valueChanged(pProperty, itr->val);
}

void EditorPropertyEditorResourcePropertyManager::setResourceType(QtProperty *pProperty, const ResourceTypeInfo *pResourceTypeInfo)
{
    PropertyValueMap::iterator itr = m_values.find(pProperty);
    if (itr == m_values.end())
        return;

    if (itr->pResourceTypeInfo == pResourceTypeInfo)
        return;

    itr->pResourceTypeInfo = pResourceTypeInfo;

    propertyChanged(pProperty);
}

QString EditorPropertyEditorResourcePropertyManager::valueText(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.end())
        return QString();

    return itr->val;
}

void EditorPropertyEditorResourcePropertyManager::initializeProperty(QtProperty *pProperty)
{
    m_values[pProperty] = Data();
}

void EditorPropertyEditorResourcePropertyManager::uninitializeProperty(QtProperty *pProperty)
{
    m_values.remove(pProperty);
}

EditorPropertyEditorResourceEditorFactory::EditorPropertyEditorResourceEditorFactory(QObject *pParent /*= NULL*/)
    : QtAbstractEditorFactory<EditorPropertyEditorResourcePropertyManager>(pParent)
{

}

EditorPropertyEditorResourceEditorFactory::~EditorPropertyEditorResourceEditorFactory()
{
    DebugAssert(m_createdEditors.size() == 0);
}

void EditorPropertyEditorResourceEditorFactory::connectPropertyManager(EditorPropertyEditorResourcePropertyManager *pManager)
{
    connect(pManager, SIGNAL(valueChanged(QtProperty *, const QString &)), this, SLOT(slotManagerValueChanged(QtProperty *, const QString &)));
}

QWidget *EditorPropertyEditorResourceEditorFactory::createEditor(EditorPropertyEditorResourcePropertyManager *pManager, QtProperty *pProperty, QWidget *pParent)
{
    EditorResourceSelectionWidget *pEdit = new EditorResourceSelectionWidget(pParent);

    PropertyToEditorListMap::iterator itr = m_createdEditors.find(pProperty);
    if (itr == m_createdEditors.end())
        itr = m_createdEditors.insert(pProperty, EditorList());

    itr->append(pEdit);
    m_editorToProperty.insert(pEdit, pProperty);

    pEdit->setResourceTypeInfo(pManager->resourceType(pProperty));
    pEdit->setValue(pManager->value(pProperty));

    connect(pEdit, SIGNAL(valueChanged(const QString &)), this, SLOT(slotEditorValueChanged(const QString &)));
    connect(pEdit, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));

    return pEdit;
}

void EditorPropertyEditorResourceEditorFactory::disconnectPropertyManager(EditorPropertyEditorResourcePropertyManager *pManager)
{
    disconnect(pManager, SIGNAL(valueChanged(const QString &)), this, SLOT(slotManagerValueChanged(const QString &)));
}

void EditorPropertyEditorResourceEditorFactory::slotManagerValueChanged(QtProperty *pProperty, const QString &value)
{
    if (!m_createdEditors.contains(pProperty))
        return;

    QListIterator<EditorResourceSelectionWidget *> itEditor(m_createdEditors[pProperty]);
    while (itEditor.hasNext())
    {
        EditorResourceSelectionWidget *editor = itEditor.next();
        if (editor->getValue() != value)
            editor->setValue(value);
    }
}

void EditorPropertyEditorResourceEditorFactory::slotEditorValueChanged(const QString &value)
{
    QObject *pSender = sender();
    const EditorToPropertyMap::ConstIterator ecend = m_editorToProperty.constEnd();
    for (EditorToPropertyMap::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend; ++itEditor)
    {
        if (itEditor.key() == pSender)
        {
            QtProperty *property = itEditor.value();
            EditorPropertyEditorResourcePropertyManager *manager = propertyManager(property);
            if (manager == NULL)
                return;

            manager->setValue(property, value);
            return;
        }
    }
}

void EditorPropertyEditorResourceEditorFactory::slotEditorDestroyed(QObject *pEditor)
{
    const EditorToPropertyMap::iterator ecend = m_editorToProperty.end();
    for (EditorToPropertyMap::iterator itEditor = m_editorToProperty.begin(); itEditor !=  ecend; ++itEditor)
    {
        if (itEditor.key() == pEditor)
        {
            EditorResourceSelectionWidget *editor = itEditor.key();
            QtProperty *property = itEditor.value();
            const PropertyToEditorListMap::iterator pit = m_createdEditors.find(property);
            if (pit != m_createdEditors.end())
            {
                pit.value().removeAll(editor);
                if (pit.value().empty())
                    m_createdEditors.erase(pit);
            }
            m_editorToProperty.erase(itEditor);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

EditorPropertyEditorVectorPropertyManager::EditorPropertyEditorVectorPropertyManager(QObject *pParent /*= NULL*/)
    : QtAbstractPropertyManager(pParent)
{

}

EditorPropertyEditorVectorPropertyManager::~EditorPropertyEditorVectorPropertyManager()
{

}

uint32 EditorPropertyEditorVectorPropertyManager::numComponents(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.constEnd())
        return 4;

    return itr->numComponents;
}

QString EditorPropertyEditorVectorPropertyManager::valueString(const QtProperty *pProperty) const
{
    return valueText(pProperty);
}

float4 EditorPropertyEditorVectorPropertyManager::valueVector(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.constEnd())
        return float4::Zero;

    return itr->valueVector;
}

void EditorPropertyEditorVectorPropertyManager::setNumComponents(QtProperty *pProperty, uint32 nComponents)
{
    PropertyValueMap::iterator itr = m_values.find(pProperty);
    if (itr == m_values.end())
        return;

    if (itr->numComponents == nComponents)
        return;

    itr->numComponents = nComponents;
    propertyChanged(pProperty);
}

void EditorPropertyEditorVectorPropertyManager::setValueString(QtProperty *pProperty, const char *value)
{
    PropertyValueMap::iterator itr = m_values.find(pProperty);
    if (itr == m_values.end())
        return;

    float4 newValue(float4::Zero);

    switch (itr->numComponents)
    {
    case 1:     newValue = float4(StringConverter::StringToFloat(value), 0.0f, 0.0f, 0.0f);     break;
    case 2:     newValue = float4(StringConverter::StringToFloat2(value), 0.0f, 0.0f);          break;
    case 3:     newValue = float4(StringConverter::StringToFloat3(value), 0.0f);                break;
    case 4:     newValue = StringConverter::StringToFloat4(value);                              break;
    }

    if (itr->valueVector == newValue)
        return;

    itr->valueVector = newValue;
    propertyChanged(pProperty);
    valueChanged(pProperty, newValue);
}

void EditorPropertyEditorVectorPropertyManager::setValueVector(QtProperty *pProperty, const float4 &value)
{
    PropertyValueMap::iterator itr = m_values.find(pProperty);
    if (itr == m_values.end())
        return;

    if (itr->valueVector == value)
        return;

    itr->valueVector = value;
    propertyChanged(pProperty);
    valueChanged(pProperty, value);
}

QString EditorPropertyEditorVectorPropertyManager::valueText(const QtProperty *pProperty) const
{
    PropertyValueMap::const_iterator itr = m_values.constFind(pProperty);
    if (itr == m_values.constEnd())
        return QString();

    switch (itr->numComponents)
    {
    case 1:     return ConvertStringToQString(StringConverter::FloatToString(itr->valueVector.x));          break;
    case 2:     return ConvertStringToQString(StringConverter::Float2ToString(itr->valueVector.xy()));      break;
    case 3:     return ConvertStringToQString(StringConverter::Float3ToString(itr->valueVector.xyz()));     break;
    case 4:     return ConvertStringToQString(StringConverter::Float4ToString(itr->valueVector));           break;
    }

    return QString();
}

void EditorPropertyEditorVectorPropertyManager::initializeProperty(QtProperty *pProperty)
{
    m_values[pProperty] = Data();
}

void EditorPropertyEditorVectorPropertyManager::uninitializeProperty(QtProperty *pProperty)
{
    m_values.remove(pProperty);
}

EditorPropertyEditorVectorEditFactory::EditorPropertyEditorVectorEditFactory(QObject *pParent /*= NULL*/)
    : QtAbstractEditorFactory<EditorPropertyEditorVectorPropertyManager>(pParent)
{

}

EditorPropertyEditorVectorEditFactory::~EditorPropertyEditorVectorEditFactory()
{
    DebugAssert(m_createdEditors.size() == 0);
}

void EditorPropertyEditorVectorEditFactory::connectPropertyManager(EditorPropertyEditorVectorPropertyManager *pManager)
{
    connect(pManager, SIGNAL(valueChanged(QtProperty *, const QString &)), this, SLOT(slotManagerValueChanged(QtProperty *, const QString &)));
}

QWidget *EditorPropertyEditorVectorEditFactory::createEditor(EditorPropertyEditorVectorPropertyManager *pManager, QtProperty *pProperty, QWidget *pParent)
{
    EditorVectorEditWidget *pEdit = new EditorVectorEditWidget(pParent);

    PropertyToEditorListMap::iterator itr = m_createdEditors.find(pProperty);
    if (itr == m_createdEditors.end())
        itr = m_createdEditors.insert(pProperty, EditorList());

    itr->append(pEdit);
    m_editorToProperty.insert(pEdit, pProperty);

    pEdit->SetNumComponents(pManager->numComponents(pProperty));

    float4 value(pManager->valueVector(pProperty));
    pEdit->SetValue(value.x, value.y, value.z, value.w);

    connect(pEdit, SIGNAL(ValueChangedFloat4(const float4 &)), this, SLOT(slotEditorValueChanged(const float4 &)));
    connect(pEdit, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));

    return pEdit;
}

void EditorPropertyEditorVectorEditFactory::disconnectPropertyManager(EditorPropertyEditorVectorPropertyManager *pManager)
{
    disconnect(pManager, SIGNAL(valueChanged(const float4 &)), this, SLOT(slotManagerValueChanged(const float4 &)));
}

void EditorPropertyEditorVectorEditFactory::slotManagerValueChanged(QtProperty *pProperty, const float4 &value)
{
    if (!m_createdEditors.contains(pProperty))
        return;

    QListIterator<EditorVectorEditWidget *> itEditor(m_createdEditors[pProperty]);
    while (itEditor.hasNext())
    {
        EditorVectorEditWidget *editor = itEditor.next();
        if (editor->GetValueFloat4() != value)
            editor->SetValue(value.x, value.y, value.z, value.w);
    }
}

void EditorPropertyEditorVectorEditFactory::slotEditorValueChanged(const float4 &value)
{
    QObject *pSender = sender();
    const EditorToPropertyMap::ConstIterator ecend = m_editorToProperty.constEnd();
    for (EditorToPropertyMap::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend; ++itEditor)
    {
        if (itEditor.key() == pSender)
        {
            QtProperty *property = itEditor.value();
            EditorPropertyEditorVectorPropertyManager *manager = propertyManager(property);
            if (manager == NULL)
                return;

            manager->setValueVector(property, value);
            return;
        }
    }
}

void EditorPropertyEditorVectorEditFactory::slotEditorDestroyed(QObject *pEditor)
{
    const EditorToPropertyMap::iterator ecend = m_editorToProperty.end();
    for (EditorToPropertyMap::iterator itEditor = m_editorToProperty.begin(); itEditor !=  ecend; ++itEditor)
    {
        if (itEditor.key() == pEditor)
        {
            EditorVectorEditWidget *editor = itEditor.key();
            QtProperty *property = itEditor.value();
            const PropertyToEditorListMap::iterator pit = m_createdEditors.find(property);
            if (pit != m_createdEditors.end())
            {
                pit.value().removeAll(editor);
                if (pit.value().empty())
                    m_createdEditors.erase(pit);
            }
            m_editorToProperty.erase(itEditor);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

