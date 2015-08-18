#pragma once
#include "Editor/Common.h"
#include "Editor/EditorResourceSelectionDialogModel.h"

class EditorResourceSaveDialog : public QDialog
{
    Q_OBJECT

public:
    EditorResourceSaveDialog(QWidget *pParent, const ResourceTypeInfo *pResourceTypeInfo);
    ~EditorResourceSaveDialog();

    const ResourceTypeInfo *GetResourceTypeInfo() const { return m_pResourceTypeInfo; }
    void SetResourceTypeInfo(const ResourceTypeInfo *pResourceTypeInfo) { m_pResourceTypeInfo = pResourceTypeInfo; }

    const String GetReturnValueResourceName() const { return m_returnValueResourceName; }

    bool NavigateToDirectory(const char *directory, bool addToHistory = true);

private Q_SLOTS:
    void OnDirectoryComboBoxActivated(const QString &text);
    void OnBackButtonPressed();
    void OnUpDirectoryButtonPressed();
    void OnMakeDirectoryButtonPressed();
    void OnTreeViewItemActivated(const QModelIndex &index);
    void OnTreeViewItemClicked(const QModelIndex &index);
    void OnResourceNameEditTextChanged(const QString &contents);
    void OnSaveButtonClicked();
    void OnCancelButtonClicked();

private:
    void CreateUI();
    void UpdateReturnValue();

    const ResourceTypeInfo *m_pResourceTypeInfo;
    EditorResourceSelectionDialogModel *m_pModel;
    QStack<QString> m_history;

    QComboBox *m_pDirectoryComboBox;  
    QToolButton *m_pBackButton;
    QToolButton *m_pUpDirectoryButton;
    QTreeView *m_pTreeView;
    QLabel *m_pFilterTextLabel;

    QComboBox *m_pResourceNameComboBox;
    String m_returnValueResourceName;

    QPushButton *m_pSaveButton;  
};