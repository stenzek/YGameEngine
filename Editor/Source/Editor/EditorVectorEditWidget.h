#pragma once
#include "Editor/Common.h"

class EditorVectorEditWidget : public QWidget
{
    Q_OBJECT

public:
    EditorVectorEditWidget(QWidget *pParent = NULL);
    ~EditorVectorEditWidget();

    const uint32 GetNumComponents() const { return m_numComponents; }
    const float GetValueX() const { return m_x; }
    const float GetValueY() const { return m_y; }
    const float GetValueZ() const { return m_z; }
    const float GetValueW() const { return m_w; }
    const float2 GetValueFloat2() const { return float2(m_x, m_y); }
    const float3 GetValueFloat3() const { return float3(m_x, m_y, m_z); }
    const float4 GetValueFloat4() const { return float4(m_x, m_y, m_z, m_w); }
    QString GetValueString() const;

public Q_SLOTS:
    void SetNumComponents(uint32 numComponents);
    void SetValue(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f);

Q_SIGNALS:
    void ValueChangedFloat2(const float2 &value);
    void ValueChangedFloat3(const float3 &value);
    void ValueChangedFloat4(const float4 &value);
    void ValueChangedString(const QString &value);

private Q_SLOTS:
    void OnExpandButtonClicked();
    void OnPopupPanelXChanged(double value);
    void OnPopupPanelYChanged(double value);
    void OnPopupPanelZChanged(double value);
    void OnPopupPanelWChanged(double value);
    void OnPopupPanelNormalizeClicked();

private:
    void UpdateLineEdit();
    void EmitValueChangedSignals();

    uint32 m_numComponents;
    float m_x, m_y, m_z, m_w;

    QLineEdit *m_pLineEdit;
    QPushButton *m_pBrowseButton;
    QWidget *m_pPopupPanel;
};
