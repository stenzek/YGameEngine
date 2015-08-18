#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorProgressCallbacks.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
Log_SetChannel(EditorProgressDialog);

EditorProgressCallbacks::EditorProgressCallbacks(QWidget *parentWidgetForPopups)
    : m_pParentWidgetForPopups(parentWidgetForPopups)
{

}

EditorProgressCallbacks::~EditorProgressCallbacks()
{

}

void EditorProgressCallbacks::ModalError(const char *message)
{
    g_pEditor->BlockFrameExecution();

    QMessageBox::critical(m_pParentWidgetForPopups, QStringLiteral("Error"), message, QMessageBox::Ok, QMessageBox::Ok);

    g_pEditor->UnblockFrameExecution();
    g_pEditor->ProcessBackgroundEvents();
}

bool EditorProgressCallbacks::ModalConfirmation(const char *message)
{
    g_pEditor->BlockFrameExecution();

    bool result = (QMessageBox::question(m_pParentWidgetForPopups, QStringLiteral("Question"), message, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes);

    g_pEditor->UnblockFrameExecution();

    g_pEditor->ProcessBackgroundEvents();
    return result;
}

uint32 EditorProgressCallbacks::ModalPrompt(const char *message, uint32 nOptions, ...)
{
    DebugAssert(nOptions > 0 && nOptions < 5);

    va_list ap;
    va_start(ap, nOptions);

    QMessageBox messageBox(m_pParentWidgetForPopups);

    switch (nOptions)
    {
    case 4:
        messageBox.addButton(va_arg(ap, const char *), QMessageBox::NoRole);

    case 3:
        messageBox.addButton(va_arg(ap, const char *), QMessageBox::NoRole);

    case 2:
        messageBox.addButton(va_arg(ap, const char *), QMessageBox::NoRole);

    case 1:
        messageBox.addButton(va_arg(ap, const char *), QMessageBox::NoRole);
        break;
    }

    g_pEditor->BlockFrameExecution();
    uint32 result = (uint32)messageBox.exec();
    g_pEditor->UnblockFrameExecution();

    g_pEditor->ProcessBackgroundEvents();
    return result;
}
