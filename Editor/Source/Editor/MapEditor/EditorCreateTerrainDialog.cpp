#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorCreateTerrainDialog.h"
#include "Editor/EditorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/ResourceManager.h"

EditorCreateTerrainDialog::EditorCreateTerrainDialog(QWidget *pParent, Qt::WindowFlags windowFlags /*= 0*/)
    : QDialog(pParent, windowFlags),
      m_ui(new Ui_EditorCreateTerrainDialog()),
      m_pLayerList(NULL),
      m_sectionSize(0),
      m_unitsPerPoint(0),
      m_minHeight(0),
      m_maxHeight(0),
      m_baseHeight(0),
      m_createCenterSection(false)
{
    m_ui->setupUi(this);

    // set state up
    m_ui->layerListSelection->setResourceTypeInfo(OBJECT_TYPEINFO(TerrainLayerList));
    m_ui->sectionSize->setRange(32, 4096);
    m_ui->sectionSize->setSingleStep(16);
    m_ui->unitsPerPoint->setRange(1, 64);
    m_ui->patchSize->setRange(4, 1024);
    m_ui->heightFormat->addItem("8-Bit Integer");
    m_ui->heightFormat->addItem("16-Bit Integer");
    m_ui->heightFormat->addItem("32-Bit Float");

    // set some defaults
    //m_ui->layerListSelection->setValue(ConvertStringToQString(g_pEngine->GetDefaultTerrainLayerListName()));
    m_ui->sectionSize->setValue(1024);
    m_ui->unitsPerPoint->setValue(4);
    m_ui->patchSize->setValue(64);
    m_ui->heightFormat->setCurrentIndex(2);
    m_ui->minimumHeight->setValue(-16384);
    m_ui->maximumHeight->setValue(16384);
    m_ui->textureRepeatInterval->setValue(16);
    m_ui->createCenterSectionCheckBox->setChecked(true);
    OnHeightFormatChanged(2);

    // connect events
    connect(m_ui->layerListSelection, SIGNAL(valueChanged(const QString &)), this, SLOT(OnLayerListChanged(const QString &)));
    connect(m_ui->heightFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(OnHeightFormatChanged(int)));
    connect(m_ui->createButton, SIGNAL(clicked()), this, SLOT(OnCreateButtonTriggered()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(OnCancelButtonTriggered()));
}

EditorCreateTerrainDialog::~EditorCreateTerrainDialog()
{
    if (m_pLayerList != NULL)
        m_pLayerList->Release();
}

bool EditorCreateTerrainDialog::Validate()
{
    if (m_pLayerList != NULL)
        m_pLayerList->Release();

    // todo: update tick buttons along the way
    m_pLayerList = g_pResourceManager->GetTerrainLayerList(ConvertQStringToString(m_ui->layerListSelection->getValue()));
    if (m_pLayerList == NULL)
    {
        m_ui->createButton->setEnabled(false);
        return false;
    }

    m_sectionSize = (uint32)m_ui->sectionSize->value();
    m_unitsPerPoint = (uint32)m_ui->unitsPerPoint->value();
    m_quadTreeNodeSize = (uint32)m_ui->patchSize->value();
    m_textureRepeatInterval = (uint32)m_ui->textureRepeatInterval->value();
    if (/*!TerrainManager::IsValidSectionSize(m_sectionSize, m_quadTreeNodeSize) ||*/
        (m_quadTreeNodeSize % m_textureRepeatInterval) != 0)
    {
        m_ui->createButton->setEnabled(false);
        return false;
    }

    m_storageFormat = (TERRAIN_HEIGHT_STORAGE_FORMAT)m_ui->heightFormat->currentIndex();
    m_minHeight = (int32)m_ui->minimumHeight->value();
    m_maxHeight = (int32)m_ui->maximumHeight->value();
    m_baseHeight = (int32)m_ui->baseHeight->value();
    m_createCenterSection = m_ui->createCenterSectionCheckBox->isChecked();

    m_ui->createButton->setEnabled(true);
    return true;
}

void EditorCreateTerrainDialog::OnLayerListChanged(const QString &value)
{
    Validate();
}

void EditorCreateTerrainDialog::OnHeightFormatChanged(int currentIndex)
{
    static const int32 heightRanges[3][2] = 
    {
        { -0x7E, 0x7F },
        { -0x7FFE, 0x7FFF },
        { -0x7FFFF, 0x7FFFF },
    };

    DebugAssert(currentIndex < countof(heightRanges));
    
    int32 minHeight = heightRanges[currentIndex][0];
    int32 maxHeight = heightRanges[currentIndex][1];
    m_ui->minimumHeight->setRange(minHeight, maxHeight);
    m_ui->maximumHeight->setRange(minHeight, maxHeight);
    m_ui->baseHeight->setRange(minHeight, maxHeight);

    if (currentIndex == 2)
    {
        // float
        m_ui->minimumHeight->setValue(minHeight);
        m_ui->minimumHeight->setEnabled(false);
        m_ui->maximumHeight->setValue(maxHeight);
        m_ui->maximumHeight->setEnabled(false);
    }
    else
    {
        // int
        m_ui->minimumHeight->setEnabled(true);
        m_ui->maximumHeight->setEnabled(true);
    }

    Validate();
}

void EditorCreateTerrainDialog::OnCreateButtonTriggered()
{
    if (!Validate())
        return;

    done(1);
}

void EditorCreateTerrainDialog::OnCancelButtonTriggered()
{
    done(0);
}
