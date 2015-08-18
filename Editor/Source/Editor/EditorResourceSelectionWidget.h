#pragma once
#include "Editor/Common.h"

class EditorResourceSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    EditorResourceSelectionWidget(QWidget *pParent = NULL);
    ~EditorResourceSelectionWidget();

    const ResourceTypeInfo *getResourceTypeInfo() const { return m_pResourceTypeInfo; }
    const QString getValue() const { return m_pLineEdit->text(); }

public Q_SLOTS:
    void setResourceTypeInfo(const ResourceTypeInfo *pResourceTypeInfo);
    void setValue(const QString &value);

Q_SIGNALS:
    void valueChanged(const QString &value);

private Q_SLOTS:
    void onBrowseButtonClicked();
    void onLineEditEdited(const QString &value);

private:
    const ResourceTypeInfo *m_pResourceTypeInfo;

    QLineEdit *m_pLineEdit;
    QPushButton *m_pBrowseButton;
    QWidget *m_pEditorArea;
};
