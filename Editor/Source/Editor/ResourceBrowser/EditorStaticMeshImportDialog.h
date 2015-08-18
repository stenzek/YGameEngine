#pragma once
#include "Editor/Common.h"
#include "Editor/EditorProgressCallbacks.h"

class Ui_EditorStaticMeshImportDialog;

class EditorStaticMeshImportDialog : public QDialog, private EditorProgressCallbacks
{
    Q_OBJECT

public:
    EditorStaticMeshImportDialog(QWidget *parent);
    virtual ~EditorStaticMeshImportDialog();

    void SetMaterialDirectory(const QString &materialDirectory);

private:
    Ui_EditorStaticMeshImportDialog *m_ui;

private:
    // progress callbacks interface
    virtual void SetStatusText(const char *statusText) override;
    virtual void SetProgressRange(uint32 range) override;
    virtual void SetProgressValue(uint32 value) override;
    virtual void DisplayError(const char *message) override;
    virtual void DisplayWarning(const char *message) override;
    virtual void DisplayInformation(const char *message) override;
    virtual void DisplayDebugMessage(const char *message) override;

    virtual void closeEvent(QCloseEvent *pCloseEvent) override;

    void ConnectUIEvents();
    void UpdateUIState();

private Q_SLOTS:
    void OnFileNameLineEditEditingFinished();
    void OnFileNameBrowseClicked();
    void OnImportButtonClicked();
    void OnCloseButtonClicked();
};

