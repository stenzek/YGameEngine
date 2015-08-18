#pragma once
#include "Editor/Common.h"
#include "Core/PropertyTable.h"
#include "Core/PropertyTemplate.h"

class ResourceTypeInfo;

class EditorEntityPropertyDefinition;
class EditorMapWindow;
class EditorMap;

class EditorResourceSelectionWidget;
class EditorPropertyEditorResourcePropertyManager;
class EditorPropertyEditorResourceEditorFactory;

class EditorVectorEditWidget;
class EditorPropertyEditorVectorPropertyManager;
class EditorPropertyEditorVectorEditFactory;

class EditorPropertyEditorWidget : public QWidget
{
    Q_OBJECT

public:
    EditorPropertyEditorWidget(QWidget *pParent);
    ~EditorPropertyEditorWidget();

    const PropertyTable *GetDestinationPropertyTable() const { return m_pDestinationPropertyTable; }
    PropertyTable *GetDestinationPropertyTable() { return m_pDestinationPropertyTable; }

public Q_SLOTS:
    // set the title text
    //void SetTitleText(const char *titleText);

    // adds a property to the editor
    bool AddProperty(const PropertyTemplateProperty *pPropertyDefinition, const char *currentValue);

    // initialize the editor for the specified property template
    void AddPropertiesFromTemplate(const PropertyTemplate *pTemplate);

    // clear everything from the property editor
    void ClearProperties();

    // set the property list this editor is connected to
    void SetDestinationPropertyTable(PropertyTable *pPropertyTable);

    // update property value from an external source (only updates UI, not table)
    void UpdatePropertyValue(const char *propertyName, const char *propertyValue);

    // update property value from bound table (only updates UI)
    void UpdatePropertyValueFromTable(const char *propertyName);

Q_SIGNALS:
    void OnPropertyValueChange(const char *propertyName, const char *propertyValue);

private Q_SLOTS:
    void OnTreeBrowserCurrentItemChanged(QtBrowserItem *pBrowserItem);
    void OnPropertyManagerPropertyChanged(QtProperty *pProperty);

private:
    void CreateUI();
    void ConnectEvents();
    bool GetFinalPropertyValue(const QtProperty *pProperty, String *pPropertyName, String *pPropertyValue);

    // property managers
    QtGroupPropertyManager *m_pGroupPropertyManager;
    QtBoolPropertyManager *m_pBoolPropertyManager;
    QtStringPropertyManager *m_pStringPropertyManager;
    QtIntPropertyManager *m_pIntSpinBoxPropertyManager;
    QtIntPropertyManager *m_pIntSliderPropertyManager;
    QtDoublePropertyManager *m_pDoubleSpinBoxPropertyManager;
    QtEnumPropertyManager *m_pEnumPropertyManager;
    QtFlagPropertyManager *m_pFlagPropertyManager;
    EditorPropertyEditorResourcePropertyManager *m_pResourcePropertyManager;
    EditorPropertyEditorVectorPropertyManager *m_pVectorPropertyManager;
    bool m_supressPropertyUpdates;

    // ui elements
    QtTreePropertyBrowser *m_pTreePropertyBrowser;
    //QLabel *m_pTitleText;
    QTextEdit *m_pHelpText;

    // for entities
    struct PropertyEntry
    {
        String PropertyName;
        const PropertyTemplateProperty *pPropertyDefinition;
        QtProperty *pProperty;
        QtAbstractPropertyManager *pPropertyManager;
        bool OwnsPropertyManager;
    };
    MemArray<PropertyEntry> m_properties;
    PODArray<QtProperty *> m_categories;
    PropertyTable *m_pDestinationPropertyTable;
};

class EditorPropertyEditorResourcePropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT

public:
    EditorPropertyEditorResourcePropertyManager(QObject *pParent = NULL);
    ~EditorPropertyEditorResourcePropertyManager();

    QString value(const QtProperty *pProperty) const;
    const ResourceTypeInfo *resourceType(const QtProperty *pProperty) const;

public Q_SLOTS:
    void setValue(QtProperty *pProperty, const QString &value);
    void setResourceType(QtProperty *pProperty, const ResourceTypeInfo *pResourceTypeInfo);

