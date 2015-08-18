/********************************************************************************
** Form generated from reading UI file 'EditorCreateTerrainDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITORCREATETERRAINDIALOG_H
#define UI_EDITORCREATETERRAINDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include "Editor/EditorResourceSelectionWidget.h"

QT_BEGIN_NAMESPACE

class Ui_EditorCreateTerrainDialog
{
public:
    QGridLayout *gridLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *label_2;
    EditorResourceSelectionWidget *layerListSelection;
    QLabel *label_3;
    QSpinBox *sectionSize;
    QLabel *label_6;
    QSpinBox *unitsPerPoint;
    QLabel *label_9;
    QSpinBox *patchSize;
    QLabel *label_5;
    QComboBox *heightFormat;
    QLabel *label_7;
    QSpinBox *minimumHeight;
    QLabel *label_4;
    QSpinBox *maximumHeight;
    QLabel *label_8;
    QSpinBox *baseHeight;
    QSpinBox *textureRepeatInterval;
    QCheckBox *createCenterSectionCheckBox;
    QLabel *label_10;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *createButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *EditorCreateTerrainDialog)
    {
        if (EditorCreateTerrainDialog->objectName().isEmpty())
            EditorCreateTerrainDialog->setObjectName(QStringLiteral("EditorCreateTerrainDialog"));
        EditorCreateTerrainDialog->resize(433, 347);
        gridLayout = new QGridLayout(EditorCreateTerrainDialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label = new QLabel(EditorCreateTerrainDialog);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(0, QFormLayout::SpanningRole, label);

        label_2 = new QLabel(EditorCreateTerrainDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        layerListSelection = new EditorResourceSelectionWidget(EditorCreateTerrainDialog);
        layerListSelection->setObjectName(QStringLiteral("layerListSelection"));

        formLayout->setWidget(1, QFormLayout::FieldRole, layerListSelection);

        label_3 = new QLabel(EditorCreateTerrainDialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        sectionSize = new QSpinBox(EditorCreateTerrainDialog);
        sectionSize->setObjectName(QStringLiteral("sectionSize"));

        formLayout->setWidget(2, QFormLayout::FieldRole, sectionSize);

        label_6 = new QLabel(EditorCreateTerrainDialog);
        label_6->setObjectName(QStringLiteral("label_6"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_6);

        unitsPerPoint = new QSpinBox(EditorCreateTerrainDialog);
        unitsPerPoint->setObjectName(QStringLiteral("unitsPerPoint"));

        formLayout->setWidget(3, QFormLayout::FieldRole, unitsPerPoint);

        label_9 = new QLabel(EditorCreateTerrainDialog);
        label_9->setObjectName(QStringLiteral("label_9"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_9);

        patchSize = new QSpinBox(EditorCreateTerrainDialog);
        patchSize->setObjectName(QStringLiteral("patchSize"));

        formLayout->setWidget(4, QFormLayout::FieldRole, patchSize);

        label_5 = new QLabel(EditorCreateTerrainDialog);
        label_5->setObjectName(QStringLiteral("label_5"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_5);

        heightFormat = new QComboBox(EditorCreateTerrainDialog);
        heightFormat->setObjectName(QStringLiteral("heightFormat"));

        formLayout->setWidget(5, QFormLayout::FieldRole, heightFormat);

        label_7 = new QLabel(EditorCreateTerrainDialog);
        label_7->setObjectName(QStringLiteral("label_7"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_7);

        minimumHeight = new QSpinBox(EditorCreateTerrainDialog);
        minimumHeight->setObjectName(QStringLiteral("minimumHeight"));

        formLayout->setWidget(6, QFormLayout::FieldRole, minimumHeight);

        label_4 = new QLabel(EditorCreateTerrainDialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_4);

        maximumHeight = new QSpinBox(EditorCreateTerrainDialog);
        maximumHeight->setObjectName(QStringLiteral("maximumHeight"));

        formLayout->setWidget(7, QFormLayout::FieldRole, maximumHeight);

        label_8 = new QLabel(EditorCreateTerrainDialog);
        label_8->setObjectName(QStringLiteral("label_8"));

        formLayout->setWidget(8, QFormLayout::LabelRole, label_8);

        baseHeight = new QSpinBox(EditorCreateTerrainDialog);
        baseHeight->setObjectName(QStringLiteral("baseHeight"));

        formLayout->setWidget(8, QFormLayout::FieldRole, baseHeight);

        textureRepeatInterval = new QSpinBox(EditorCreateTerrainDialog);
        textureRepeatInterval->setObjectName(QStringLiteral("textureRepeatInterval"));

        formLayout->setWidget(9, QFormLayout::FieldRole, textureRepeatInterval);

        createCenterSectionCheckBox = new QCheckBox(EditorCreateTerrainDialog);
        createCenterSectionCheckBox->setObjectName(QStringLiteral("createCenterSectionCheckBox"));

        formLayout->setWidget(10, QFormLayout::FieldRole, createCenterSectionCheckBox);

        label_10 = new QLabel(EditorCreateTerrainDialog);
        label_10->setObjectName(QStringLiteral("label_10"));

        formLayout->setWidget(9, QFormLayout::LabelRole, label_10);


        gridLayout->addLayout(formLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        createButton = new QPushButton(EditorCreateTerrainDialog);
        createButton->setObjectName(QStringLiteral("createButton"));
        createButton->setDefault(true);

        horizontalLayout_2->addWidget(createButton);

        cancelButton = new QPushButton(EditorCreateTerrainDialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        horizontalLayout_2->addWidget(cancelButton);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        QWidget::setTabOrder(layerListSelection, sectionSize);
        QWidget::setTabOrder(sectionSize, unitsPerPoint);
        QWidget::setTabOrder(unitsPerPoint, patchSize);
        QWidget::setTabOrder(patchSize, heightFormat);
        QWidget::setTabOrder(heightFormat, minimumHeight);
        QWidget::setTabOrder(minimumHeight, maximumHeight);
        QWidget::setTabOrder(maximumHeight, baseHeight);
        QWidget::setTabOrder(baseHeight, textureRepeatInterval);
        QWidget::setTabOrder(textureRepeatInterval, createCenterSectionCheckBox);
        QWidget::setTabOrder(createCenterSectionCheckBox, createButton);
        QWidget::setTabOrder(createButton, cancelButton);

        retranslateUi(EditorCreateTerrainDialog);

        QMetaObject::connectSlotsByName(EditorCreateTerrainDialog);
    } // setupUi

    void retranslateUi(QDialog *EditorCreateTerrainDialog)
    {
        EditorCreateTerrainDialog->setWindowTitle(QApplication::translate("EditorCreateTerrainDialog", "Create Heightfield Terrain", 0));
        label->setText(QApplication::translate("EditorCreateTerrainDialog", "To create heightfield terrain for this map, provide the following details:", 0));
        label_2->setText(QApplication::translate("EditorCreateTerrainDialog", "Layer List: ", 0));
        label_3->setText(QApplication::translate("EditorCreateTerrainDialog", "Section Size: \n"
"(the size of each terrain texture)", 0));
        label_6->setText(QApplication::translate("EditorCreateTerrainDialog", "Resolution:\n"
"(world units per terrain point)", 0));
        label_9->setText(QApplication::translate("EditorCreateTerrainDialog", "Patch Size: \n"
"(minimum node size)", 0));
        label_5->setText(QApplication::translate("EditorCreateTerrainDialog", "Height Format: ", 0));
        label_7->setText(QApplication::translate("EditorCreateTerrainDialog", "Minimum Height: ", 0));
        label_4->setText(QApplication::translate("EditorCreateTerrainDialog", "Maximum Height: ", 0));
        label_8->setText(QApplication::translate("EditorCreateTerrainDialog", "Base Height:", 0));
        createCenterSectionCheckBox->setText(QApplication::translate("EditorCreateTerrainDialog", "Create Center Section", 0));
        label_10->setText(QApplication::translate("EditorCreateTerrainDialog", "Texture Repeat Interval:", 0));
        createButton->setText(QApplication::translate("EditorCreateTerrainDialog", "Create", 0));
        cancelButton->setText(QApplication::translate("EditorCreateTerrainDialog", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class EditorCreateTerrainDialog: public Ui_EditorCreateTerrainDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITORCREATETERRAINDIALOG_H
