/********************************************************************************
** Form generated from reading UI file 'EditorStaticMeshImportDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITORSTATICMESHIMPORTDIALOG_H
#define UI_EDITORSTATICMESHIMPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "Editor/EditorVectorEditWidget.h"

QT_BEGIN_NAMESPACE

class Ui_EditorStaticMeshImportDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *mainHeading;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *fileNameLabel;
    QHBoxLayout *horizontalLayout;
    QLineEdit *fileNameLineEdit;
    QPushButton *fileNameBrowseButton;
    QLabel *subObjectLabel;
    QComboBox *subObjectComboBox;
    QLabel *coordinateSystemLabel;
    QComboBox *coordinateSystemComboBox;
    QLabel *flipWindingLabel;
    QCheckBox *flipWindingCheckBox;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout_2;
    QLabel *generateTangentsLabel;
    QCheckBox *generateTangentsCheckBox;
    QLabel *importMaterialsLabel;
    QCheckBox *importMaterialsCheckBox;
    QLabel *materialPrefixLabel;
    QLineEdit *materialPrefixLineEdit;
    QLabel *scaleLabel;
    EditorVectorEditWidget *scaleVectorEdit;
    QLabel *materialDirectoryLabel;
    QLineEdit *materialDirectoryLineEdit;
    QLabel *collisionShapeTypeLabel;
    QComboBox *collisionShapeTypeComboBox;
    QLabel *importerMessagesLabel;
    QListWidget *importerMessagesListWidget;
    QProgressBar *progressBar;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *keepOpenAfterImportingCheckBox;
    QSpacerItem *horizontalSpacer;
    QPushButton *importButton;
    QPushButton *closeButton;

    void setupUi(QDialog *EditorStaticMeshImportDialog)
    {
        if (EditorStaticMeshImportDialog->objectName().isEmpty())
            EditorStaticMeshImportDialog->setObjectName(QStringLiteral("EditorStaticMeshImportDialog"));
        EditorStaticMeshImportDialog->resize(476, 605);
        EditorStaticMeshImportDialog->setSizeGripEnabled(true);
        verticalLayout = new QVBoxLayout(EditorStaticMeshImportDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        mainHeading = new QLabel(EditorStaticMeshImportDialog);
        mainHeading->setObjectName(QStringLiteral("mainHeading"));
        mainHeading->setTextFormat(Qt::RichText);
        mainHeading->setWordWrap(true);

        verticalLayout->addWidget(mainHeading);

        groupBox = new QGroupBox(EditorStaticMeshImportDialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        fileNameLabel = new QLabel(groupBox);
        fileNameLabel->setObjectName(QStringLiteral("fileNameLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, fileNameLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        fileNameLineEdit = new QLineEdit(groupBox);
        fileNameLineEdit->setObjectName(QStringLiteral("fileNameLineEdit"));

        horizontalLayout->addWidget(fileNameLineEdit);

        fileNameBrowseButton = new QPushButton(groupBox);
        fileNameBrowseButton->setObjectName(QStringLiteral("fileNameBrowseButton"));

        horizontalLayout->addWidget(fileNameBrowseButton);


        formLayout->setLayout(0, QFormLayout::FieldRole, horizontalLayout);

        subObjectLabel = new QLabel(groupBox);
        subObjectLabel->setObjectName(QStringLiteral("subObjectLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, subObjectLabel);

        subObjectComboBox = new QComboBox(groupBox);
        subObjectComboBox->setObjectName(QStringLiteral("subObjectComboBox"));

        formLayout->setWidget(1, QFormLayout::FieldRole, subObjectComboBox);

        coordinateSystemLabel = new QLabel(groupBox);
        coordinateSystemLabel->setObjectName(QStringLiteral("coordinateSystemLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, coordinateSystemLabel);

        coordinateSystemComboBox = new QComboBox(groupBox);
        coordinateSystemComboBox->setObjectName(QStringLiteral("coordinateSystemComboBox"));

        formLayout->setWidget(2, QFormLayout::FieldRole, coordinateSystemComboBox);

        flipWindingLabel = new QLabel(groupBox);
        flipWindingLabel->setObjectName(QStringLiteral("flipWindingLabel"));

        formLayout->setWidget(3, QFormLayout::LabelRole, flipWindingLabel);

        flipWindingCheckBox = new QCheckBox(groupBox);
        flipWindingCheckBox->setObjectName(QStringLiteral("flipWindingCheckBox"));

        formLayout->setWidget(3, QFormLayout::FieldRole, flipWindingCheckBox);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(EditorStaticMeshImportDialog);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        formLayout_2 = new QFormLayout(groupBox_2);
        formLayout_2->setObjectName(QStringLiteral("formLayout_2"));
        formLayout_2->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        generateTangentsLabel = new QLabel(groupBox_2);
        generateTangentsLabel->setObjectName(QStringLiteral("generateTangentsLabel"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, generateTangentsLabel);

        generateTangentsCheckBox = new QCheckBox(groupBox_2);
        generateTangentsCheckBox->setObjectName(QStringLiteral("generateTangentsCheckBox"));
        generateTangentsCheckBox->setChecked(true);

        formLayout_2->setWidget(0, QFormLayout::FieldRole, generateTangentsCheckBox);

        importMaterialsLabel = new QLabel(groupBox_2);
        importMaterialsLabel->setObjectName(QStringLiteral("importMaterialsLabel"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, importMaterialsLabel);

        importMaterialsCheckBox = new QCheckBox(groupBox_2);
        importMaterialsCheckBox->setObjectName(QStringLiteral("importMaterialsCheckBox"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, importMaterialsCheckBox);

        materialPrefixLabel = new QLabel(groupBox_2);
        materialPrefixLabel->setObjectName(QStringLiteral("materialPrefixLabel"));

        formLayout_2->setWidget(3, QFormLayout::LabelRole, materialPrefixLabel);

        materialPrefixLineEdit = new QLineEdit(groupBox_2);
        materialPrefixLineEdit->setObjectName(QStringLiteral("materialPrefixLineEdit"));

        formLayout_2->setWidget(3, QFormLayout::FieldRole, materialPrefixLineEdit);

        scaleLabel = new QLabel(groupBox_2);
        scaleLabel->setObjectName(QStringLiteral("scaleLabel"));

        formLayout_2->setWidget(4, QFormLayout::LabelRole, scaleLabel);

        scaleVectorEdit = new EditorVectorEditWidget(groupBox_2);
        scaleVectorEdit->setObjectName(QStringLiteral("scaleVectorEdit"));

        formLayout_2->setWidget(4, QFormLayout::FieldRole, scaleVectorEdit);

        materialDirectoryLabel = new QLabel(groupBox_2);
        materialDirectoryLabel->setObjectName(QStringLiteral("materialDirectoryLabel"));

        formLayout_2->setWidget(2, QFormLayout::LabelRole, materialDirectoryLabel);

        materialDirectoryLineEdit = new QLineEdit(groupBox_2);
        materialDirectoryLineEdit->setObjectName(QStringLiteral("materialDirectoryLineEdit"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, materialDirectoryLineEdit);

        collisionShapeTypeLabel = new QLabel(groupBox_2);
        collisionShapeTypeLabel->setObjectName(QStringLiteral("collisionShapeTypeLabel"));

        formLayout_2->setWidget(5, QFormLayout::LabelRole, collisionShapeTypeLabel);

        collisionShapeTypeComboBox = new QComboBox(groupBox_2);
        collisionShapeTypeComboBox->setObjectName(QStringLiteral("collisionShapeTypeComboBox"));

        formLayout_2->setWidget(5, QFormLayout::FieldRole, collisionShapeTypeComboBox);


        verticalLayout->addWidget(groupBox_2);

        importerMessagesLabel = new QLabel(EditorStaticMeshImportDialog);
        importerMessagesLabel->setObjectName(QStringLiteral("importerMessagesLabel"));

        verticalLayout->addWidget(importerMessagesLabel);

        importerMessagesListWidget = new QListWidget(EditorStaticMeshImportDialog);
        importerMessagesListWidget->setObjectName(QStringLiteral("importerMessagesListWidget"));

        verticalLayout->addWidget(importerMessagesListWidget);

        progressBar = new QProgressBar(EditorStaticMeshImportDialog);
        progressBar->setObjectName(QStringLiteral("progressBar"));
        progressBar->setValue(0);
        progressBar->setTextVisible(false);

        verticalLayout->addWidget(progressBar);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        keepOpenAfterImportingCheckBox = new QCheckBox(EditorStaticMeshImportDialog);
        keepOpenAfterImportingCheckBox->setObjectName(QStringLiteral("keepOpenAfterImportingCheckBox"));

        horizontalLayout_2->addWidget(keepOpenAfterImportingCheckBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        importButton = new QPushButton(EditorStaticMeshImportDialog);
        importButton->setObjectName(QStringLiteral("importButton"));
        importButton->setDefault(true);
        importButton->setFlat(false);

        horizontalLayout_2->addWidget(importButton);

        closeButton = new QPushButton(EditorStaticMeshImportDialog);
        closeButton->setObjectName(QStringLiteral("closeButton"));

        horizontalLayout_2->addWidget(closeButton);


        verticalLayout->addLayout(horizontalLayout_2);


        retranslateUi(EditorStaticMeshImportDialog);

        coordinateSystemComboBox->setCurrentIndex(1);
        collisionShapeTypeComboBox->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(EditorStaticMeshImportDialog);
    } // setupUi

    void retranslateUi(QDialog *EditorStaticMeshImportDialog)
    {
        EditorStaticMeshImportDialog->setWindowTitle(QApplication::translate("EditorStaticMeshImportDialog", "Static Mesh Importer", 0));
        mainHeading->setText(QApplication::translate("EditorStaticMeshImportDialog", "<html><head/><body><p><span style=\" font-size:16pt; font-weight:600;\">Static Mesh Importer</span></p><p>Fill out this form to import an object, or multiple objects into the engine. After importing is complete, an editor window will open with the imported mesh.</p></body></html>", 0));
        groupBox->setTitle(QApplication::translate("EditorStaticMeshImportDialog", "Source File", 0));
        fileNameLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "File Name: ", 0));
        fileNameBrowseButton->setText(QApplication::translate("EditorStaticMeshImportDialog", "Browse...", 0));
        subObjectLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Sub Object: ", 0));
        coordinateSystemLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Coordinate System: ", 0));
        coordinateSystemComboBox->clear();
        coordinateSystemComboBox->insertItems(0, QStringList()
         << QApplication::translate("EditorStaticMeshImportDialog", "Y-Up Left Handed", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Y-Up Right Handed", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Z-Up Left Handed", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Z-Up Right Handed", 0)
        );
        flipWindingLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Flip Faces: ", 0));
        groupBox_2->setTitle(QApplication::translate("EditorStaticMeshImportDialog", "Post Processing Options", 0));
        generateTangentsLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Generate Tangents: ", 0));
        importMaterialsLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Import Materials: ", 0));
        materialPrefixLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Material Prefix: ", 0));
        scaleLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Scale: ", 0));
        materialDirectoryLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Material Directory: ", 0));
        collisionShapeTypeLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Collision Shape Type:  ", 0));
        collisionShapeTypeComboBox->clear();
        collisionShapeTypeComboBox->insertItems(0, QStringList()
         << QApplication::translate("EditorStaticMeshImportDialog", "None", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Box", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Sphere", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Triangle Mesh", 0)
         << QApplication::translate("EditorStaticMeshImportDialog", "Convex Hull", 0)
        );
        importerMessagesLabel->setText(QApplication::translate("EditorStaticMeshImportDialog", "Importer Messages: ", 0));
        keepOpenAfterImportingCheckBox->setText(QApplication::translate("EditorStaticMeshImportDialog", "Keep Open After Importing", 0));
        importButton->setText(QApplication::translate("EditorStaticMeshImportDialog", "Import", 0));
        closeButton->setText(QApplication::translate("EditorStaticMeshImportDialog", "Close", 0));
    } // retranslateUi

};

namespace Ui {
    class EditorStaticMeshImportDialog: public Ui_EditorStaticMeshImportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITORSTATICMESHIMPORTDIALOG_H
