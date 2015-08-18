#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/EditorHelpers.h"
#include "Renderer/Renderer.h"

EditorRendererSwapChainWidget::EditorRendererSwapChainWidget(QWidget *pParent, Qt::WindowFlags flags /* = 0 */)
    : QWidget(pParent, flags | Qt::MSWindowsOwnDC),
      m_pSwapChain(NULL)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    RecreateSwapChain();
}

EditorRendererSwapChainWidget::~EditorRendererSwapChainWidget()
{
    SAFE_RELEASE(m_pSwapChain);
}

RendererOutputBuffer *EditorRendererSwapChainWidget::GetSwapChain() const
{
    if (m_pSwapChain == NULL)
        const_cast<EditorRendererSwapChainWidget *>(this)->RecreateSwapChain();

    return m_pSwapChain;
}

void EditorRendererSwapChainWidget::DestroySwapChain()
{
    if (m_pSwapChain != NULL)
    {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }
}

void EditorRendererSwapChainWidget::RecreateSwapChain()
{
    if (m_pSwapChain != NULL)
        m_pSwapChain->Release();

    m_pSwapChain = g_pRenderer->CreateOutputBuffer(reinterpret_cast<RenderSystemWindowHandle>(winId()), RENDERER_VSYNC_TYPE_NONE);
}

QPaintEngine *EditorRendererSwapChainWidget::paintEngine() const
{
    return NULL;
}

void EditorRendererSwapChainWidget::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);

    if (m_pSwapChain == NULL || !m_pSwapChain->ResizeBuffers())
        RecreateSwapChain();

    ResizedEvent();
}

void EditorRendererSwapChainWidget::paintEvent(QPaintEvent *pEvent)
{
    PaintEvent();
}

void EditorRendererSwapChainWidget::focusInEvent(QFocusEvent *pEvent)
{
    QWidget::focusInEvent(pEvent);

    GainedFocusEvent();
}

void EditorRendererSwapChainWidget::focusOutEvent(QFocusEvent *pEvent)
{
    QWidget::focusOutEvent(pEvent);

    LostFocusEvent();
}

void EditorRendererSwapChainWidget::keyPressEvent(QKeyEvent *pKeyEvent)
{
    QWidget::keyPressEvent(pKeyEvent);
    KeyboardEvent(pKeyEvent);
}

void EditorRendererSwapChainWidget::keyReleaseEvent(QKeyEvent *pKeyEvent)
{
    QWidget::keyReleaseEvent(pKeyEvent);
    KeyboardEvent(pKeyEvent);
}

void EditorRendererSwapChainWidget::mouseMoveEvent(QMouseEvent *pMouseEvent)
{
    QWidget::mouseMoveEvent(pMouseEvent);
    MouseEvent(pMouseEvent);
}

void EditorRendererSwapChainWidget::mousePressEvent(QMouseEvent *pMouseEvent)
{
    QWidget::mousePressEvent(pMouseEvent);
    MouseEvent(pMouseEvent);
}

void EditorRendererSwapChainWidget::mouseReleaseEvent(QMouseEvent *pMouseEvent)
{
    QWidget::mouseReleaseEvent(pMouseEvent);
    MouseEvent(pMouseEvent);
}

void EditorRendererSwapChainWidget::wheelEvent(QWheelEvent *pWheelEvent)
{
    QWidget::wheelEvent(pWheelEvent);
    WheelEvent(pWheelEvent);
}

void EditorRendererSwapChainWidget::dropEvent(QDropEvent *pDropEvent)
{
    QWidget::dropEvent(pDropEvent);
    DropEvent(pDropEvent);
}
