#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorEditMode.h"
#include "Editor/MapEditor/EditorMapViewport.h"

EditorEditMode::EditorEditMode(EditorMap *pMap)
    : m_pMap(pMap),
      m_bActive(false),
      m_cameraLeftMouseButtonDown(false),
      m_cameraRightMouseButtonDown(false)
{

}

EditorEditMode::~EditorEditMode()
{

}

bool EditorEditMode::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    return true;
}

QWidget *EditorEditMode::CreateUI(QWidget *parentWidget)
{
    QWidget *rootWidget = new QWidget(parentWidget);
    return rootWidget;
}

void EditorEditMode::Activate()
{
    DebugAssert(!m_bActive);
    m_bActive = true;
}

void EditorEditMode::Deactivate()
{
    DebugAssert(m_bActive);
    m_bActive = false;
    m_cameraLeftMouseButtonDown = false;
    m_cameraRightMouseButtonDown = false;
}

void EditorEditMode::Update(const float timeSinceLastUpdate)
{

}

void EditorEditMode::OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue)
{

}

void EditorEditMode::OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport)
{
    m_cameraLeftMouseButtonDown = false;
    m_cameraRightMouseButtonDown = false;
}

bool EditorEditMode::HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent)
{
    // pass through to camera
    if (pViewport->GetViewController().HandleKeyboardEvent(pKeyboardEvent))
        return true;

    return false;
}

bool EditorEditMode::HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent)
{
    if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton)
    {
        if ((m_cameraLeftMouseButtonDown | m_cameraRightMouseButtonDown) == 0)
        {
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_CROSS);
            pViewport->LockMouseCursor();
        }

        m_cameraLeftMouseButtonDown = true;
        return true;
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton)
    {
        m_cameraLeftMouseButtonDown = false;

        if ((m_cameraLeftMouseButtonDown | m_cameraRightMouseButtonDown) == 0)
        {
            pViewport->UnlockMouseCursor();
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
        }

        return true;
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton)
    {
        if ((m_cameraLeftMouseButtonDown | m_cameraRightMouseButtonDown) == 0)
        {
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_CROSS);
            pViewport->LockMouseCursor();
        }

        m_cameraRightMouseButtonDown = true;
        return true;
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton)
    {
        m_cameraRightMouseButtonDown = false;

        if ((m_cameraLeftMouseButtonDown | m_cameraRightMouseButtonDown) == 0)
        {
            pViewport->UnlockMouseCursor();
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
        }

        return true;
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        // pass to camera
        if (m_cameraLeftMouseButtonDown)
        {
            pViewport->GetViewController().MoveFromMousePosition(pViewport->GetMouseDelta());
            return true;
        }

        if (m_cameraRightMouseButtonDown)
        {
            pViewport->GetViewController().RotateFromMousePosition(pViewport->GetMouseDelta());
            return true;
        }
    }

    return false;
}

bool EditorEditMode::HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent)
{
    return false;
}

void EditorEditMode::OnViewportDrawAfterWorld(EditorMapViewport *pViewport)
{

}

void EditorEditMode::OnViewportDrawBeforeWorld(EditorMapViewport *pViewport)
{

}

void EditorEditMode::OnViewportDrawAfterPost(EditorMapViewport *pViewport)
{

}

void EditorEditMode::OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport)
{

}

void EditorEditMode::OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport)
{

}

bool EditorEditMode::HandleResourceViewResourceActivatedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName)
{
    return false;
}

bool EditorEditMode::HandleResourceViewResourceDroppedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName, const EditorMapViewport *pViewport, int32 x, int32 y)
{
    return false;
}
