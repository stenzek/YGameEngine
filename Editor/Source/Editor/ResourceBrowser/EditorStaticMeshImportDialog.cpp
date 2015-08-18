#include "Editor/PrecompiledHeader.h"
#include "Editor/ResourceBrowser/EditorStaticMeshImportDialog.h"
#include "Editor/ResourceBrowser/ui_EditorStaticMeshImportDialog.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
#include "ContentConverter/AssimpStaticMeshImporter.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
#include "Editor/StaticMeshEditor/EditorStaticMeshEditor.h"

EditorStaticMeshImportDialog::EditorStaticMeshImportDialog(QWidget *parent)
    : QDialog(parent),
      EditorProgressCallbacks(parent),
      m_ui(new Ui_EditorStaticMeshImportDialog())
{
    m_ui->setupUi(this);

    // minimum size
    setMinimumSize(100, 300);

    // fix up the ui
    m_ui->scaleVectorEdit->SetNumComponents(3);
    m_ui->scaleVectorEdit->SetValue(1.0f, 1.0f, 1.0f);
    ConnectUIEvents();
    UpdateUIState();
}

EditorStaticMeshImportDialog::~EditorStaticMeshImportDialog()
{
    delete m_ui;
}

void EditorStaticMeshImportDialog::SetMaterialDirectory(const QString &materialDirectory)
{
    m_ui->materialDirectoryLineEdit->setText(materialDirectory);
}

