#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorResourceSelectionWidget.h"
#include "Editor/EditorResourceSelectionDialog.h"
#include "Editor/EditorHelpers.h"

EditorResourceSelectionWidget::EditorResourceSelectionWidget(QWidget *pParent /*= NULL*/)
    : QWidget(pParent)
{
    m_pLineEdit = new QLineEdit(this);
    m_pBrowseButton = new QPushButton(this);
    m_pBrowseButton->setText(tr("..."));
    m_pBrowseButton->setMinimumSize(16, 16);
    m_pBrowseButton->setMaximumSize(20, 16777215);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_pLineEdit, 1);
    hbox->addWidget(m_pBrowseButton, 0);
    setLayout(hbox);

    connect(m_pLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(onLineEditEdited(const QString &)));
    connect(m_pBrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseButtonClicked()));
}

EditorResourceSelectionWidget::~EditorResourceSelectionWidget()
{

}

void EditorResourceSelectionWidget::setResourceTypeInfo(const ResourceTypeInfo *pResourceTypeInfo)
{
    m_pResourceTypeInfo = pResourceTypeInfo;
}

void EditorResourceSelectionWidget::setValue(const QString &value)
{
    m_pLineEdit->setText(value);
}

void EditorResourceSelectionWidget::onBrowseButtonClicked()
{
    EditorResourceSelectionDialog selectionDialog(this);
    if (m_pResourceTypeInfo != NULL)
        selectionDialog.AddResourceFilter(m_pResourceTypeInfo);

    // find the directory of this resource
    SmallString currentValue;
    ConvertQStringToString(currentValue, m_pLineEdit->text());
    int32 lastSlash = currentValue.RFind('/');
    if (lastSlash >= 0)
    {
        currentValue.Erase(lastSlash);
        if (!selectionDialog.NavigateToDirectory(currentValue, false))
            selectionDialog.NavigateToDirectory("/", false);
    }

    selectionDialog.exec();

    if (selectionDialog.GetReturnValueResourceType() != NULL)
    {
        QString value(ConvertStringToQString(selectionDialog.GetReturnValueResourceName()));
        m_pLineEdit->setText(value);
        valueChanged(value);
    }
}

void EditorResourceSelectionWidget::onLineEditEdited(const QString &value)
{
    valueChanged(value);
}
