#pragma once
#include "Editor/Common.h"
#include "Editor/EditorProgressCallbacks.h"
#include "Editor/ui_EditorProgressDialog.h"

class EditorProgressDialog : public QDialog, public EditorProgressCallbacks
{
    Q_OBJECT

public:
    EditorProgressDialog(QWidget *pParent, Qt::WindowFlags windowFlags = 0);
    ~EditorProgressDialog();

    virtual void SetCancellable(bool cancellable);
    virtual void SetStatusText(const char *statusText);
    virtual void SetProgressRange(uint32 range);
    virtual void SetProgressValue(uint32 value);

    virtual void DisplayError(const char *message);
    virtual void DisplayWarning(const char *message);
    virtual void DisplayInformation(const char *message);
    virtual void DisplayDebugMessage(const char *message);

private:
    Ui_EditorProgressDialog *m_ui;
};

