#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorVectorEditWidget.h"
#include "Editor/EditorHelpers.h"

EditorVectorEditWidget::EditorVectorEditWidget(QWidget *pParent /*= NULL*/)
    : QWidget(pParent),
      m_numComponents(4),
      m_x(0.0f), m_y(0.0f), m_z(0.0f), m_w(0.0f)
{
    m_pLineEdit = new QLineEdit(this);
    m_pLineEdit->setDisabled(true);

    m_pBrowseButton = new QPushButton(this);
    m_pBrowseButton->setText(tr("..."));
    m_pBrowseButton->setMinimumSize(16, 16);
    m_pBrowseButton->setMaximumSize(20, 16777215);
    
    m_pPopupPanel = nullptr;

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_pLineEdit, 1);
    hbox->addWidget(m_pBrowseButton, 0);
    setLayout(hbox);

    connect(m_pBrowseButton, SIGNAL(clicked()), this, SLOT(OnExpandButtonClicked()));

    UpdateLineEdit();
}

EditorVectorEditWidget::~EditorVectorEditWidget()
{
    delete m_pPopupPanel;
}

QString EditorVectorEditWidget::GetValueString() const
{
    QString valueString;
    DebugAssert(m_numComponents > 0 && m_numComponents <= 4);

    switch (m_numComponents)
    {
    case 1: valueString = ConvertStringToQString(StringConverter::FloatToString(m_x));                          break;
    case 2: valueString = ConvertStringToQString(StringConverter::Float2ToString(float2(m_x, m_y)));            break;
    case 3: valueString = ConvertStringToQString(StringConverter::Float3ToString(float3(m_x, m_y, m_z)));       break;
    case 4: valueString = ConvertStringToQString(StringConverter::Float4ToString(float4(m_x, m_y, m_z, m_w)));  break;
    }

    return valueString;
}

void EditorVectorEditWidget::SetNumComponents(uint32 numComponents)
{
    DebugAssert(numComponents > 0 && numComponents <= 4);
    m_numComponents = numComponents;
}

void EditorVectorEditWidget::SetValue(float x /*= 0.0f*/, float y /*= 0.0f*/, float z /*= 0.0f*/, float w /*= 0.0f*/)
{
    m_x = x;
    m_y = y;
    m_z = z;
    m_w = w;

    UpdateLineEdit();

    // lazy way to force it to update
    if (m_pPopupPanel != nullptr && m_pPopupPanel->isVisible())
        OnExpandButtonClicked();
}

void EditorVectorEditWidget::UpdateLineEdit()
{
    m_pLineEdit->setText(GetValueString());
}

void EditorVectorEditWidget::EmitValueChangedSignals()
{
    ValueChangedFloat2(float2(m_x, m_y));
    ValueChangedFloat3(float3(m_x, m_y, m_z));
    ValueChangedFloat4(float4(m_x, m_y, m_z, m_w));
    ValueChangedString(GetValueString());
}

void EditorVectorEditWidget::OnExpandButtonClicked()
{
    if (m_pPopupPanel != nullptr)
        delete m_pPopupPanel;
    // create the editor panel
    QWidget *pPopupPanel = new QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint);
    pPopupPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pPopupPanel->setFixedSize(200, 34 + m_numComponents * 30);

    // create form layout
    QFormLayout *formLayout = new QFormLayout(pPopupPanel);
    formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    QDoubleSpinBox *pSpinBox;
    QDoubleSpinBox *pFocusSpinBox;

    // create each component

    // x
    pSpinBox = new QDoubleSpinBox(pPopupPanel);
    pSpinBox->setRange(Y_FLT_MIN, Y_FLT_MAX);
    pSpinBox->setDecimals(7);
    pSpinBox->setSingleStep(0.1);
    pSpinBox->setValue(m_x);
    connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnPopupPanelXChanged(double)));
    formLayout->addRow("&X: ", pSpinBox);
    pFocusSpinBox = pSpinBox;

    // y
    if (m_numComponents > 1)
    {
        pSpinBox = new QDoubleSpinBox(pPopupPanel);
        pSpinBox->setRange(Y_FLT_MIN, Y_FLT_MAX);
        pSpinBox->setDecimals(7);
        pSpinBox->setSingleStep(0.1);
        pSpinBox->setValue(m_y);
        connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnPopupPanelYChanged(double)));
        formLayout->addRow("&Y: ", pSpinBox);

        // z
        if (m_numComponents > 2)
        {
            pSpinBox = new QDoubleSpinBox(pPopupPanel);
            pSpinBox->setRange(Y_FLT_MIN, Y_FLT_MAX);
            pSpinBox->setDecimals(7);
            pSpinBox->setSingleStep(0.1);
            pSpinBox->setValue(m_z);
            connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnPopupPanelZChanged(double)));
            formLayout->addRow("&Z: ", pSpinBox);

            // w
            if (m_numComponents > 3)
            {
                pSpinBox = new QDoubleSpinBox(pPopupPanel);
                pSpinBox->setRange(Y_FLT_MIN, Y_FLT_MAX);
                pSpinBox->setDecimals(7);
                pSpinBox->setSingleStep(0.1);
                pSpinBox->setValue(m_w);
                connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnPopupPanelWChanged(double)));
                formLayout->addRow("&W: ", pSpinBox);
            }
        }
    }

    // normalize button
    QPushButton *pNormalizeButton = new QPushButton(pPopupPanel);
    pNormalizeButton->setText(tr("Normalize"));
    connect(pNormalizeButton, SIGNAL(clicked()), this, SLOT(OnPopupPanelNormalizeClicked()));
    formLayout->addRow(pNormalizeButton);

    // set layout
    pPopupPanel->setLayout(formLayout);

    // set pointer
    m_pPopupPanel = pPopupPanel;

    // move it into position and show
    m_pPopupPanel->move(m_pBrowseButton->mapToGlobal(QPoint(0, m_pBrowseButton->height())));
    m_pPopupPanel->show();

    // foxus it
    pFocusSpinBox->setFocus();
}

void EditorVectorEditWidget::OnPopupPanelXChanged(double value)
{
    m_x = (float)value;
    UpdateLineEdit();
    EmitValueChangedSignals();
}

void EditorVectorEditWidget::OnPopupPanelYChanged(double value)
{
    m_y = (float)value;
    UpdateLineEdit();
    EmitValueChangedSignals();
}

void EditorVectorEditWidget::OnPopupPanelZChanged(double value)
{
    m_z = (float)value;
    UpdateLineEdit();
    EmitValueChangedSignals();
}

void EditorVectorEditWidget::OnPopupPanelWChanged(double value)
{
    m_w = (float)value;
    UpdateLineEdit();
    EmitValueChangedSignals();
}

void EditorVectorEditWidget::OnPopupPanelNormalizeClicked()
{
    float length = m_x * m_x;

    if (m_numComponents > 1)
    {
        length += m_y * m_y;
        
        if (m_numComponents > 2)
        {
            length += m_z * m_z;

            if (m_numComponents > 3)
                length += m_w * m_w;
        }
    }

    if (Math::NearEqual(length, 0.0f, Y_FLT_EPSILON))
        return;

    length = Math::Sqrt(length);
    m_x /= length;
    m_y /= length;
    m_z /= length;
    m_w /= length;

    UpdateLineEdit();
    EmitValueChangedSignals();

    // lazy way to force it to update
    if (m_pPopupPanel != nullptr && m_pPopupPanel->isVisible())
        OnExpandButtonClicked();
}

