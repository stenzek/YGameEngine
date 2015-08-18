#pragma once
#include "Editor/Common.h"
#include "Editor/EditorResourceSelectionDialogModel.h"

class EditorResourcePreviewWidget;

class EditorResourceSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    EditorResourceSelectionDialog(QWidget *pParent);
    ~EditorResourceSelectionDialog();

    const ResourceTypeInfo *GetReturnValueResourceType() const { return m_pReturnValueResourceType; }
    const String &GetReturnValueResourceName() const { return m_returnValueResourceName; }

    void AddResourceFilter(const ResourceTypeInfo *pResourceTypeInfo);
    bool NavigateToDirectory(const char *directory, bool addToHistory = true);

private Q_SLOTS:
    void OnDirectoryComboBoxActivated(const QString &text);
    void OnBackButtonPressed();
    void OnUpDirectoryButtonPressed();
    void OnMakeDirectoryButtonPressed();
    void OnTreeViewItemActivated(const QModelIndex &index);
    void OnTreeViewItemClicked(const QModelIndex &index);
    void OnSelectButtonClicked();
    void OnCancelButtonClicked();

private:
    void CreateUI();

    PODArray<const ResourceTypeInfo *> m_resourceFilters;
    EditorResourceSelectionDialogModel *m_pModel;
    QStack<QString> m_history;
    const ResourceTypeInfo *m_pReturnValueResourceType;
    String m_returnValueResourceName;

    QComboBox *m_pDirectoryComboBox;  
    QToolButton *m_pBackButton;
    QToolButton *m_pUpDirectoryButton;
    QTreeView *m_pTreeView;
    QLabel *m_pFilterTextLabel;
    QPushButton *m_pSelectButton;  
    EditorResourcePreviewWidget *m_pPreviewWidget;
};