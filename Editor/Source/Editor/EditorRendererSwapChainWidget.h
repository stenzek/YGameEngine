#pragma once
#include "Editor/Common.h"

class EditorRendererSwapChainWidget : public QWidget
{
    Q_OBJECT

public:
    EditorRendererSwapChainWidget(QWidget *pParent, Qt::WindowFlags flags = 0);
    ~EditorRendererSwapChainWidget();

    GPUOutputBuffer *GetSwapChain() const;
    void DestroySwapChain();

protected:
    virtual QPaintEngine *paintEngine() const;
    virtual void resizeEvent(QResizeEvent *pEvent);
    virtual void paintEvent(QPaintEvent *pEvent);
    virtual void focusInEvent(QFocusEvent *pEvent);
    virtual void focusOutEvent(QFocusEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pKeyEvent);
    virtual void keyReleaseEvent(QKeyEvent *pKeyEvent);
    virtual void mouseMoveEvent(QMouseEvent *pMouseEvent);
    virtual void mousePressEvent(QMouseEvent *pMouseEvent);
    virtual void mouseReleaseEvent(QMouseEvent *pMouseEvent);
    virtual void wheelEvent(QWheelEvent *pWheelEvent);
    virtual void dropEvent(QDropEvent *pDropEvent);

Q_SIGNALS:
    void ResizedEvent();
    void PaintEvent();
    void GainedFocusEvent();
    void LostFocusEvent();
    void KeyboardEvent(const QKeyEvent *pKeyboardEvent);
    void MouseEvent(const QMouseEvent *pMouseEvent);
    void WheelEvent(const QWheelEvent *pWheelEvent);
    void DropEvent(QDropEvent *pDropEvent);

private:
    void RecreateSwapChain();

    GPUOutputBuffer *m_pSwapChain;
};

