#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
Log_SetChannel(EditorProgressDialog);

EditorProgressDialog::EditorProgressDialog(QWidget *pParent, Qt::WindowFlags windowFlags /*= 0*/)
    : QDialog(pParent, windowFlags),
      EditorProgressCallbacks(pParent),
      m_ui(new Ui_EditorProgressDialog())
{
    m_ui->setupUi(this);
    setModal(true);
}

EditorProgressDialog::~EditorProgressDialog()
{
    delete m_ui;
}

void EditorProgressDialog::SetCancellable(bool cancellable)
{
    BaseProgressCallbacks::SetCancellable(cancellable);

    m_ui->m_cancelButton->setEnabled(false);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorProgressDialog::SetStatusText(const char *statusText)
{
    BaseProgressCallbacks::SetStatusText(statusText);
    
    m_ui->m_status->setText(statusText);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorProgressDialog::SetProgressRange(uint32 range)
{
    BaseProgressCallbacks::SetProgressRange(range);

    m_ui->m_progressBar->setRange(0, m_progressRange);
    m_ui->m_progressBar->setValue(m_progressValue);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorProgressDialog::SetProgressValue(uint32 value)
{
#if 1
    float oldPercent = (m_progressRange != 0) ? Math::Clamp((float)m_progressValue / (float)m_progressRange, 0.0f, 1.0f) : 0.0f;
    int32 oldWidthInPixels = Math::Truncate(oldPercent * (float)m_ui->m_progressBar->width());

    BaseProgressCallbacks::SetProgressValue(value);

    float newPercent = (m_progressRange != 0) ? Math::Clamp((float)m_progressValue / (float)m_progressRange, 0.0f, 1.0f) : 0.0f;
    int32 newWidthInPixels = Math::Truncate(newPercent * (float)m_ui->m_progressBar->width());

    if (oldWidthInPixels != newWidthInPixels)
    {
        m_ui->m_progressBar->setValue(m_progressValue);
        g_pEditor->ProcessBackgroundEvents();
    }
#else
    float oldPercent = (m_progressRange != 0) ? Math::Clamp((float)m_progressValue / (float)m_progressRange, 0.0f, 1.0f) : 0.0f;

    BaseProgressCallbacks::SetProgressValue(value);

    float newPercent = (m_progressRange != 0) ? Math::Clamp((float)m_progressValue / (float)m_progressRange, 0.0f, 1.0f) : 0.0f;

    if (Math::Truncate(oldPercent) != Math::Truncate(newPercent))
    {
        m_ui->m_progressBar->setValue(m_progressValue);
        ProcessEvents();
    }
#endif
}

void EditorProgressDialog::DisplayError(const char *message)
{
    // push to log for now, fix me later when the ui is updated
    Log_ErrorPrint(message);
}

void EditorProgressDialog::DisplayWarning(const char *message)
{
    // push to log for now, fix me later when the ui is updated
    Log_WarningPrint(message);
}

void EditorProgressDialog::DisplayInformation(const char *message)
{
    // push to log for now, fix me later when the ui is updated
    Log_InfoPrint(message);
}

void EditorProgressDialog::DisplayDebugMessage(const char *message)
{
    // push to log for now, fix me later when the ui is updated
    Log_DevPrint(message);
}