Q_SIGNALS:
    void valueChanged(QtProperty *pProperty, const QString &value);

protected:
    virtual QString valueText(const QtProperty *pProperty) const;
    virtual void initializeProperty(QtProperty *pProperty);
    virtual void uninitializeProperty(QtProperty *pProperty);

private:
    struct Data
    {
        Data() : pResourceTypeInfo(NULL) {}

        QString val;
        const ResourceTypeInfo *pResourceTypeInfo;
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;
};

class EditorPropertyEditorResourceEditorFactory : public QtAbstractEditorFactory<EditorPropertyEditorResourcePropertyManager>
{
    Q_OBJECT

public:
    EditorPropertyEditorResourceEditorFactory(QObject *pParent = NULL);
    ~EditorPropertyEditorResourceEditorFactory();

protected:
    virtual void connectPropertyManager(EditorPropertyEditorResourcePropertyManager *pManager);
    virtual QWidget *createEditor(EditorPropertyEditorResourcePropertyManager *pManager, QtProperty *pProperty, QWidget *pParent);
    virtual void disconnectPropertyManager(EditorPropertyEditorResourcePropertyManager *pManager);

private Q_SLOTS:
    void slotManagerValueChanged(QtProperty *pProperty, const QString &value);
    void slotEditorValueChanged(const QString &value);
    void slotEditorDestroyed(QObject *pEditor);

private:
    typedef QList<EditorResourceSelectionWidget *> EditorList;
    typedef QMap<QtProperty *, EditorList> PropertyToEditorListMap;
    typedef QMap<EditorResourceSelectionWidget *, QtProperty *> EditorToPropertyMap;

    PropertyToEditorListMap  m_createdEditors;
    EditorToPropertyMap m_editorToProperty;
};

class EditorPropertyEditorVectorPropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT

public:
    EditorPropertyEditorVectorPropertyManager(QObject *pParent = NULL);
    ~EditorPropertyEditorVectorPropertyManager();

    uint32 numComponents(const QtProperty *pProperty) const;
    QString valueString(const QtProperty *pProperty) const;
    float4 valueVector(const QtProperty *pProperty) const;

public Q_SLOTS:
    void setNumComponents(QtProperty *pProperty, uint32 nComponents);
    void setValueString(QtProperty *pProperty, const char *value);
    void setValueVector(QtProperty *pProperty, const float4 &value);

Q_SIGNALS:
    void valueChanged(QtProperty *pProperty, const float4 &value);

protected:
    virtual QString valueText(const QtProperty *pProperty) const;
    virtual void initializeProperty(QtProperty *pProperty);
    virtual void uninitializeProperty(QtProperty *pProperty);

private:
    struct Data
    {
        Data() : numComponents(4), valueVector(float4::Zero) {}

        uint32 numComponents;
        float4 valueVector;
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;
};

class EditorPropertyEditorVectorEditFactory : public QtAbstractEditorFactory<EditorPropertyEditorVectorPropertyManager>
{
    Q_OBJECT

public:
    EditorPropertyEditorVectorEditFactory(QObject *pParent = NULL);
    ~EditorPropertyEditorVectorEditFactory();

protected:
    virtual void connectPropertyManager(EditorPropertyEditorVectorPropertyManager *pManager);
    virtual QWidget *createEditor(EditorPropertyEditorVectorPropertyManager *pManager, QtProperty *pProperty, QWidget *pParent);
    virtual void disconnectPropertyManager(EditorPropertyEditorVectorPropertyManager *pManager);

private Q_SLOTS:
    void slotManagerValueChanged(QtProperty *pProperty, const float4 &value);
    void slotEditorValueChanged(const float4 &value);
    void slotEditorDestroyed(QObject *pEditor);

private:
    typedef QList<EditorVectorEditWidget *> EditorList;
    typedef QMap<QtProperty *, EditorList> PropertyToEditorListMap;
    typedef QMap<EditorVectorEditWidget *, QtProperty *> EditorToPropertyMap;

    PropertyToEditorListMap  m_createdEditors;
    EditorToPropertyMap m_editorToProperty;
};