void EditorStaticMeshImportDialog::SetStatusText(const char *statusText)
{
    m_ui->importerMessagesLabel->setText(statusText);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::SetProgressRange(uint32 range)
{
    m_ui->progressBar->setRange(0, range);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::SetProgressValue(uint32 value)
{
    m_ui->progressBar->setValue(value);
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::DisplayError(const char *message)
{
    m_ui->importerMessagesListWidget->addItem(ConvertStringToQString(SmallString::FromFormat("[E] %s", message)));
    m_ui->importerMessagesListWidget->scrollToBottom();
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::DisplayWarning(const char *message)
{
    m_ui->importerMessagesListWidget->addItem(ConvertStringToQString(SmallString::FromFormat("[W] %s", message)));
    m_ui->importerMessagesListWidget->scrollToBottom();
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::DisplayInformation(const char *message)
{
    m_ui->importerMessagesListWidget->addItem(ConvertStringToQString(SmallString::FromFormat("[I] %s", message)));
    m_ui->importerMessagesListWidget->scrollToBottom();
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::DisplayDebugMessage(const char *message)
{
    m_ui->importerMessagesListWidget->addItem(ConvertStringToQString(SmallString::FromFormat("[D] %s", message)));
    m_ui->importerMessagesListWidget->scrollToBottom();
    g_pEditor->ProcessBackgroundEvents();
}

void EditorStaticMeshImportDialog::closeEvent(QCloseEvent *pCloseEvent)
{
    QDialog::closeEvent(pCloseEvent);
    deleteLater();
}

void EditorStaticMeshImportDialog::ConnectUIEvents()
{
    connect(m_ui->fileNameLineEdit, SIGNAL(editingFinished()), this, SLOT(OnFileNameLineEditEditingFinished()));
    connect(m_ui->fileNameBrowseButton, SIGNAL(clicked()), this, SLOT(OnFileNameBrowseClicked()));
    connect(m_ui->importButton, SIGNAL(clicked()), this, SLOT(OnImportButtonClicked()));
    connect(m_ui->closeButton, SIGNAL(clicked()), this, SLOT(OnCloseButtonClicked()));
}

void EditorStaticMeshImportDialog::UpdateUIState()
{
    String fileName(ConvertQStringToString(m_ui->fileNameLineEdit->text()));
    if (fileName.IsEmpty() || !FileSystem::FileExists(fileName))
    {
        m_ui->importButton->setEnabled(false);
        m_ui->subObjectComboBox->setEnabled(false);
        m_ui->subObjectComboBox->clear();
        return;
    }

    // try opening the file
    Array<String> subMeshNames;
    if (!AssimpStaticMeshImporter::GetFileSubMeshNames(fileName, subMeshNames, this))
    {
        m_ui->importButton->setEnabled(false);
        m_ui->subObjectComboBox->setEnabled(false);
        m_ui->subObjectComboBox->clear();
        return;
    }

    // add to combo box
    m_ui->subObjectComboBox->clear();
    m_ui->subObjectComboBox->addItem(tr("* All Objects"), QVariant(-1));
    for (uint32 i = 0; i < subMeshNames.GetSize(); i++)
        m_ui->subObjectComboBox->addItem(ConvertStringToQString(subMeshNames[i]), QVariant((int)i));

    // enable everything
    m_ui->subObjectComboBox->setEnabled(true);
    m_ui->importButton->setEnabled(true);
}

void EditorStaticMeshImportDialog::OnFileNameLineEditEditingFinished()
{
    UpdateUIState();
}

void EditorStaticMeshImportDialog::OnFileNameBrowseClicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select Mesh File..."),
                                                    QStringLiteral(""),
                                                    tr("Static Mesh Formats (*.obj *.fbx *.ase *.3ds *.bsp *.md3 *.dae *.blend)"),
                                                    NULL,
                                                    0);

    if (filename.isEmpty())
        return;

    m_ui->fileNameLineEdit->setText(filename);
    UpdateUIState();
}

void EditorStaticMeshImportDialog::OnImportButtonClicked()
{
    AssimpStaticMeshImporter::Options options;
    AssimpStaticMeshImporter::SetDefaultOptions(&options);
    options.SourcePath = ConvertQStringToString(m_ui->fileNameLineEdit->text());
    options.ImportMaterials = m_ui->importMaterialsCheckBox->isChecked();
    options.MaterialDirectory = ConvertQStringToString(m_ui->materialDirectoryLineEdit->text());
    options.MaterialPrefix = ConvertQStringToString(m_ui->materialPrefixLineEdit->text());
    options.DefaultMaterialName = g_pEngine->GetDefaultMaterialName();
    options.CoordinateSystem = (COORDINATE_SYSTEM)m_ui->coordinateSystemComboBox->currentIndex();
    options.FlipFaceOrder = m_ui->flipWindingCheckBox->isChecked();
    options.TransformMatrix = float4x4::MakeScaleMatrix(m_ui->scaleVectorEdit->GetValueFloat3());
    options.BuildCollisionShapeType = m_ui->collisionShapeTypeComboBox->currentIndex();

    // disable controls
    setEnabled(false);

    // create and run importer
    AssimpStaticMeshImporter *pImporter = new AssimpStaticMeshImporter(&options, this);
    if (!pImporter->Execute())
    {
        DisplayError("Import failed. The error log may contain more information.");
        delete pImporter;
        setEnabled(true);
        return;
    }

    // set to full progress
    m_ui->progressBar->setRange(0, 1);
    m_ui->progressBar->setValue(1);
    setEnabled(true);

    // valid?
    if (!pImporter->GetOutputGenerator()->IsCompleteMesh())
    {
        DisplayError("Import did not produce any usable meshes.");
        delete pImporter;
        return;
    }

    // spawn a static mesh editor
    EditorStaticMeshEditor *pStaticMeshEditor = new EditorStaticMeshEditor();
    if (!pStaticMeshEditor->Create(pImporter->GetOutputGenerator(), nullptr, this))
    {
        DisplayError("Failed to create editor.");
        delete pImporter;
        return;
    }

    pStaticMeshEditor->show();
    
    // and cleanup
    delete pImporter;

    // close us if requested
    if (!m_ui->keepOpenAfterImportingCheckBox->isChecked())
        OnCloseButtonClicked();
}

void EditorStaticMeshImportDialog::OnCloseButtonClicked()
{
    done(0);
}

