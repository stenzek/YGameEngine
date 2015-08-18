/********************************************************************************
** Form generated from reading UI file 'EditorProgressDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITORPROGRESSDIALOG_H
#define UI_EDITORPROGRESSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditorProgressDialog
{
public:
    QGroupBox *m_statusGroupBox;
    QLabel *m_status;
    QProgressBar *m_progressBar;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_cancelButton;

    void setupUi(QDialog *EditorProgressDialog)
    {
        if (EditorProgressDialog->objectName().isEmpty())
            EditorProgressDialog->setObjectName(QStringLiteral("EditorProgressDialog"));
        EditorProgressDialog->resize(660, 170);
        EditorProgressDialog->setModal(true);
        m_statusGroupBox = new QGroupBox(EditorProgressDialog);
        m_statusGroupBox->setObjectName(QStringLiteral("m_statusGroupBox"));
        m_statusGroupBox->setGeometry(QRect(10, 10, 641, 71));
        m_status = new QLabel(m_statusGroupBox);
        m_status->setObjectName(QStringLiteral("m_status"));
        m_status->setGeometry(QRect(10, 20, 621, 41));
        m_status->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        m_progressBar = new QProgressBar(EditorProgressDialog);
        m_progressBar->setObjectName(QStringLiteral("m_progressBar"));
        m_progressBar->setGeometry(QRect(10, 90, 641, 31));
        m_progressBar->setValue(24);
        horizontalLayoutWidget = new QWidget(EditorProgressDialog);
        horizontalLayoutWidget->setObjectName(QStringLiteral("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(10, 130, 641, 31));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_cancelButton = new QPushButton(horizontalLayoutWidget);
        m_cancelButton->setObjectName(QStringLiteral("m_cancelButton"));

        horizontalLayout->addWidget(m_cancelButton);


        retranslateUi(EditorProgressDialog);

        QMetaObject::connectSlotsByName(EditorProgressDialog);
    } // setupUi

    void retranslateUi(QDialog *EditorProgressDialog)
    {
        EditorProgressDialog->setWindowTitle(QApplication::translate("EditorProgressDialog", "Action In Progress...", 0));
        m_statusGroupBox->setTitle(QApplication::translate("EditorProgressDialog", "Status", 0));
        m_status->setText(QApplication::translate("EditorProgressDialog", "Current Status", 0));
        m_cancelButton->setText(QApplication::translate("EditorProgressDialog", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class EditorProgressDialog: public Ui_EditorProgressDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITORPROGRESSDIALOG_H
