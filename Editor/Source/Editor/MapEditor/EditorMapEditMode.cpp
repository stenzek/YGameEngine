#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapEditMode.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/EditorPropertyEditorWidget.h"
#include "Editor/EditorHelpers.h"
#include "Editor/ToolMenuWidget.h"
#include "MapCompiler/MapSource.h"
#include "ResourceCompiler/ObjectTemplate.h"
Log_SetChannel(EditorMapEditMode);

struct ui_EditorMapEditMode
{
    QWidget *root;

    QAction *actionCheckMap;
    QAction *actionCompileMap;
    QAction *actionTestMap;

    QLabel *regionSize;
    QLabel *regionCount;
    QLabel *objectCount;

    QCheckBox *showSky;
    QCheckBox *showSun;

    QCheckBox *useTimeOfDay;
    QTimeEdit *timeOfDayStartTime;
    QTimeEdit *timeOfDayEndTime;
    QDoubleSpinBox *timeOfDayRate;

    void CreateUI(QWidget *parentWidget)
    {
        root = new QWidget(parentWidget);

        QVBoxLayout *rootLayout = new QVBoxLayout(root);
        rootLayout->setMargin(0);
        rootLayout->setSpacing(0);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        // actions
        {
            actionCheckMap = new QAction(root->tr("Check Map"), root);
            actionCheckMap->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
            actionCompileMap = new QAction(root->tr("Compile Map"), root);
            actionCompileMap->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
            actionTestMap = new QAction(root->tr("Test Map"), root);
            actionTestMap->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        }

        // fixed toolbar
        {
            ToolMenuWidget *fixedToolMenu = new ToolMenuWidget(root);
            fixedToolMenu->addAction(actionCheckMap);
            fixedToolMenu->addAction(actionCompileMap);
            fixedToolMenu->addAction(actionTestMap);
            rootLayout->addWidget(fixedToolMenu);
        }

        // scrollable area
        {
            QScrollArea *scrollArea = new QScrollArea(root);
            QWidget *scrollAreaWidget = new QWidget(scrollArea);
            scrollArea->setBackgroundRole(QPalette::Background);
            scrollArea->setFrameStyle(0);
            scrollArea->setWidgetResizable(true);
            scrollArea->setWidget(scrollAreaWidget);

            QVBoxLayout *scrollAreaLayout = new QVBoxLayout(scrollArea);
            scrollAreaLayout->setMargin(0);
            scrollAreaLayout->setSpacing(2);
            scrollAreaLayout->setContentsMargins(0, 0, 0, 0);

            // summary
            {
                QGroupBox *summaryBox = new QGroupBox(root->tr("Summary"), scrollAreaWidget);
                QFormLayout *summaryLayout = new QFormLayout(summaryBox);

                regionSize = new QLabel(summaryBox);
                summaryLayout->addRow(root->tr("Region Size: "), regionSize);

                regionCount = new QLabel(summaryBox);
                summaryLayout->addRow(root->tr("Region Count: "), regionCount);

                objectCount = new QLabel(summaryBox);
                summaryLayout->addRow(root->tr("Object Count: "), objectCount);

                summaryBox->setLayout(summaryLayout);
                scrollAreaLayout->addWidget(summaryBox);
            }

            // shows
            {
                QGroupBox *showBox = new QGroupBox(root->tr("Visibility"), scrollAreaWidget);
                QFormLayout *showLayout = new QFormLayout(showBox);

                showSky = new QCheckBox(root->tr("Sky"), showBox);
                showLayout->addRow(showSky);

                showSun = new QCheckBox(root->tr("Sun"), showBox);
                showLayout->addRow(showSun);

                showBox->setLayout(showLayout);
                scrollAreaLayout->addWidget(showBox);
            }

            // time of day
            {
                QGroupBox *timeOfDayBox = new QGroupBox(root->tr("Time of Day"), scrollAreaWidget);
                QFormLayout *timeOfDayLayout = new QFormLayout(timeOfDayBox);
                timeOfDayLayout->setLabelAlignment(Qt::AlignTop);
                
                useTimeOfDay = new QCheckBox(root->tr("Enable time of day"), timeOfDayBox);
                timeOfDayLayout->addRow(useTimeOfDay);

                timeOfDayStartTime = new QTimeEdit(timeOfDayBox);
                timeOfDayLayout->addRow(root->tr("Start Time: "), timeOfDayStartTime);

                timeOfDayEndTime = new QTimeEdit(timeOfDayBox);
                timeOfDayLayout->addRow(root->tr("End Time: "), timeOfDayEndTime);

                timeOfDayRate = new QDoubleSpinBox(timeOfDayBox);
                timeOfDayLayout->addRow(root->tr("Time Rate: "), timeOfDayRate);

                timeOfDayBox->setLayout(timeOfDayLayout);
                scrollAreaLayout->addWidget(timeOfDayBox);
            }
            
            // use remaining space
            scrollAreaLayout->addStretch(1);

            // finalize scroll area
            scrollAreaWidget->setLayout(scrollAreaLayout);
            rootLayout->addWidget(scrollArea, 1);
        }

        root->setLayout(rootLayout);
    }
};

QWidget *EditorMapEditMode::CreateUI(QWidget *pParentWidget)
{
    DebugAssert(m_ui == nullptr);
    m_ui = new ui_EditorMapEditMode();
    m_ui->CreateUI(pParentWidget);

    // bind events

    // update summary
    UpdateSummary();

    return m_ui->root;
}

void EditorMapEditMode::OnUIShowSkyChanged(bool checked)
{
}

void EditorMapEditMode::OnUIShowSunChanged(bool checked)
{
}

EditorMapEditMode::EditorMapEditMode(EditorMap *pMap)
    : EditorEditMode(pMap),
      m_ui(nullptr)
{
}

EditorMapEditMode::~EditorMapEditMode()
{
    delete m_ui;
}

void EditorMapEditMode::UpdateSummary()
{
    m_ui->regionSize->setText(ConvertStringToQString(TinyString::FromFormat("%u x %u units", m_pMap->GetMapSource()->GetRegionSize(), m_pMap->GetMapSource()->GetRegionSize())));
    m_ui->regionCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(0)));
    m_ui->objectCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pMap->GetMapSource()->GetEntityCount())));
}

void EditorMapEditMode::OnMapPropertyChanged(const char *propertyName, const char *propertyValue)
{

}

bool EditorMapEditMode::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    if (!EditorEditMode::Initialize(pProgressCallbacks))
        return false;

    return true;
}

void EditorMapEditMode::Activate()
{
    EditorEditMode::Activate();

    // initialize the property editor with the map properties
    EditorPropertyEditorWidget *propertyEditor = m_pMap->GetMapWindow()->GetPropertyEditorWidget();
    DebugAssert(m_pMap->GetMapSource()->GetObjectTemplate() != nullptr);
    propertyEditor->AddPropertiesFromTemplate(m_pMap->GetMapSource()->GetObjectTemplate()->GetPropertyTemplate());
    //propertyEditor->SetTitleText(m_pMap->GetMapSource()->GetObjectTemplate()->GetTypeName());
}

void EditorMapEditMode::Deactivate()
{
    EditorEditMode::Deactivate();
}

void EditorMapEditMode::Update(const float timeSinceLastUpdate)
{
    EditorEditMode::Update(timeSinceLastUpdate);
}

void EditorMapEditMode::OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue)
{
    // this must be a map property! update it in the map...
    m_pMap->SetMapProperty(propertyName, propertyValue);
}

void EditorMapEditMode::OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport)
{
    EditorEditMode::OnActiveViewportChanged(pOldActiveViewport, pNewActiveViewport);
}

bool EditorMapEditMode::HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent)
{
    return EditorEditMode::HandleViewportKeyboardInputEvent(pViewport, pKeyboardEvent);
}

bool EditorMapEditMode::HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent)
{
    return EditorEditMode::HandleViewportMouseInputEvent(pViewport, pMouseEvent);
}

bool EditorMapEditMode::HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent)
{
    return EditorEditMode::HandleViewportWheelInputEvent(pViewport, pWheelEvent);
}

void EditorMapEditMode::OnViewportDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterWorld(pViewport);
}

void EditorMapEditMode::OnViewportDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawBeforeWorld(pViewport);
}

void EditorMapEditMode::OnViewportDrawAfterPost(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterPost(pViewport);
}

void EditorMapEditMode::OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawAfterWorld(pViewport);
}

void EditorMapEditMode::OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawBeforeWorld(pViewport);
}
