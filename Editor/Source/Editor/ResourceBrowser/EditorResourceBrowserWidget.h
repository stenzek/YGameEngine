#pragma once
#include "Editor/Common.h"

struct Ui_EditorResourceBrowserWidget;
class ResourceTypeInfo;
class EditorMapWindow;
class EditorResourceSelectionDialogModel;

class EditorResourceBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    EditorResourceBrowserWidget(EditorMapWindow *pMapWindow, QWidget *parent);
    virtual ~EditorResourceBrowserWidget();

    const String &GetCurrentDirectory() const { return m_currentDirectory; }
    void NavigateToDirectory(const char *path);

private:
    Ui_EditorResourceBrowserWidget *m_ui;

    EditorMapWindow *m_pMapWindow;
    String m_currentDirectory;
    String m_selectedResourceName;
    const ResourceTypeInfo *m_pSelectedResourceType;

    void ConnectUIEvents();

private Q_SLOTS:
    void OnActionDirectoryBackTriggered();
    void OnActionDirectoryUpTriggered();
    void OnActionNewDirectoryTriggered();
    void OnActionDeleteResourceTriggered();
    void OnActionImportStaticMeshTriggered();
    void OnActionInsertResourceTriggered();
    void OnActionEditBlockMeshTriggered();
    void OnActionEditSkeletalMeshTriggered();
    void OnActionEditSkeletalAnimationTriggered();
    void OnActionEditStaticMeshTriggered();
    void OnDirectoryTreeItemClickedOrActivated(const QModelIndex &index);
    void OnResourceListItemClicked(const QModelIndex &index);
    void OnResourceListItemActivated(const QModelIndex &index);
};

