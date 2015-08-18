#pragma once
#include "Editor/Common.h"

class EditorProgressCallbacks : public BaseProgressCallbacks
{
public:
    EditorProgressCallbacks(QWidget *parentWidgetForPopups);
    virtual ~EditorProgressCallbacks();

    virtual void ModalError(const char *message) override;
    virtual bool ModalConfirmation(const char *message) override;
    virtual uint32 ModalPrompt(const char *message, uint32 nOptions, ...) override;

private:
    QWidget *m_pParentWidgetForPopups;
};

