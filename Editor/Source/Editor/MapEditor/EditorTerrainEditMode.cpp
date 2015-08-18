#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorTerrainEditMode.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorProgressDialog.h"
#include "Engine/Entity.h"
#include "Engine/StaticMesh.h"
#include "Engine/ResourceManager.h"
#include "Engine/Texture.h"
#include "Engine/EngineCVars.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/RenderProxies/CompositeRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "ResourceCompiler/TerrainLayerListGenerator.h"
Log_SetChannel(EditorTerrainEditMode);

// helper function to create the default layer list
TerrainLayerListGenerator *EditorTerrainEditMode::CreateEmptyLayerList()
{
    TerrainLayerListGenerator *pGenerator = new TerrainLayerListGenerator();
    pGenerator->Create(256, 256);

    // create the default layer
    const TerrainLayerListGenerator::BaseLayer *pBaseLayer = pGenerator->GetBaseLayerByName("default");
    DebugAssert(pBaseLayer != nullptr);

    // load in the default texture
    {
        Image baseMap;
        AutoReleasePtr<const Texture2D> pTexture = g_pResourceManager->GetTexture2D("resources/editor/textures/default_terrain_base_map");
        if (pTexture != nullptr && pTexture->ExportToImage(0, &baseMap))
        {
            baseMap.ConvertPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM);
            baseMap.Resize(IMAGE_RESIZE_FILTER_LANCZOS3, pGenerator->GetBaseLayerBaseMapResolution(), pGenerator->GetBaseLayerBaseMapResolution(), 1);
            pGenerator->SetBaseLayerBaseMap(pBaseLayer->Index, &baseMap);
        }
    }

    // just for testing util editor is done
#if 1
    pBaseLayer = pGenerator->CreateBaseLayer("grass");
    {
        Image baseMap;
        AutoReleasePtr<const Texture2D> pTexture = g_pResourceManager->GetTexture2D("textures/terrain/GrassGreenTexture0006");
        if (pTexture != nullptr && pTexture->ExportToImage(0, &baseMap))
        {
            baseMap.ConvertPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM);
            baseMap.Resize(IMAGE_RESIZE_FILTER_LANCZOS3, pGenerator->GetBaseLayerBaseMapResolution(), pGenerator->GetBaseLayerBaseMapResolution(), 1);
            pGenerator->SetBaseLayerBaseMap(pBaseLayer->Index, &baseMap);
        }
    }
    pBaseLayer = pGenerator->CreateBaseLayer("dirt");
    {
        Image baseMap;
        AutoReleasePtr<const Texture2D> pTexture = g_pResourceManager->GetTexture2D("textures/terrain/Dirt00seamless");
        if (pTexture != nullptr && pTexture->ExportToImage(0, &baseMap))
        {
            baseMap.ConvertPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM);
            baseMap.Resize(IMAGE_RESIZE_FILTER_LANCZOS3, pGenerator->GetBaseLayerBaseMapResolution(), pGenerator->GetBaseLayerBaseMapResolution(), 1);
            pGenerator->SetBaseLayerBaseMap(pBaseLayer->Index, &baseMap);
        }
    }
#endif

    // done
    return pGenerator;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ui_EditorTerrainEditMode
{
    QWidget *root;

    QWidget *buttonsPanel;
    QToolButton *buttonEditHeight;
    QToolButton *buttonEditDetails;
    QToolButton *buttonEditHoles;
    QToolButton *buttonEditSections;
    QToolButton *buttonEditLayers;
    QToolButton *buttonImportHeightmap;
    QToolButton *buttonSettings;

    QGroupBox *BrushPanel;
    QComboBox *BrushPanelBrushType;
    QDoubleSpinBox *BrushPanelBrushRadius;
    QDoubleSpinBox *BrushPanelBrushFalloff;
    QCheckBox *BrushPanelBrushInvert;

    QGroupBox *EditHeightPanel;
    QDoubleSpinBox *EditHeightPanelStepHeight;
    QGroupBox *EditLayersPanel;
    QDoubleSpinBox *EditLayersPanelStepWeight;
    QListWidget *EditLayersPanelLayerList;
    QPushButton *EditLayersPanelEditLayers;

    QWidget *EditDetailsPanel;

    QWidget *EditHolesPanel;

    QWidget *EditSectionsPanel;
    QGroupBox *EditSectionsNoSelection;
    QGroupBox *EditSectionsSectionInfo;
    QLabel *EditSectionsSectionInfoSectionX;
    QLabel *EditSectionsSectionInfoSectionY;
    QGroupBox *EditSectionsActiveSection;
    QLabel *EditSectionsActiveSectionNodeCount;
    QLabel *EditSectionsActiveSectionSplatMapCount;
    QListWidget *EditSectionsActiveSectionUsedLayerList;
    QGroupBox *EditSectionsActiveSectionOps;
    QPushButton *EditSectionsActiveSectionOpsDeleteLayers;
    QPushButton *EditSectionsActiveSectionOpsDeleteSection;
    QPushButton *EditSectionsActiveSectionOpsRebuildQuadTree;
    QPushButton *EditSectionsActiveSectionOpsRebuildSplatMap;
    QGroupBox *EditSectionsInactiveSection;
    QComboBox *EditSectionsInactiveSectionCreateLayer;
    QDoubleSpinBox *EditSectionsInactiveSectionCreateHeight;
    QGroupBox *EditSectionsInactiveSectionOps;
    QPushButton *EditSectionsInactiveSectionOpsCreateSection;

    QWidget *HeightmapImportPanel;
    QRadioButton *HeightmapImportPanelSourceTypeRaw8;
    QRadioButton *HeightmapImportPanelSourceTypeRaw16;
    QRadioButton *HeightmapImportPanelSourceTypeRawFloat;
    QRadioButton *HeightmapImportPanelSourceTypeImage;
    QPushButton *HeightmapImportPanelSource;
    QString HeightmapImportPanelSourceFileName;
    QLabel *HeightmapImportPanelSourcePreview;
    QLabel *HeightmapImportPanelSourceWidth;
    QLabel *HeightmapImportPanelSourceHeight;
    QSpinBox *HeightmapImportPanelDestinationStartSectionX;
    QSpinBox *HeightmapImportPanelDestinationStartSectionY;
    QDoubleSpinBox *HeightmapImportPanelDestinationMinHeight;
    QDoubleSpinBox *HeightmapImportPanelDestinationMaxHeight;
    QButtonGroup *HeightmapImportPanelScaleType;
    QRadioButton *HeightmapImportPanelScaleTypeNone;
    QRadioButton *HeightmapImportPanelScaleTypeDownscale;
    QRadioButton *HeightmapImportPanelScaleTypeUpscale;
    QSpinBox *HeightmapImportPanelScaleAmount;
    QLabel *HeightmapImportPanelResultSizeX;
    QLabel *HeightmapImportPanelResultSizeY;
    QPushButton *HeightmapImportPanelImport;

    QWidget *SettingsPanel;
    QSpinBox *SettingsPanelViewDistance;
    QSpinBox *SettingsPanelRenderResolutionMultiplier;
    QDoubleSpinBox *SettingsPanelLODDistanceRatio;
    QPushButton *SettingsPanelRebuildQuadtree;
    QPushButton *SettingsPanelRebuildSplatMaps;

    void CreateUI(QWidget *parentWidget)
    {
        QVBoxLayout *verticalLayout2;
        QFormLayout *formLayout;
        QLabel *label;

        // helpful bits
        QSizePolicy expandAndVerticalStretchSizePolicy;
        expandAndVerticalStretchSizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        expandAndVerticalStretchSizePolicy.setVerticalPolicy(QSizePolicy::Expanding);
        expandAndVerticalStretchSizePolicy.setVerticalStretch(1);

        root = new QWidget(parentWidget);

        QVBoxLayout *rootLayout = new QVBoxLayout(root);
        rootLayout->setSpacing(2);
        rootLayout->setMargin(0);
        rootLayout->setContentsMargins(2, 2, 2, 2);

        // toolbar
        {
            buttonsPanel = new QWidget(root);
            buttonsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            {
                verticalLayout2 = new QVBoxLayout(buttonsPanel);
                verticalLayout2->setSizeConstraint(QLayout::SetMaximumSize);
                verticalLayout2->setSpacing(0);
                verticalLayout2->setMargin(0);

                buttonEditHeight = new QToolButton(buttonsPanel);
                buttonEditHeight->setText(root->tr("Edit Height"));
                buttonEditHeight->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonEditHeight->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonEditHeight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonEditHeight->setMaximumSize(QSize(16777215, 24));
                buttonEditHeight->setCheckable(true);
                buttonEditHeight->setAutoRaise(true);
                verticalLayout2->addWidget(buttonEditHeight);

                buttonEditLayers = new QToolButton(buttonsPanel);
                buttonEditLayers->setText(root->tr("Edit Layers"));
                buttonEditLayers->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonEditLayers->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonEditLayers->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonEditLayers->setMaximumSize(QSize(16777215, 24));
                buttonEditLayers->setCheckable(true);
                buttonEditLayers->setAutoRaise(true);
                verticalLayout2->addWidget(buttonEditLayers);

                buttonEditDetails = new QToolButton(buttonsPanel);
                buttonEditDetails->setText(root->tr("Edit Detail"));
                buttonEditDetails->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonEditDetails->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonEditDetails->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonEditDetails->setMaximumSize(QSize(16777215, 24));
                buttonEditDetails->setCheckable(true);
                buttonEditDetails->setAutoRaise(true);
                verticalLayout2->addWidget(buttonEditDetails);

                buttonEditHoles = new QToolButton(buttonsPanel);
                buttonEditHoles->setText(root->tr("Edit Holes"));
                buttonEditHoles->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonEditHoles->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonEditHoles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonEditHoles->setMaximumSize(QSize(16777215, 24));
                buttonEditHoles->setCheckable(true);
                buttonEditHoles->setAutoRaise(true);
                verticalLayout2->addWidget(buttonEditHoles);

                buttonEditSections = new QToolButton(buttonsPanel);
                buttonEditSections->setText(root->tr("Edit Sections"));
                buttonEditSections->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonEditSections->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonEditSections->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonEditSections->setMaximumSize(QSize(16777215, 24));
                buttonEditSections->setCheckable(true);
                buttonEditSections->setAutoRaise(true);
                verticalLayout2->addWidget(buttonEditSections);

                buttonImportHeightmap = new QToolButton(buttonsPanel);
                buttonImportHeightmap->setText(root->tr("Import Heightmap"));
                buttonImportHeightmap->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonImportHeightmap->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonImportHeightmap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonImportHeightmap->setMaximumSize(QSize(16777215, 24));
                buttonImportHeightmap->setCheckable(true);
                buttonImportHeightmap->setAutoRaise(true);
                verticalLayout2->addWidget(buttonImportHeightmap);

                buttonSettings = new QToolButton(buttonsPanel);
                buttonSettings->setText(root->tr("Terrain Settings"));
                buttonSettings->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
                buttonSettings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                buttonSettings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                buttonSettings->setMaximumSize(QSize(16777215, 24));
                buttonSettings->setCheckable(true);
                buttonSettings->setAutoRaise(true);
                verticalLayout2->addWidget(buttonSettings);
            }
            buttonsPanel->setLayout(verticalLayout2);
            rootLayout->addWidget(buttonsPanel);
        }

        // brush settings
        BrushPanel = new QGroupBox(root->tr("Brush Settings"), root);
        BrushPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        {
            formLayout = new QFormLayout(BrushPanel);
            formLayout->setSizeConstraint(QLayout::SetMaximumSize);
            //formLayout->setSpacing(0);
            //formLayout->setMargin(0);

            BrushPanelBrushType = new QComboBox(BrushPanel);
            BrushPanelBrushType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            BrushPanelBrushType->setMaximumSize(16777215, 24);
            formLayout->addRow(root->tr("Brush Type"), BrushPanelBrushType);

            BrushPanelBrushRadius = new QDoubleSpinBox(BrushPanel);
            BrushPanelBrushRadius->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            BrushPanelBrushRadius->setMaximumSize(16777215, 24);
            BrushPanelBrushRadius->setRange(0.1f, 64.0f);
            formLayout->addRow(root->tr("Brush Radius: "), BrushPanelBrushRadius);

            BrushPanelBrushFalloff = new QDoubleSpinBox(BrushPanel);
            BrushPanelBrushFalloff->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            BrushPanelBrushFalloff->setMaximumSize(16777215, 24);
            BrushPanelBrushFalloff->setRange(0.1f, 10.0f);
            formLayout->addRow(root->tr("Brush Falloff: "), BrushPanelBrushFalloff);

            BrushPanelBrushInvert = new QCheckBox(BrushPanel);
            BrushPanelBrushInvert->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            BrushPanelBrushInvert->setMaximumSize(16777215, 24);
            formLayout->addRow(root->tr("Invert Brush"), BrushPanelBrushInvert);

            BrushPanel->setLayout(formLayout);
            BrushPanel->hide();
            rootLayout->addWidget(BrushPanel);
        }

        // edit terrain panel
        {
            EditHeightPanel = new QGroupBox(root->tr("Height Options"), root);

            formLayout = new QFormLayout(EditHeightPanel);

            EditHeightPanelStepHeight = new QDoubleSpinBox(EditHeightPanel);
            EditHeightPanelStepHeight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            EditHeightPanelStepHeight->setMaximumSize(16777215, 24);
            EditHeightPanelStepHeight->setRange(0.1f, 16.0f);
            formLayout->addRow(root->tr("Height Step: "), EditHeightPanelStepHeight);

            EditHeightPanel->setLayout(formLayout);
            EditHeightPanel->hide();
            rootLayout->addWidget(EditHeightPanel);
        }

        // edit layers panel
        {
            EditLayersPanel = new QGroupBox(root->tr("Layer Options"), root);

            formLayout = new QFormLayout(EditLayersPanel);
            formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

            EditLayersPanelStepWeight = new QDoubleSpinBox(EditLayersPanel);
            EditLayersPanelStepWeight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            EditLayersPanelStepWeight->setMaximumSize(16777215, 24);
            EditLayersPanelStepWeight->setRange(0.1f, 1.0f);
            formLayout->addRow(root->tr("Weight Step: "), EditLayersPanelStepWeight);

            EditLayersPanelLayerList = new QListWidget(EditLayersPanel);
            EditLayersPanelLayerList->setSizePolicy(expandAndVerticalStretchSizePolicy);
            formLayout->addRow(new QLabel(root->tr("Layer List: "), EditLayersPanel));
            formLayout->addRow(EditLayersPanelLayerList);

            EditLayersPanelEditLayers = new QPushButton(EditLayersPanel);
            EditLayersPanelEditLayers->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            EditLayersPanelEditLayers->setMaximumSize(16777215, 24);
            EditLayersPanelEditLayers->setText(root->tr("Edit Layers..."));
            formLayout->addRow(EditLayersPanelEditLayers);

            EditLayersPanel->setLayout(formLayout);

            EditLayersPanel->hide();
            rootLayout->addWidget(EditLayersPanel, 1);
        }

        // edit details panel
        {
            EditDetailsPanel = new QWidget(root);

            EditDetailsPanel->hide();
            rootLayout->addWidget(EditDetailsPanel);
        }

        // edit holes panel
        {
            EditHolesPanel = new QWidget(root);
            EditHolesPanel->hide();
            rootLayout->addWidget(EditHolesPanel);
        }

        // edit sections panel
        {
            EditSectionsPanel = new QWidget(root);
            QVBoxLayout *editSectionsLayout = new QVBoxLayout(EditSectionsPanel);
            editSectionsLayout->setContentsMargins(0, 0, 0, 0);

            // no selection
            {
                EditSectionsNoSelection = new QGroupBox(root->tr("Section Info"), EditSectionsPanel);

                formLayout = new QFormLayout(EditSectionsNoSelection);
                formLayout->addRow(new QLabel(root->tr("No selection"), EditSectionsNoSelection));

                EditSectionsNoSelection->setLayout(formLayout);
                editSectionsLayout->addWidget(EditSectionsNoSelection);
            }

            // section info
            {
                EditSectionsSectionInfo = new QGroupBox(root->tr("Section Info"), EditSectionsPanel);

                formLayout = new QFormLayout(EditSectionsSectionInfo);

                EditSectionsSectionInfoSectionX = new QLabel(EditSectionsSectionInfo);
                formLayout->addRow(root->tr("Section X: "), EditSectionsSectionInfoSectionX);

                EditSectionsSectionInfoSectionY = new QLabel(EditSectionsSectionInfo);
                formLayout->addRow(root->tr("Section Y: "), EditSectionsSectionInfoSectionY);

                EditSectionsSectionInfo->setLayout(formLayout);
                editSectionsLayout->addWidget(EditSectionsSectionInfo);
            }


            // active selection info
            {
                EditSectionsActiveSection = new QGroupBox(root->tr("Section Contents"), EditSectionsPanel);

                formLayout = new QFormLayout(EditSectionsActiveSection);

                EditSectionsActiveSectionNodeCount = new QLabel(EditSectionsActiveSection);
                formLayout->addRow(root->tr("Node Count: "), EditSectionsActiveSectionNodeCount);

                EditSectionsActiveSectionSplatMapCount = new QLabel(EditSectionsActiveSection);
                formLayout->addRow(root->tr("Splat Map Count: "), EditSectionsActiveSectionSplatMapCount);

                EditSectionsActiveSectionUsedLayerList = new QListWidget(EditSectionsActiveSection);
                formLayout->addRow(new QLabel(root->tr("Used Layers: "), EditSectionsActiveSection));
                formLayout->addRow(EditSectionsActiveSectionUsedLayerList);

                EditSectionsActiveSection->setLayout(formLayout);
                EditSectionsActiveSection->hide();
                editSectionsLayout->addWidget(EditSectionsActiveSection);
            }

            // active selection ops
            {
                EditSectionsActiveSectionOps = new QGroupBox(root->tr("Operations"), EditSectionsPanel);

                QVBoxLayout *vboxLayout = new QVBoxLayout(EditSectionsActiveSectionOps);

                EditSectionsActiveSectionOpsDeleteLayers = new QPushButton(root->tr("Delete Layers..."), EditSectionsActiveSectionOps);
                vboxLayout->addWidget(EditSectionsActiveSectionOpsDeleteLayers);

                EditSectionsActiveSectionOpsDeleteSection = new QPushButton(root->tr("Delete Section"), EditSectionsActiveSectionOps);
                vboxLayout->addWidget(EditSectionsActiveSectionOpsDeleteSection);

                EditSectionsActiveSectionOpsRebuildQuadTree = new QPushButton(root->tr("Rebuild QuadTree"), EditSectionsActiveSectionOps);
                vboxLayout->addWidget(EditSectionsActiveSectionOpsRebuildQuadTree);

                EditSectionsActiveSectionOpsRebuildSplatMap = new QPushButton(root->tr("Rebuild Splat Map"), EditSectionsActiveSectionOps);
                vboxLayout->addWidget(EditSectionsActiveSectionOpsRebuildSplatMap);

                EditSectionsActiveSectionOps->setLayout(vboxLayout);
                EditSectionsActiveSectionOps->hide();
                editSectionsLayout->addWidget(EditSectionsActiveSectionOps);
            }

            // inactive selection create options
            {
                EditSectionsInactiveSection = new QGroupBox(root->tr("Section Contents"), EditSectionsPanel);

                formLayout = new QFormLayout(EditSectionsInactiveSection);

                EditSectionsInactiveSectionCreateLayer = new QComboBox(EditSectionsInactiveSection);
                formLayout->addRow(new QLabel(root->tr("Create Layer: "), EditSectionsInactiveSection));
                formLayout->addRow(EditSectionsInactiveSectionCreateLayer);

                EditSectionsInactiveSectionCreateHeight = new QDoubleSpinBox(EditSectionsInactiveSection);
                formLayout->addRow(new QLabel(root->tr("Create Height: "), EditSectionsInactiveSection));
                formLayout->addRow(EditSectionsInactiveSectionCreateHeight);

                EditSectionsInactiveSection->setLayout(formLayout);
                EditSectionsInactiveSection->hide();
                editSectionsLayout->addWidget(EditSectionsInactiveSection);
            }

            // inactive selection ops
            {
                EditSectionsInactiveSectionOps = new QGroupBox(root->tr("Operations"), EditSectionsPanel);

                QVBoxLayout *vboxLayout = new QVBoxLayout(EditSectionsInactiveSectionOps);

                EditSectionsInactiveSectionOpsCreateSection = new QPushButton(root->tr("Create Section"), EditSectionsInactiveSectionOps);
                vboxLayout->addWidget(EditSectionsInactiveSectionOpsCreateSection);

                EditSectionsInactiveSectionOps->setLayout(vboxLayout);
                EditSectionsInactiveSectionOps->hide();
                editSectionsLayout->addWidget(EditSectionsInactiveSectionOps);
            }

            EditSectionsPanel->setLayout(editSectionsLayout);
            EditSectionsPanel->hide();
            rootLayout->addWidget(EditSectionsPanel);
        }

        // heightmap import panel
        {
            HeightmapImportPanel = new QWidget(root);
            QVBoxLayout *heightmapImportLayout = new QVBoxLayout(HeightmapImportPanel);
            heightmapImportLayout->setContentsMargins(0, 0, 0, 0);

            // source
            {
                QGroupBox *heightmapImportSourceGroupBox = new QGroupBox(root->tr("Source"), HeightmapImportPanel);
                QFormLayout *heightmapImportSourceFormLayout = new QFormLayout(heightmapImportSourceGroupBox);

                QButtonGroup *heightmapImportSourceTypeButtonGroup = new QButtonGroup(heightmapImportSourceGroupBox);
                HeightmapImportPanelSourceTypeRaw8 = new QRadioButton(root->tr("Raw 8-bit Heightmap"), heightmapImportSourceGroupBox);
                heightmapImportSourceTypeButtonGroup->addButton(HeightmapImportPanelSourceTypeRaw8);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSourceTypeRaw8);
                HeightmapImportPanelSourceTypeRaw16 = new QRadioButton(root->tr("Raw 16-bit Heightmap"), heightmapImportSourceGroupBox);
                heightmapImportSourceTypeButtonGroup->addButton(HeightmapImportPanelSourceTypeRaw16);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSourceTypeRaw16);
                HeightmapImportPanelSourceTypeRawFloat = new QRadioButton(root->tr("Raw Float Heightmap"), heightmapImportSourceGroupBox);
                heightmapImportSourceTypeButtonGroup->addButton(HeightmapImportPanelSourceTypeRawFloat);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSourceTypeRawFloat);
                HeightmapImportPanelSourceTypeImage = new QRadioButton(root->tr("Image"), heightmapImportSourceGroupBox);
                heightmapImportSourceTypeButtonGroup->addButton(HeightmapImportPanelSourceTypeImage);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSourceTypeImage);

                HeightmapImportPanelSource = new QPushButton(heightmapImportSourceGroupBox);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSource);

                HeightmapImportPanelSourceWidth = new QLabel(heightmapImportSourceGroupBox);
                HeightmapImportPanelSourceWidth->setText(QStringLiteral("0"));
                heightmapImportSourceFormLayout->addRow(root->tr("Source Width: "), HeightmapImportPanelSourceWidth);

                HeightmapImportPanelSourceHeight = new QLabel(heightmapImportSourceGroupBox);
                HeightmapImportPanelSourceHeight->setText(QStringLiteral("0"));
                heightmapImportSourceFormLayout->addRow(root->tr("Source Height: "), HeightmapImportPanelSourceHeight);

                HeightmapImportPanelSourcePreview = new QLabel(heightmapImportSourceGroupBox);
                HeightmapImportPanelSourcePreview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                HeightmapImportPanelSourcePreview->setFixedSize(128, 128);
                HeightmapImportPanelSourcePreview->setBackgroundRole(QPalette::Dark);
                heightmapImportSourceFormLayout->addRow(HeightmapImportPanelSourcePreview);

                heightmapImportSourceGroupBox->setLayout(heightmapImportSourceFormLayout);
                heightmapImportLayout->addWidget(heightmapImportSourceGroupBox);
            }

            // destination
            {
                QGroupBox *heightmapImportDestinationGroupBox = new QGroupBox(root->tr("Destination"), HeightmapImportPanel);
                QFormLayout *heightmapImportDestinationFormLayout = new QFormLayout(heightmapImportDestinationGroupBox);

                HeightmapImportPanelDestinationStartSectionX = new QSpinBox(heightmapImportDestinationGroupBox);
                heightmapImportDestinationFormLayout->addRow(root->tr("Start Section X: "), HeightmapImportPanelDestinationStartSectionX);

                HeightmapImportPanelDestinationStartSectionY = new QSpinBox(heightmapImportDestinationGroupBox);
                heightmapImportDestinationFormLayout->addRow(root->tr("Start Section Y: "), HeightmapImportPanelDestinationStartSectionY);

                HeightmapImportPanelDestinationMinHeight = new QDoubleSpinBox(heightmapImportDestinationGroupBox);
                HeightmapImportPanelDestinationMinHeight->setMinimum(-65536.0);
                HeightmapImportPanelDestinationMinHeight->setMaximum(65536.0);
                HeightmapImportPanelDestinationMinHeight->setSingleStep(0.1);
                HeightmapImportPanelDestinationMinHeight->setValue(0);
                heightmapImportDestinationFormLayout->addRow(root->tr("Min Height: "), HeightmapImportPanelDestinationMinHeight);

                HeightmapImportPanelDestinationMaxHeight = new QDoubleSpinBox(heightmapImportDestinationGroupBox);
                HeightmapImportPanelDestinationMaxHeight->setMinimum(-65536.0);
                HeightmapImportPanelDestinationMaxHeight->setMaximum(65536.0);
                HeightmapImportPanelDestinationMaxHeight->setSingleStep(0.1);
                HeightmapImportPanelDestinationMaxHeight->setValue(32.0);
                heightmapImportDestinationFormLayout->addRow(root->tr("Max Height: "), HeightmapImportPanelDestinationMaxHeight);

                heightmapImportDestinationGroupBox->setLayout(heightmapImportDestinationFormLayout);
                heightmapImportLayout->addWidget(heightmapImportDestinationGroupBox);
            }

            // scale
            {
                QGroupBox *heightmapImportScaleGroupBox = new QGroupBox(root->tr("Scale"), HeightmapImportPanel);
                QFormLayout *heightmapImportScaleFormLayout = new QFormLayout(heightmapImportScaleGroupBox);

                HeightmapImportPanelScaleType = new QButtonGroup(heightmapImportScaleGroupBox);

                HeightmapImportPanelScaleTypeNone = new QRadioButton(root->tr("None"), heightmapImportScaleGroupBox);
                heightmapImportScaleFormLayout->addRow(HeightmapImportPanelScaleTypeNone);

                HeightmapImportPanelScaleTypeDownscale = new QRadioButton(root->tr("Downscale"), heightmapImportScaleGroupBox);
                heightmapImportScaleFormLayout->addRow(HeightmapImportPanelScaleTypeDownscale);

                HeightmapImportPanelScaleTypeUpscale = new QRadioButton(root->tr("Upscale"), heightmapImportScaleGroupBox);
                heightmapImportScaleFormLayout->addRow(HeightmapImportPanelScaleTypeUpscale);

                HeightmapImportPanelScaleAmount = new QSpinBox(heightmapImportScaleGroupBox);
                HeightmapImportPanelScaleAmount->setMinimum(1);
                HeightmapImportPanelScaleAmount->setMaximum(128);
                heightmapImportScaleFormLayout->addRow(root->tr("Scale Amount: "), HeightmapImportPanelScaleAmount);

                heightmapImportScaleGroupBox->setLayout(heightmapImportScaleFormLayout);
                heightmapImportLayout->addWidget(heightmapImportScaleGroupBox);
            }

            // result
            {
                QGroupBox *heightmapImportResultGroupBox = new QGroupBox(root->tr("Result"), HeightmapImportPanel);
                QFormLayout *heightmapImportResultFormLayout = new QFormLayout(heightmapImportResultGroupBox);

                HeightmapImportPanelResultSizeX = new QLabel(heightmapImportResultGroupBox);
                HeightmapImportPanelResultSizeX->setText(QStringLiteral("0"));
                heightmapImportResultFormLayout->addRow(root->tr("X Points: "), HeightmapImportPanelResultSizeX);

                HeightmapImportPanelResultSizeY = new QLabel(heightmapImportResultGroupBox);
                HeightmapImportPanelResultSizeY->setText(QStringLiteral("0"));
                heightmapImportResultFormLayout->addRow(root->tr("Y Points: "), HeightmapImportPanelResultSizeY);

                HeightmapImportPanelImport = new QPushButton(heightmapImportResultGroupBox);
                HeightmapImportPanelImport->setText(root->tr("Import..."));
                heightmapImportResultFormLayout->addRow(HeightmapImportPanelImport);

                heightmapImportResultGroupBox->setLayout(heightmapImportResultFormLayout);
                heightmapImportLayout->addWidget(heightmapImportResultGroupBox);
            }

            HeightmapImportPanel->setLayout(heightmapImportLayout);
            HeightmapImportPanel->hide();
            rootLayout->addWidget(HeightmapImportPanel);
        }

        // terrain settings panel
        {
            SettingsPanel = new QWidget(root);
            SettingsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

            verticalLayout2 = new QVBoxLayout(SettingsPanel);
            verticalLayout2->setSizeConstraint(QLayout::SetMaximumSize);
            verticalLayout2->setSpacing(0);
            verticalLayout2->setMargin(0);
            verticalLayout2->setContentsMargins(4, 4, 4, 4);

            label = new QLabel(SettingsPanel);
            label->setText(root->tr("View Distance: "));
            label->setMaximumSize(QSize(16777215, 24));
            verticalLayout2->addWidget(label);

            SettingsPanelViewDistance = new QSpinBox(SettingsPanel);
            SettingsPanelViewDistance->setRange(16, 65535);
            SettingsPanelViewDistance->setSingleStep(16);
            SettingsPanelViewDistance->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            SettingsPanelViewDistance->setMaximumSize(QSize(16777215, 24));
            SettingsPanelViewDistance->setValue(CVars::r_terrain_view_distance.GetInt());
            verticalLayout2->addWidget(SettingsPanelViewDistance);

            label = new QLabel(SettingsPanel);
            label->setText(root->tr("Render Resolution Multiplier: "));
            label->setMaximumSize(QSize(16777215, 24));
            verticalLayout2->addWidget(label);

            SettingsPanelRenderResolutionMultiplier = new QSpinBox(SettingsPanel);
            SettingsPanelRenderResolutionMultiplier->setRange(1, 10);
            SettingsPanelRenderResolutionMultiplier->setSingleStep(1);
            SettingsPanelRenderResolutionMultiplier->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            SettingsPanelRenderResolutionMultiplier->setMaximumSize(QSize(16777215, 24));
            SettingsPanelRenderResolutionMultiplier->setValue(CVars::r_terrain_render_resolution_multiplier.GetInt());
            verticalLayout2->addWidget(SettingsPanelRenderResolutionMultiplier);

            label = new QLabel(SettingsPanel);
            label->setText(root->tr("LOD Distance Ratio: "));
            label->setMaximumSize(QSize(16777215, 24));
            verticalLayout2->addWidget(label);

            SettingsPanelLODDistanceRatio = new QDoubleSpinBox(SettingsPanel);
            SettingsPanelLODDistanceRatio->setRange(1.0, 10.0);
            SettingsPanelLODDistanceRatio->setSingleStep(0.5);
            SettingsPanelLODDistanceRatio->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            SettingsPanelLODDistanceRatio->setMaximumSize(QSize(16777215, 24));
            SettingsPanelLODDistanceRatio->setValue(CVars::r_terrain_lod_distance_ratio.GetFloat());
            verticalLayout2->addWidget(SettingsPanelLODDistanceRatio);

            SettingsPanelRebuildQuadtree = new QPushButton(root->tr("Rebuild QuadTrees"), SettingsPanel);
            verticalLayout2->addWidget(SettingsPanelRebuildQuadtree);

            SettingsPanelRebuildSplatMaps = new QPushButton(root->tr("Rebuild Splat Maps"), SettingsPanel);
            verticalLayout2->addWidget(SettingsPanelRebuildSplatMaps);

            SettingsPanel->setLayout(verticalLayout2);
            SettingsPanel->hide();
            rootLayout->addWidget(SettingsPanel);
        }

        // complete widget setup
        rootLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
        root->setLayout(rootLayout);
    }

    void OnWidgetChanged(EDITOR_TERRAIN_EDIT_MODE_WIDGET currentWidget)
    {
        buttonEditHeight->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT));
        buttonEditLayers->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS));
        buttonEditDetails->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS));
        buttonEditHoles->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HOLES));
        buttonEditSections->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS));
        buttonImportHeightmap->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_IMPORT_HEIGHTMAP));
        buttonSettings->setChecked((currentWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_SETTINGS));

        BrushPanel->hide();
        EditHeightPanel->hide();
        EditLayersPanel->hide();
        EditDetailsPanel->hide();
        EditHolesPanel->hide();
        EditSectionsPanel->hide();
        HeightmapImportPanel->hide();
        SettingsPanel->hide();

        ResetHeightmapImport();

        switch (currentWidget)
        {
        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT:
            BrushPanel->show();
            EditHeightPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS:
            BrushPanel->show();
            EditLayersPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS:
            BrushPanel->show();
            EditDetailsPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HOLES:
            EditHolesPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS:
            EditSectionsPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_IMPORT_HEIGHTMAP:
            HeightmapImportPanel->show();
            break;

        case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_SETTINGS:
            SettingsPanel->show();
            break;
        }
    }

    void ResetHeightmapImport(bool clearType = true)
    {
        if (clearType)
            HeightmapImportPanelSourceTypeRaw8->setChecked(true);

        HeightmapImportPanelSourceFileName.clear();
        HeightmapImportPanelSourceWidth->setText(QStringLiteral("0"));
        HeightmapImportPanelSourceHeight->setText(QStringLiteral("0"));
        HeightmapImportPanelSourcePreview->setPixmap(QPixmap());
        HeightmapImportPanelDestinationStartSectionX->setValue(0);
        HeightmapImportPanelDestinationStartSectionY->setValue(0);
        HeightmapImportPanelScaleTypeNone->setChecked(true);
        HeightmapImportPanelScaleAmount->setValue(1);
        UpdateHeightmapImport();
    }

    void UpdateHeightmapImport()
    {
        bool allow = true;
        uint32 pointsX = 0;
        uint32 pointsY = 0;

        if (HeightmapImportPanelSourceFileName.length() > 0)
        {
            HeightmapImportPanelSource->setText(HeightmapImportPanelSourceFileName);

            uint32 imageSizeX = StringConverter::StringToUInt32(ConvertQStringToString(HeightmapImportPanelSourceWidth->text()));
            uint32 imageSizeY = StringConverter::StringToUInt32(ConvertQStringToString(HeightmapImportPanelSourceHeight->text()));
            uint32 scale = (uint32)HeightmapImportPanelScaleAmount->value();

            if (HeightmapImportPanelScaleTypeDownscale->isChecked())
            {
                pointsX = imageSizeX / scale;
                pointsY = imageSizeY / scale;
            }
            else if (HeightmapImportPanelScaleTypeUpscale->isChecked())
            {
                pointsX = imageSizeX * scale;
                pointsY = imageSizeY * scale;
            }
            else
            {
                pointsX = imageSizeX;
                pointsY = imageSizeY;
            }
        }
        else
        {
            HeightmapImportPanelSource->setText(QStringLiteral("Select Source..."));
            allow = false;
        }

        if (HeightmapImportPanelScaleTypeNone->isChecked())
        {
            HeightmapImportPanelScaleAmount->setValue(1);
            HeightmapImportPanelScaleAmount->setEnabled(false);
        }
        else
        {
            HeightmapImportPanelScaleAmount->setEnabled(true);
        }

        HeightmapImportPanelResultSizeX->setText(QString::number(pointsX));
        HeightmapImportPanelResultSizeY->setText(QString::number(pointsY));
        HeightmapImportPanelImport->setEnabled(allow);
    }
};

QWidget *EditorTerrainEditMode::CreateUI(QWidget *parentWidget)
{
    DebugAssert(m_ui == nullptr);
    m_ui = new ui_EditorTerrainEditMode();
    m_ui->CreateUI(parentWidget);

    connect(m_ui->buttonEditHeight, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetEditTerrainClicked(bool)));
    connect(m_ui->buttonEditDetails, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetEditDetailsClicked(bool)));
    connect(m_ui->buttonEditHoles, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetEditHolesClicked(bool)));
    connect(m_ui->buttonEditSections, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetEditSectionsClicked(bool)));
    connect(m_ui->buttonEditLayers, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetEditLayersClicked(bool)));
    connect(m_ui->buttonImportHeightmap, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetImportHeightmapClicked(bool)));
    connect(m_ui->buttonSettings, SIGNAL(clicked(bool)), this, SLOT(OnUIWidgetSettingsClicked(bool)));

    connect(m_ui->BrushPanelBrushRadius, SIGNAL(valueChanged(double)), this, SLOT(OnUIBrushRadiusChanged(double)));
    connect(m_ui->BrushPanelBrushFalloff, SIGNAL(valueChanged(double)), this, SLOT(OnUIBrushFalloffChanged(double)));
    connect(m_ui->BrushPanelBrushInvert, SIGNAL(clicked(bool)), this, SLOT(OnUIBrushInvertChanged(bool)));

    connect(m_ui->EditHeightPanelStepHeight, SIGNAL(valueChanged(double)), this, SLOT(OnUITerrainStepHeightChanged(double)));

    connect(m_ui->EditLayersPanelStepWeight, SIGNAL(valueChanged(double)), this, SLOT(OnUILayersStepWeightChanged(double)));
    connect(m_ui->EditLayersPanelLayerList, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(OnUILayersLayerListItemClicked(QListWidgetItem *)));
    connect(m_ui->EditLayersPanelEditLayers, SIGNAL(clicked()), this, SLOT(OnUILayersEditLayerListClicked()));

    connect(m_ui->EditSectionsActiveSectionOpsDeleteLayers, SIGNAL(clicked()), this, SLOT(OnUISectionsActiveSectionDeleteLayersClicked()));
    connect(m_ui->EditSectionsActiveSectionOpsDeleteSection, SIGNAL(clicked()), this, SLOT(OnUISectionsActiveSectionDeleteSectionClicked()));
    connect(m_ui->EditSectionsInactiveSectionOpsCreateSection, SIGNAL(clicked()), this, SLOT(OnUISectionsInactiveSectionCreateSectionClicked()));

    connect(m_ui->HeightmapImportPanelSourceTypeRaw8, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelSourceTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelSourceTypeRaw16, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelSourceTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelSourceTypeRawFloat, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelSourceTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelSource, SIGNAL(clicked()), this, SLOT(OnUIHeightmapImportPanelSelectSourceImageClicked()));
    connect(m_ui->HeightmapImportPanelScaleTypeNone, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelScaleTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelScaleTypeDownscale, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelScaleTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelScaleTypeUpscale, SIGNAL(clicked(bool)), this, SLOT(OnUIHeightmapImportPanelScaleTypeClicked(bool)));
    connect(m_ui->HeightmapImportPanelScaleAmount, SIGNAL(valueChanged(int)), this, SLOT(OnUIHeightmapImportPanelScaleAmountChanged(int)));
    connect(m_ui->HeightmapImportPanelImport, SIGNAL(clicked()), this, SLOT(OnUIHeightmapImportPanelImportClicked()));

    connect(m_ui->SettingsPanelViewDistance, SIGNAL(valueChanged(int)), this, SLOT(OnUISettingsPanelViewDistanceValueChanged(int)));
    connect(m_ui->SettingsPanelRenderResolutionMultiplier, SIGNAL(valueChanged(int)), this, SLOT(OnUISettingsPanelRenderResolutionMultiplierChanged(int)));
    connect(m_ui->SettingsPanelLODDistanceRatio, SIGNAL(valueChanged(double)), this, SLOT(OnUISettingsPanelLODDistanceRatioChanged(double)));
    connect(m_ui->SettingsPanelRebuildQuadtree, SIGNAL(clicked()), this, SLOT(OnUISettingsPanelRebuildQuadtreeClicked()));

    // create the layer list view
    UpdateDetailLayerList();

    // update ui
    m_ui->OnWidgetChanged(m_eActiveWidget);

    // one-time filling of data
    BlockSignalsForCall(m_ui->BrushPanelBrushRadius)->setValue(m_brushRadius);
    BlockSignalsForCall(m_ui->BrushPanelBrushFalloff)->setValue(m_brushFalloff);
    BlockSignalsForCall(m_ui->EditHeightPanelStepHeight)->setValue(m_brushTerrainStep);
    BlockSignalsForCall(m_ui->EditLayersPanelStepWeight)->setValue(m_brushLayerStep);
    UpdateUIForSelectedSection();

    return m_ui->root;
}

void EditorTerrainEditMode::OnUIWidgetEditTerrainClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT);
}

void EditorTerrainEditMode::OnUIWidgetEditDetailsClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS);
}

void EditorTerrainEditMode::OnUIWidgetEditHolesClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HOLES);
}

void EditorTerrainEditMode::OnUIWidgetEditSectionsClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS);
}

void EditorTerrainEditMode::OnUIWidgetEditLayersClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS);
}

void EditorTerrainEditMode::OnUIWidgetImportHeightmapClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_IMPORT_HEIGHTMAP);
}

void EditorTerrainEditMode::OnUIWidgetSettingsClicked(bool checked)
{
    SetActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_SETTINGS);
}

void EditorTerrainEditMode::OnUIBrushRadiusChanged(double value)
{
    SetBrushRadius((float)value);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUIBrushFalloffChanged(double value)
{
    SetBrushFalloff((float)value);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUIBrushInvertChanged(bool checked)
{

}

void EditorTerrainEditMode::OnUITerrainStepHeightChanged(double value)
{
    SetBrushHeightStep((float)value);
}

void EditorTerrainEditMode::OnUILayersStepWeightChanged(double value)
{
    SetBrushLayerStep((float)value);
}

void EditorTerrainEditMode::OnUILayersLayerListItemClicked(QListWidgetItem *pListItem)
{
    const TerrainLayerListBaseLayer *pBaseLayer = m_pTerrainData->GetLayerList()->GetBaseLayerByName(ConvertQStringToString(pListItem->text()));
    SetBrushLayerSelectedLayer((pBaseLayer != nullptr) ? pBaseLayer->Index : -1);  
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUILayersEditLayerListClicked()
{

}

void EditorTerrainEditMode::OnUISectionsActiveSectionDeleteLayersClicked()
{

}

void EditorTerrainEditMode::OnUISectionsActiveSectionDeleteSectionClicked()
{

}

void EditorTerrainEditMode::OnUISectionsInactiveSectionCreateSectionClicked()
{
    // skip if nothing we can create
    if (!m_validSelectedSection || m_pTerrainData->IsSectionAvailable(m_selectedSection.x, m_selectedSection.y))
        return;

    // read the create layer and height
    float createHeight = 0.0f;
    uint8 createLayer = 0;

    // invoke the creation
    if (CreateSection(m_selectedSection.x, m_selectedSection.y, createHeight, createLayer))
    {
        // update ui
        UpdateUIForSelectedSection();
    }
}

void EditorTerrainEditMode::OnUIHeightmapImportPanelSourceTypeClicked(bool checked)
{
    if (checked)
        m_ui->ResetHeightmapImport(false);
}

void EditorTerrainEditMode::OnUIHeightmapImportPanelSelectSourceImageClicked()
{
    if (m_ui->HeightmapImportPanelSourceTypeImage->isChecked())
    {
        QString filename = QFileDialog::getOpenFileName(m_pMap->GetMapWindow(),
                                                        tr("Select Image..."),
                                                        QStringLiteral(""),
                                                        tr("Image Files (*.bmp *.jpg *.jpeg *.png)"),
                                                        NULL,
                                                        0);

        if (filename.isEmpty())
            return;

        QImage loadedImage;
        if (!loadedImage.load(filename))
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Failed to load image at ") + filename);
            return;
        }

        m_ui->HeightmapImportPanelSourceFileName = filename;
        m_ui->HeightmapImportPanelSourceWidth->setText(QString::number(loadedImage.width()));
        m_ui->HeightmapImportPanelSourceHeight->setText(QString::number(loadedImage.height()));

        QPixmap loadedImagePixmap(QPixmap::fromImage(loadedImage));
        m_ui->HeightmapImportPanelSourcePreview->setPixmap(loadedImagePixmap.scaled(128, 128));
    }
    else
    {
        QString filename = QFileDialog::getOpenFileName(m_pMap->GetMapWindow(),
                                                        tr("Select Raw File..."),
                                                        QStringLiteral(""),
                                                        tr("Any File (*.*)"),
                                                        NULL,
                                                        0);

        if (filename.isEmpty())
            return;

        // calculate bytes per pixel
        uint32 bytesPerPixel = 1;
        if (m_ui->HeightmapImportPanelSourceTypeRaw16->isChecked())
            bytesPerPixel = 2;
        else if (m_ui->HeightmapImportPanelSourceTypeRawFloat->isChecked())
            bytesPerPixel = 4;

        // open file
        AutoReleasePtr<ByteStream> pStream = FileSystem::OpenFile(ConvertQStringToString(filename), BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Failed to load image at ") + filename);
            return;
        }

        // work out dimensions, if the heightmap is >4gb we are in trouble
        uint32 fileSize = (uint32)pStream->GetSize();
        float imageSizeF = Y_sqrtf(static_cast<float>(fileSize / bytesPerPixel));
        uint32 imageSize = Math::Truncate(imageSizeF);
        if (Math::FractionalPart(imageSizeF) != 0.0f)
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Heightmap is unacceptable or not square: ") + filename);
            return;
        }

        // create the image
        byte *pInRow = new byte[imageSize * bytesPerPixel];
        QImage previewImage(imageSize, imageSize, QImage::Format_RGB32);
        for (uint32 y = 0; y < imageSize; y++)
        {
            // read the row
            if (!pStream->Read2(pInRow, imageSize * bytesPerPixel))
            {
                QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Read error in heightmap: ") + filename);
                delete[] pInRow;
                return;
            }

            // process it
            if (m_ui->HeightmapImportPanelSourceTypeRawFloat->isChecked())
            {
                const float *pRowPointer = reinterpret_cast<const float *>(pInRow);
                float minHeight = (float)m_ui->HeightmapImportPanelDestinationMinHeight->value();
                float maxHeight = (float)m_ui->HeightmapImportPanelDestinationMaxHeight->value();

                for (uint32 x = 0; x < imageSize; x++)
                {
                    float value = (*pRowPointer - minHeight) / maxHeight;
                    pRowPointer++;

                    uint8 byteValue = (uint8)Math::Truncate(Math::Clamp(value * 255.0f, 0.0f, 255.0f));
                    previewImage.setPixel(x, y, qRgb(byteValue, byteValue, byteValue));
                }
            }
            else if (m_ui->HeightmapImportPanelSourceTypeRaw16->isChecked())
            {
                const uint16 *pRowPointer = reinterpret_cast<const uint16 *>(pInRow);

                for (uint32 x = 0; x < imageSize; x++)
                {
                    float value = *pRowPointer / 65535.0f;
                    pRowPointer++;

                    uint8 byteValue = (uint8)Math::Truncate(Math::Clamp(value * 255.0f, 0.0f, 255.0f));
                    previewImage.setPixel(x, y, qRgb(byteValue, byteValue, byteValue));
                }
            }
            else
            {
                const uint8 *pRowPointer = reinterpret_cast<const uint8 *>(pInRow);

                for (uint32 x = 0; x < imageSize; x++)
                {
                    float value = *pRowPointer / 255.0f;
                    pRowPointer++;

                    uint8 byteValue = (uint8)Math::Truncate(Math::Clamp(value * 255.0f, 0.0f, 255.0f));
                    previewImage.setPixel(x, y, qRgb(byteValue, byteValue, byteValue));
                }
            }
        }

        m_ui->HeightmapImportPanelSourceFileName = filename;
        m_ui->HeightmapImportPanelSourceWidth->setText(QString::number(previewImage.width()));
        m_ui->HeightmapImportPanelSourceHeight->setText(QString::number(previewImage.height()));

        QPixmap loadedImagePixmap(QPixmap::fromImage(previewImage.scaled(128, 128)));
        m_ui->HeightmapImportPanelSourcePreview->setPixmap(loadedImagePixmap);
    }

    m_ui->UpdateHeightmapImport();
}

void EditorTerrainEditMode::OnUIHeightmapImportPanelScaleAmountChanged(int value)
{
    m_ui->UpdateHeightmapImport();
}

void EditorTerrainEditMode::OnUIHeightmapImportPanelScaleTypeClicked(bool checked)
{
    if (checked)
        m_ui->UpdateHeightmapImport();
}

void EditorTerrainEditMode::OnUIHeightmapImportPanelImportClicked()
{
    EditorProgressDialog progressDialog(m_pMap->GetMapWindow());
    progressDialog.show();

    // read parameters
    int32 startSectionX = (int32)m_ui->HeightmapImportPanelDestinationStartSectionX->value();
    int32 startSectionY = (int32)m_ui->HeightmapImportPanelDestinationStartSectionY->value();
    float minHeight = (float)m_ui->HeightmapImportPanelDestinationMinHeight->value();
    float maxHeight = (float)m_ui->HeightmapImportPanelDestinationMaxHeight->value();
    MapSourceTerrainData::HeightmapImportScaleType scaleType = MapSourceTerrainData::HeightmapImportScaleType_None;
    uint32 scaleAmount = (uint32)m_ui->HeightmapImportPanelScaleAmount->value();

    // fix up scale
    if (m_ui->HeightmapImportPanelScaleTypeDownscale->isChecked())
        scaleType = MapSourceTerrainData::HeightmapImportScaleType_Downscale;
    else if (m_ui->HeightmapImportPanelScaleTypeUpscale->isChecked())
        scaleType = MapSourceTerrainData::HeightmapImportScaleType_Upscale;
    else
        scaleAmount = 1;

    // load up heightmap
    Image heightmapImage;
    progressDialog.SetStatusText("Loading Heightmap...");

    // for images.
    if (m_ui->HeightmapImportPanelSourceTypeImage->isChecked())
    {
        QImage loadedImage;
        if (!loadedImage.load(m_ui->HeightmapImportPanelSourceFileName))
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Failed to load image at ") + m_ui->HeightmapImportPanelSourceFileName);
            return;
        }

        if (!EditorHelpers::ConvertQImageToImage(&loadedImage, &heightmapImage, PIXEL_FORMAT_R8_UNORM))
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Failed to convert image at ") + m_ui->HeightmapImportPanelSourceFileName);
            return;
        }
    }
    else
    {
        // calculate bytes per pixel
        uint32 bytesPerPixel = 1;
        if (m_ui->HeightmapImportPanelSourceTypeRaw16->isChecked())
            bytesPerPixel = 2;
        else if (m_ui->HeightmapImportPanelSourceTypeRawFloat->isChecked())
            bytesPerPixel = 4;

        // open file
        AutoReleasePtr<ByteStream> pStream = FileSystem::OpenFile(ConvertQStringToString(m_ui->HeightmapImportPanelSourceFileName), BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Failed to load image at ") + m_ui->HeightmapImportPanelSourceFileName);
            return;
        }

        // work out dimensions, if the heightmap is >4gb we are in trouble
        uint32 fileSize = (uint32)pStream->GetSize();
        float imageSizeF = Y_sqrtf(static_cast<float>(fileSize / bytesPerPixel));
        uint32 imageSize = Math::Truncate(imageSizeF);
        if (Math::FractionalPart(imageSizeF) != 0.0f)
        {
            QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Heightmap is unacceptable or not square: ") + m_ui->HeightmapImportPanelSourceFileName);
            return;
        }

        // create the image
        if (m_ui->HeightmapImportPanelSourceTypeRawFloat->isChecked())
            heightmapImage.Create(PIXEL_FORMAT_R32_FLOAT, imageSize, imageSize, 1);
        else if (m_ui->HeightmapImportPanelSourceTypeRaw16->isChecked())
            heightmapImage.Create(PIXEL_FORMAT_R16_UNORM, imageSize, imageSize, 1);
        else
            heightmapImage.Create(PIXEL_FORMAT_R8_UNORM, imageSize, imageSize, 1);

        // read in image rows
        byte *pWritePointer = reinterpret_cast<byte *>(heightmapImage.GetData());
        for (uint32 y = 0; y < imageSize; y++)
        {
            // read the row
            if (!pStream->Read2(pWritePointer, imageSize * bytesPerPixel))
            {
                QMessageBox::critical(m_pMap->GetMapWindow(), tr("Image Load Error"), tr("Read error in heightmap: ") + m_ui->HeightmapImportPanelSourceFileName);
                return;
            }

            // increment write pointer
            pWritePointer += heightmapImage.GetDataRowPitch();
        }
    }

    // run the import
    ImportHeightmap(&heightmapImage, startSectionX, startSectionY, minHeight, maxHeight, scaleType, scaleAmount, &progressDialog);

    // redraw
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUISettingsPanelViewDistanceValueChanged(int value)
{
    g_pConsole->SetCVar(&CVars::r_terrain_view_distance, (int32)value);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUISettingsPanelRenderResolutionMultiplierChanged(int value)
{
    g_pConsole->SetCVar(&CVars::r_terrain_render_resolution_multiplier, (int32)value);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUISettingsPanelLODDistanceRatioChanged(double value)
{
    g_pConsole->SetCVar(&CVars::r_terrain_lod_distance_ratio, (float)value);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::OnUISettingsPanelRebuildQuadtreeClicked()
{
    EditorProgressDialog progressDialog(m_pMap->GetMapWindow());
    progressDialog.show();

    RebuildQuadTree(m_pTerrainData->GetParameters()->LODCount, &progressDialog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorTerrainEditMode::EditorTerrainEditMode(EditorMap *pMap)
    : EditorEditMode(pMap),
      m_ui(nullptr),
      m_pTerrainData(nullptr),
      m_pTerrainRenderer(nullptr),
      m_pBrushOverlayMaterial(nullptr),
      m_eActiveWidget(EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT),
      m_brushColor(1.0f, 1.0f, 1.0f),
      m_brushRadius(2.0f),
      m_brushFalloff(0.2f),
      m_brushTerrainStep(1.0f),
      m_brushTerrainMinHeight(0.0f),
      m_brushTerrainMaxHeight(0.0f),
      m_brushLayerStep(1.0f),
      m_brushLayerSelectedBaseLayer(-1),
      m_eCurrentState(STATE_NONE),
      m_mouseRayHitLocation(float3::Infinite),
      m_mouseOverClosestPoint(int2::Zero),
      m_mouseOverClosestPointPosition(float3::Zero),
      m_validMouseOverPosition(false),
      m_mouseOverSection(int2::Zero),
      m_validMouseOverSection(false),
      m_selectedSection(int2::Zero),
      m_validSelectedSection(false)
{

}

EditorTerrainEditMode::~EditorTerrainEditMode()
{
    delete m_ui;

    // clear the callbacks
    m_pTerrainData->SetEditCallbacks(nullptr);
    DeleteSectionRenderProxies();

    SAFE_RELEASE(m_pBrushOverlayMaterial);
}

void EditorTerrainEditMode::SetActiveWidget(EDITOR_TERRAIN_EDIT_MODE_WIDGET widget)
{
    if (m_eActiveWidget == widget)
    {
        // ensure the widget stays active
        m_ui->OnWidgetChanged(m_eActiveWidget);
        return;
    }

    ClearState();
    m_eActiveWidget = widget;
    m_ui->OnWidgetChanged(m_eActiveWidget);
    m_pMap->GetMapWindow()->RedrawAllViewports();
}

void EditorTerrainEditMode::SetBrushColor(const float3 &color)
{
    m_brushColor = color;
    m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushColor", SHADER_PARAMETER_TYPE_FLOAT3, &m_brushColor);
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::SetBrushRadius(float radius)
{
    m_brushRadius = radius;
    m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushRadius", SHADER_PARAMETER_TYPE_FLOAT, &m_brushRadius);
    UpdateBrushVisual();
    m_pMap->RedrawAllViewports();

    Log_InfoPrintf("Terrain brush radius now %s", StringConverter::FloatToString(radius).GetCharArray());
}

void EditorTerrainEditMode::SetBrushFalloff(float falloff)
{
    m_brushFalloff = falloff;
    m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushFalloff", SHADER_PARAMETER_TYPE_FLOAT, &m_brushFalloff);
    UpdateBrushVisual();
    m_pMap->RedrawAllViewports();

    Log_InfoPrintf("Terrain brush falloff now %s", StringConverter::FloatToString(falloff).GetCharArray());
}

void EditorTerrainEditMode::SetBrushLayerSelectedLayer(int32 selectedLayer)
{
    m_brushLayerSelectedBaseLayer = selectedLayer;
    if (m_brushLayerSelectedBaseLayer >= 0)
    {
        const TerrainLayerListGenerator::BaseLayer *pBaseLayer = m_pMap->GetMapSource()->GetTerrainData()->GetLayerListGenerator()->GetBaseLayer(selectedLayer);
        if (pBaseLayer != nullptr)
            Log_InfoPrintf("Terrain brush is now painting base layer '%s'", pBaseLayer->Name.GetCharArray());
        else
            m_brushLayerSelectedBaseLayer = -1;
    }
}

bool EditorTerrainEditMode::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    if (!EditorEditMode::Initialize(pProgressCallbacks))
        return false;

    // get data handle
    m_pTerrainData = m_pMap->GetMapSource()->GetTerrainData();
    DebugAssert(m_pTerrainData != nullptr);

    // create renderer
    m_pTerrainRenderer = TerrainRenderer::CreateTerrainRenderer(m_pTerrainData->GetParameters(), m_pTerrainData->GetLayerList());
    DebugAssert(m_pTerrainRenderer != nullptr);

    // create overlay material
    {
        AutoReleasePtr<const MaterialShader> pBrushOverlayMaterialShader = g_pResourceManager->GetMaterialShader("materials/editor/terrain_brush_overlay");
        if (pBrushOverlayMaterialShader == NULL)
            return false;

        m_pBrushOverlayMaterial = new Material();
        m_pBrushOverlayMaterial->Create("materials/editor/terrain_brush_overlay", pBrushOverlayMaterialShader);
        m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushCenter", SHADER_PARAMETER_TYPE_FLOAT3, &m_mouseOverClosestPointPosition);
        m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushRadius", SHADER_PARAMETER_TYPE_FLOAT, &m_brushRadius);
        m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushFalloff", SHADER_PARAMETER_TYPE_FLOAT, &m_brushFalloff);
        m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushColor", SHADER_PARAMETER_TYPE_FLOAT3, &m_brushColor);

        float brushRadiusForShader = m_brushRadius * (float)m_pTerrainData->GetParameters()->Scale;
        m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushRadius", SHADER_PARAMETER_TYPE_FLOAT, &brushRadiusForShader);
    }

    // set the callbacks and create any proxies for currently-loaded sections
    CreateSectionRenderProxies();
    m_pTerrainData->SetEditCallbacks(static_cast<MapSourceTerrainData::EditCallbacks *>(this));

    // load all sections [just for now until streaming]
    // this will also create the section render proxies
    if (!m_pTerrainData->LoadAllSections(pProgressCallbacks))
        return false;

    return true;
}

void EditorTerrainEditMode::Activate()
{
    EditorEditMode::Activate();
}

void EditorTerrainEditMode::Deactivate()
{
    // clear state variables
    ClearState();

    EditorEditMode::Deactivate();
}

void EditorTerrainEditMode::Update(const float timeSinceLastUpdate)
{
    EditorEditMode::Update(timeSinceLastUpdate);
}

void EditorTerrainEditMode::OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport)
{
    EditorEditMode::OnActiveViewportChanged(pOldActiveViewport, pNewActiveViewport);
}

bool EditorTerrainEditMode::HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent)
{
    // pass to camera
    if (pViewport->GetViewController().HandleKeyboardEvent(pKeyboardEvent))
        return true;

    return EditorEditMode::HandleViewportKeyboardInputEvent(pViewport, pKeyboardEvent);
}

bool EditorTerrainEditMode::HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent)
{
    if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton)
    {
        if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT ||
            m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS ||
            m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS)
        {
            BeginPaint();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton)
    {
        if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT ||
            m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS ||
            m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS)
        {
            EndPaint();
            return true;
        }
        else if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS)
        {
            // update the selected section
            m_selectedSection = m_mouseOverSection;
            m_validSelectedSection = m_validMouseOverSection;
            pViewport->FlagForRedraw();
            UpdateUIForSelectedSection();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::MiddleButton)
    {
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::MiddleButton)
    {
        SmallString message;
        if (!m_validMouseOverPosition)
            message.Format("invalid mouse over position");
        else
            message.Format("ray hit = [%f, %f, %f]\nclosest point = [%i, %i]\nposition = [%f, %f, %f]", m_mouseRayHitLocation.x, m_mouseRayHitLocation.y, m_mouseRayHitLocation.z, m_mouseOverClosestPoint.x, m_mouseOverClosestPoint.y, m_mouseOverClosestPointPosition.x, m_mouseOverClosestPointPosition.y, m_mouseOverClosestPointPosition.z);

        QMessageBox::information(m_pMap->GetMapWindow(), QStringLiteral("Hit information"), ConvertStringToQString(message));
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton)
    {
        // right mouse button can enter a viewport state. if we are not in a state, determine which state to enter.
        if (m_eCurrentState == STATE_NONE)
        {
            // right mouse button enters camera rotation state
            m_eCurrentState = STATE_CAMERA_ROTATION;

            // lock cursor, set cursor, and redraw
            pViewport->LockMouseCursor();
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_CROSS);
            pViewport->FlagForRedraw();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton)
    {
        // if in camera rotation state, exit it
        if (m_eCurrentState == STATE_CAMERA_ROTATION)
        {
            // exit the state
            m_eCurrentState = STATE_NONE;

            // reset viewport state
            pViewport->UnlockMouseCursor();
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
            pViewport->FlagForRedraw();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        // handle mouse movement based on state
        switch (m_eCurrentState)
        {
        case STATE_NONE:
            {
                if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT ||
                    m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS ||
                    m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS ||
                    m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HOLES)
                {
                    // update position
                    UpdateTerrainCoordinates(pViewport, pViewport->GetMousePosition(), false);
                }

                if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS)
                {
                    // update section
                    UpdateTerrainCoordinates(pViewport, pViewport->GetMousePosition(), true);
                }
            }
            break;

        case STATE_PAINT_TERRAIN_HEIGHT:
        case STATE_PAINT_TERRAIN_LAYER:
            {
                // update position, then paint it
                UpdateTerrainCoordinates(pViewport, pViewport->GetMousePosition(), false);
                UpdatePaint();
                pViewport->FlagForRedraw();
                return true;
            }
            break;

        case STATE_CAMERA_ROTATION:
            {
                // pass difference through to camera
                pViewport->GetViewController().RotateFromMousePosition(pViewport->GetMouseDelta());
                pViewport->FlagForRedraw();
            }
            break;
        }
    }

    return EditorEditMode::HandleViewportMouseInputEvent(pViewport, pMouseEvent);
}

bool EditorTerrainEditMode::HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent)
{
    return EditorEditMode::HandleViewportWheelInputEvent(pViewport, pWheelEvent);
}

void EditorTerrainEditMode::ClearState()
{
    m_eCurrentState = STATE_NONE;

    m_mouseRayHitLocation = float3::Infinite;
    m_mouseOverClosestPoint.SetZero();
    m_mouseOverClosestPointPosition = float3::Infinite;
    m_validMouseOverPosition = false;

    m_mouseOverSection.SetZero();
    m_validMouseOverSection = false;

    m_selectedSection.SetZero();
    m_validSelectedSection = false;
}

void EditorTerrainEditMode::OnViewportDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterWorld(pViewport);

    switch (m_eActiveWidget)
    {
    case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT:
    case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS:
    case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_DETAILS:
    case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HOLES:
        DrawPostWorldOverlaysTerrainWidget(pViewport);
        break;

    case EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_SECTIONS:
        DrawPostWorldOverlaysSectionWidget(pViewport);
        break;
    }
}

void EditorTerrainEditMode::OnViewportDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawBeforeWorld(pViewport);
}

void EditorTerrainEditMode::OnViewportDrawAfterPost(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterPost(pViewport);
}

void EditorTerrainEditMode::OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawAfterWorld(pViewport);
}

void EditorTerrainEditMode::OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawBeforeWorld(pViewport);
}

void EditorTerrainEditMode::UpdateDetailLayerList()
{
    // preview image size
    static const uint32 PREVIEW_IMAGE_SIZE = 48;

    // clear it to begin with
    m_ui->EditLayersPanelLayerList->clear();
    m_ui->EditLayersPanelLayerList->setIconSize(QSize(PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE));

    // reset the layer to paint
    m_brushLayerSelectedBaseLayer = -1;

    // iterate over layers
    const TerrainLayerListGenerator *pLayerListGenerator = m_pMap->GetMapSource()->GetTerrainData()->GetLayerListGenerator();
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        const TerrainLayerListGenerator::BaseLayer *pBaseLayer = pLayerListGenerator->GetBaseLayer(i);
        if (pBaseLayer == nullptr)
            continue;

        // paint this layer if it's the first one
        if (m_brushLayerSelectedBaseLayer < 0)
            m_brushLayerSelectedBaseLayer = (int32)i;

        // should have a base map
        DebugAssert(pBaseLayer->pBaseMap != nullptr);

        // convert it to the preview size
        Image resizedImage;
        if (!resizedImage.CopyAndResize(*pBaseLayer->pBaseMap, IMAGE_RESIZE_FILTER_LANCZOS3, PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE, 1))
        {
            resizedImage.Create(PIXEL_FORMAT_R8G8B8_UNORM, PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE, 1);
            Y_memset(resizedImage.GetData(), 0xFF, resizedImage.GetDataSize());
        }

        // load it into qt
        QPixmap layerPixmap;
        EditorHelpers::ConvertImageToQPixmap(resizedImage, &layerPixmap);

        // create an icon
        QIcon layerIcon;
        layerIcon.addPixmap(layerPixmap);

        // add it to the list
        m_ui->EditLayersPanelLayerList->addItem(new QListWidgetItem(layerIcon, ConvertStringToQString(pBaseLayer->Name)));
    }

    // set to first in the list
    if (m_ui->EditLayersPanelLayerList->count() > 0)
        m_ui->EditLayersPanelLayerList->setCurrentRow(0);
}

void EditorTerrainEditMode::UpdateTerrainCoordinates(EditorMapViewport *pViewport, const int2 &mousePosition, bool sectionOnly)
{
    //Timer ut;

    // get world pick ray
    Ray worldRay(pViewport->GetViewController().GetPickRay(mousePosition.x, mousePosition.y));

    // fastpath?
    if (!sectionOnly)
    {
        // issue raycast
        float3 contactPoint, contactNormal;
        if (m_pTerrainData->RayCast(worldRay, contactNormal, contactPoint))
        {
            // transform back to world coordinates
            if (!m_validMouseOverPosition || !m_validMouseOverSection || contactPoint != m_mouseRayHitLocation)
            {
                // derive section from point
                // calculate indexed position
                //Log_DevPrintf("world over pos: %f %f %f", contactPoint.x, contactPoint.y, contactPoint.z);
                m_mouseRayHitLocation = contactPoint;
                m_mouseOverClosestPoint = m_pTerrainData->CalculatePointForPosition(contactPoint);
                m_mouseOverClosestPointPosition = m_pTerrainData->CalculatePositionForPoint(m_mouseOverClosestPoint.x, m_mouseOverClosestPoint.y);
                m_validMouseOverPosition = true;

                // update overlay material
                m_pBrushOverlayMaterial->SetShaderUniformParameterByName("BrushCenter", SHADER_PARAMETER_TYPE_FLOAT3, &m_mouseRayHitLocation);
                UpdateBrushVisual();
                
                // redraw
                pViewport->FlagForRedraw();
            }

            // is valid
            //Log_ProfilePrintf("terrain intersection took %.4f msec", ut.GetTimeMilliseconds());
            return;
        }
    }
    else
    {
        float3 bestContactPoint, bestContactNormal;
        float3 contactPoint, contactNormal;
        float bestContactDistance = Y_FLT_INFINITE;
        float contactDistance;

        // null the position vars
        m_brushVertices.Clear();

        // issue raycast against the terrain
        if (m_pTerrainData->RayCast(worldRay, contactNormal, contactPoint))
        {
            // store this contact
            bestContactDistance = (contactPoint - worldRay.GetOrigin()).SquaredLength();
            bestContactPoint = contactPoint;
            bestContactNormal = contactNormal;
        }

        // intersect the ground plane with the camera ray, then use these coordinates to determine the section the mouse is under
        Plane groundPlane(float3::UnitZ, (float)m_pTerrainData->GetParameters()->BaseHeight);
        if (worldRay.PlaneIntersection(groundPlane, contactNormal, contactPoint))
        {
            contactDistance = (contactPoint - worldRay.GetOrigin()).SquaredLength();
            if (contactDistance < bestContactDistance)
            {
                bestContactDistance = contactDistance;
                bestContactPoint = contactPoint;
                bestContactNormal = contactNormal;
            }
        }

        // the camera could be under the terrain, try a plane in the opposite direction
        Plane reversedPlane(-groundPlane.GetNormal(), -groundPlane.GetDistance());
        if (worldRay.PlaneIntersection(reversedPlane, contactNormal, contactPoint))
        {
            contactDistance = (contactPoint - worldRay.GetOrigin()).SquaredLength();
            if (contactDistance < bestContactDistance)
            {
                bestContactDistance = contactDistance;
                bestContactPoint = contactPoint;
                bestContactNormal = contactNormal;
            }
        }

        // hit anything?
        if (bestContactDistance != Y_FLT_INFINITE)
        {
            // set the position
            if (!m_validMouseOverPosition || !m_validMouseOverSection || contactPoint != m_mouseRayHitLocation)
            {
                m_mouseRayHitLocation = contactPoint;
                m_mouseOverClosestPoint = m_pTerrainData->CalculatePointForPosition(contactPoint);
                m_mouseOverClosestPointPosition = m_pTerrainData->CalculatePositionForPoint(m_mouseOverClosestPoint.x, m_mouseOverClosestPoint.y);
                m_validMouseOverPosition = true;
                pViewport->FlagForRedraw();
            }

            // set the section
            int2 mouseUnderSection(m_pTerrainData->CalculateSectionForPosition(contactPoint));
            if (!m_validMouseOverSection || m_mouseOverSection != mouseUnderSection)
            {
                m_mouseOverSection = mouseUnderSection;
                m_validMouseOverSection = true;
                pViewport->FlagForRedraw();
                UpdateUIForSelectedSection();
            }

            // hit something
            //Log_ProfilePrintf("terrain intersection took %.4f msec", ut.GetTimeMilliseconds());
            return;
        }
    }
    
    if (m_validMouseOverPosition || m_validMouseOverSection)
    {
        m_mouseRayHitLocation = float3::Infinite;
        m_mouseOverClosestPoint.SetZero();
        m_validMouseOverPosition = false;
        m_mouseOverSection.SetZero();
        m_validMouseOverSection = false;
        UpdateBrushVisual();
        pViewport->FlagForRedraw();
        UpdateUIForSelectedSection();
    }

    //Log_ProfilePrintf("terrain intersection took %.4f msec", ut.GetTimeMilliseconds());
}

void EditorTerrainEditMode::UpdateBrushVisual()
{
    m_brushVertices.Clear();

    if (!m_validMouseOverPosition)
        return;

    float3 brushCenter(m_mouseRayHitLocation);
    float3 brushMinBounds(brushCenter - m_brushRadius);
    float3 brushMaxBounds(brushCenter + m_brushRadius);
    AABox brushBoundingBox(brushMinBounds, brushMaxBounds);
    m_pTerrainData->EnumerateSectionsOverlappingBox(brushBoundingBox, [this, &brushBoundingBox](const TerrainSection *pSection)
    {
        pSection->EnumerateTrianglesIntersectingBox(brushBoundingBox, [this](const float3 vertices[3])
        {
            PlainVertexFactory::Vertex vertex;
            for (uint32 i = 0; i < 3; i++)
            {
                vertex.Position = vertices[i];
                vertex.TexCoord.SetZero();
                vertex.Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
                m_brushVertices.Add(vertex);
            }
        });
    });

    m_pMap->RedrawAllViewports();
}

float EditorTerrainEditMode::GetBrushPaintAmount(int32 pointX, int32 pointY) const
{
    // calculate weight
    float2 fClosestPoint((float)m_mouseOverClosestPoint.x, (float)m_mouseOverClosestPoint.y);
    float2 pointPosition(TerrainUtilities::CalculatePositionForPoint(m_pTerrainData->GetParameters(), pointX, pointY).xy());
    float2 weightVector((fClosestPoint - pointPosition) / m_brushRadius);
    float weight = Math::Pow(1.0f - Math::Clamp(weightVector.SquaredLength(), 0.0f, 1.0f), m_brushFalloff);
    //Log_DevPrintf("p[%i,%i] weight = %f", pointX, pointY, weight);

    return weight;
}

bool EditorTerrainEditMode::BeginPaint()
{
    if (!m_validMouseOverPosition)
        return false;

    // depending on widget
    if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_HEIGHT)
    {
        // get the height of the closest point, this becomes the 'low' height range
        float closestPointHeight = GetPointHeight(m_mouseOverClosestPoint.x, m_mouseOverClosestPoint.y);
        m_brushTerrainMinHeight = closestPointHeight;

        // low + step = high
        m_brushTerrainMaxHeight = closestPointHeight + m_brushTerrainStep;

        // set state
        m_eCurrentState = STATE_PAINT_TERRAIN_HEIGHT;

        // bring up the current mouse position's height
        UpdatePaint();

        // ok!
        m_pMap->RedrawAllViewports();
        return true;
    }
    else if (m_eActiveWidget == EDITOR_HEIGHTFIELD_TERRAIN_EDIT_MODE_WIDGET_EDIT_LAYERS)
    {
        // has a layer selected
        if (m_brushLayerSelectedBaseLayer < 0)
            return false;

        // start it
        m_eCurrentState = STATE_PAINT_TERRAIN_LAYER;
        UpdatePaint();

        // ok!
        m_pMap->RedrawAllViewports();
        return true;
    }

    return false;
}

void EditorTerrainEditMode::UpdatePaint()
{
    if (!m_validMouseOverPosition)
        return;

    float brushRadiusInPoints = m_brushRadius / (float)m_pTerrainData->GetParameters()->Scale;
    int32 iBrushRadiusInPoints = (int32)Math::Ceil(brushRadiusInPoints);

    // find points in range of cursor
    int2 startPoint = m_mouseOverClosestPoint - iBrushRadiusInPoints;
    int2 endPoint = m_mouseOverClosestPoint + iBrushRadiusInPoints;

    // depending on widget
    if (m_eCurrentState == STATE_PAINT_TERRAIN_HEIGHT)
    {
        // use the brush paint height
        float heightLow = m_brushTerrainMinHeight;
        float heightHigh = m_brushTerrainMaxHeight;
        float heightRange = heightHigh - heightLow;

        // iterate over points
        for (int32 pointY = startPoint.y; pointY <= endPoint.y; pointY++)
        {
            for (int32 pointX = startPoint.x; pointX <= endPoint.x; pointX++)
            {
                // is height in range to be modified
                float currentHeight = GetPointHeight(pointX, pointY);
                //if (currentHeight > m_brushTerrainMinHeight)
                    //continue;

                // calculate weight
                float weight = GetBrushPaintAmount(pointX, pointY);
                float newHeight = heightLow + weight * heightRange;
                //Log_DevPrintf("p[%i,%i] weight = %f, height = %f", pointX, pointY, weight, newHeight);

                // apply height
                if (currentHeight < newHeight)
                    SetPointHeight(pointX, pointY, newHeight);
            }
        }

        // update brush since the height changed
        UpdateBrushVisual();
    }
    else if (m_eCurrentState == STATE_PAINT_TERRAIN_LAYER)
    {
        DebugAssert(m_brushLayerSelectedBaseLayer >= 0);

        // iterate over points
        for (int32 pointY = startPoint.y; pointY <= endPoint.y; pointY++)
        {
            for (int32 pointX = startPoint.x; pointX <= endPoint.x; pointX++)
            {
                float weight = GetBrushPaintAmount(pointX, pointY);
                //Log_DevPrintf("p[%i,%i] weight = %f", pointX, pointY, weight);

                // is height in range to be modified
                float currentWeight = GetPointLayerWeight(pointX, pointY, (uint8)m_brushLayerSelectedBaseLayer);
                if (currentWeight < weight)
                    SetPointLayerWeight(pointX, pointY, (uint8)m_brushLayerSelectedBaseLayer, m_brushLayerStep * weight);
            }
        }
    }
}

void EditorTerrainEditMode::EndPaint()
{
    m_brushTerrainMinHeight = 0.0f;
    m_brushTerrainMaxHeight = 0.0f;
    m_eCurrentState = STATE_NONE;
    m_pMap->RedrawAllViewports();
}

void EditorTerrainEditMode::DrawPostWorldOverlaysSectionWidget(EditorMapViewport *pViewport)
{
    MiniGUIContext &guiContext = pViewport->GetGUIContext();

    // draw the bounding box of the section with the mouse over
    if (m_validMouseOverSection && (!m_validSelectedSection || m_selectedSection != m_mouseOverSection))
    {
        AABox sectionBounds(TerrainUtilities::CalculateSectionBoundingBox(m_pTerrainData->GetParameters(), m_mouseOverSection.x, m_mouseOverSection.y));
        const float3 &sectionMinBounds = sectionBounds.GetMinBounds();
        const float3 &sectionMaxBounds = sectionBounds.GetMaxBounds();
        float baseZ = (float)m_pTerrainData->GetParameters()->BaseHeight;

        guiContext.SetDepthTestingEnabled(false);
        guiContext.SetAlphaBlendingEnabled(false);
        guiContext.Draw3DRect(float3(sectionMinBounds.x, sectionMaxBounds.y, baseZ),
                              float3(sectionMinBounds.x, sectionMinBounds.y, baseZ),
                              float3(sectionMaxBounds.x, sectionMinBounds.y, baseZ),
                              float3(sectionMaxBounds.x, sectionMaxBounds.y, baseZ),
                              MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    }

    // and the selected section
    if (m_validSelectedSection)
    {
        AABox sectionBounds(TerrainUtilities::CalculateSectionBoundingBox(m_pTerrainData->GetParameters(), m_selectedSection.x, m_selectedSection.y));
        const float3 &sectionMinBounds = sectionBounds.GetMinBounds();
        const float3 &sectionMaxBounds = sectionBounds.GetMaxBounds();
        float baseZ = (float)m_pTerrainData->GetParameters()->BaseHeight;

        guiContext.SetDepthTestingEnabled(false);
        guiContext.SetAlphaBlendingEnabled(false);
        guiContext.Draw3DRect(float3(sectionMinBounds.x, sectionMaxBounds.y, baseZ),
                              float3(sectionMinBounds.x, sectionMinBounds.y, baseZ),
                              float3(sectionMaxBounds.x, sectionMinBounds.y, baseZ),
                              float3(sectionMaxBounds.x, sectionMaxBounds.y, baseZ),
                              MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255));
    }
}

void EditorTerrainEditMode::DrawPostWorldOverlaysTerrainWidget(EditorMapViewport *pViewport)
{
    if (m_brushVertices.GetSize() > 0)
    {
        GPUContext *pContext = g_pRenderer->GetMainContext();
        
        pContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
        pContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
        pContext->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);

        //g_pRenderer->DrawPlainColored(pContext, m_brushVertices.GetBasePointer(), m_brushVertices.GetSize());
        //g_pRenderer->DrawMaterialPlainUserPointer(pContext, m_pBrushOverlayMaterial, m_brushVertices.GetBasePointer(), m_brushVertices.GetSize());
    }
}

void EditorTerrainEditMode::UpdateUIForSelectedSection()
{
    if (m_validSelectedSection)
    {
        m_ui->EditSectionsNoSelection->hide();
        m_ui->EditSectionsSectionInfo->show();
        m_ui->EditSectionsSectionInfoSectionX->setText(ConvertStringToQString(StringConverter::Int32ToString(m_selectedSection.x)));
        m_ui->EditSectionsSectionInfoSectionY->setText(ConvertStringToQString(StringConverter::Int32ToString(m_selectedSection.y)));

        if (m_pTerrainData->IsSectionAvailable(m_selectedSection.x, m_selectedSection.y))
        {
            m_ui->EditSectionsActiveSection->show();
            m_ui->EditSectionsActiveSectionOps->show();
            m_ui->EditSectionsInactiveSection->hide();
            m_ui->EditSectionsInactiveSectionOps->hide();

            const TerrainSection *pSection = m_pTerrainData->GetSection(m_selectedSection.x, m_selectedSection.y);
            if (pSection != nullptr)
            {
                m_ui->EditSectionsActiveSectionNodeCount->setText(ConvertStringToQString(StringConverter::Int32ToString(pSection->GetQuadTree()->GetNodeCount())));
                m_ui->EditSectionsActiveSectionSplatMapCount->setText(ConvertStringToQString(StringConverter::Int32ToString(pSection->GetSplatMapCount())));
            }
        }
        else
        {
            m_ui->EditSectionsActiveSection->hide();
            m_ui->EditSectionsActiveSectionOps->hide();
            m_ui->EditSectionsInactiveSection->show();
            m_ui->EditSectionsInactiveSectionOps->show();
        }
    }
    else
    {
        // mouse over?
        if (m_validMouseOverSection)
        {
            m_ui->EditSectionsNoSelection->hide();
            m_ui->EditSectionsSectionInfo->show();
            m_ui->EditSectionsSectionInfoSectionX->setText(ConvertStringToQString(StringConverter::Int32ToString(m_mouseOverSection.x)));
            m_ui->EditSectionsSectionInfoSectionY->setText(ConvertStringToQString(StringConverter::Int32ToString(m_mouseOverSection.y)));
        }
        else
        {
            m_ui->EditSectionsNoSelection->show();
            m_ui->EditSectionsSectionInfo->hide();
        }

        m_ui->EditSectionsActiveSection->hide();
        m_ui->EditSectionsActiveSectionOps->hide();
        m_ui->EditSectionsInactiveSection->hide();
        m_ui->EditSectionsInactiveSectionOps->hide();
    }
}

bool EditorTerrainEditMode::CreateSection(int32 sectionX, int32 sectionY, float createHeight, uint8 createLayer)
{
    if (m_pTerrainData->IsSectionAvailable(sectionX, sectionY))
        return false;

    EditorProgressDialog progressDialog(m_pMap->GetMapWindow());
    progressDialog.show();

    if (!m_pTerrainData->CreateSection(sectionX, sectionY, createHeight, createLayer, &progressDialog))
        return false;

    // update bounding box of map
    m_pMap->MergeMapBoundingBox(m_pTerrainData->CalculateTerrainBoundingBox());
    return true;
}

bool EditorTerrainEditMode::DeleteSection(int32 sectionX, int32 sectionY)
{
    if (!m_pTerrainData->IsSectionAvailable(sectionX, sectionY))
        return false;

    EditorProgressDialog progressDialog(m_pMap->GetMapWindow());
    progressDialog.show();

    m_pTerrainData->DeleteSection(sectionX, sectionY, &progressDialog);

    return true;
}

bool EditorTerrainEditMode::ImportHeightmap(const Image *pHeightmap, int32 startSectionX, int32 startSectionY, float minHeight, float maxHeight, MapSourceTerrainData::HeightmapImportScaleType scaleType, uint32 scaleAmount, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // delete all section render proxies, and unset the callback interface. this way it'll be a lot faster.
    DeleteSectionRenderProxies();
    m_pTerrainData->SetEditCallbacks(nullptr);

    // run the import
    bool importResult = m_pTerrainData->ImportHeightmap(pHeightmap, startSectionX, startSectionY, minHeight, maxHeight, scaleType, scaleAmount, pProgressCallbacks);

    // re-create section render proxies, and the callback interface
    CreateSectionRenderProxies();
    m_pTerrainData->SetEditCallbacks(static_cast<MapSourceTerrainData::EditCallbacks *>(this));

    // update bounding box of map
    if (importResult)
        m_pMap->MergeMapBoundingBox(m_pTerrainData->CalculateTerrainBoundingBox());

    // done
    return importResult;
}

bool EditorTerrainEditMode::RebuildQuadTree(uint32 newLodCount, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // delete all section render proxies, and unset the callback interface. this way it'll be a lot faster.
    DeleteSectionRenderProxies();
    m_pTerrainData->SetEditCallbacks(nullptr);

    // run the rebuild
    bool rebuildResult = m_pTerrainData->RebuildQuadTree(newLodCount, pProgressCallbacks);

    // re-create section render proxies, and the callback interface
    CreateSectionRenderProxies();
    m_pTerrainData->SetEditCallbacks(static_cast<MapSourceTerrainData::EditCallbacks *>(this));

    // done
    return rebuildResult;
}

void EditorTerrainEditMode::OnSectionCreated(int32 sectionX, int32 sectionY)
{
    Log_DevPrintf("EditorTerrainEditMode::OnSectionCreated: Section created: [%i, %i]", sectionX, sectionY);
}

void EditorTerrainEditMode::OnSectionDeleted(int32 sectionX, int32 sectionY)
{
    Log_DevPrintf("EditorTerrainEditMode::OnSectionDeleted: Section deleted: [%i, %i]", sectionX, sectionY);
}

void EditorTerrainEditMode::OnSectionLoaded(const TerrainSection *pSection)
{
    Log_DevPrintf("EditorTerrainEditMode::OnSectionLoaded: Section loaded: [%i, %i]", pSection->GetSectionX(), pSection->GetSectionY());

    // shouldn't have a render proxy for it
    DebugAssert(GetSectionRenderProxy(pSection->GetSectionX(), pSection->GetSectionY()) == nullptr);

    // so create one
    TerrainSectionRenderProxy *pRenderProxy = m_pTerrainRenderer->CreateSectionRenderProxy(0, pSection);
    DebugAssert(pRenderProxy != nullptr);

    // and add it to the world
    m_pMap->GetRenderWorld()->AddRenderable(pRenderProxy);
    m_sectionRenderProxies.Add(pRenderProxy);
}

void EditorTerrainEditMode::OnSectionUnloaded(const TerrainSection *pSection)
{
    Log_DevPrintf("EditorTerrainEditMode::OnSectionUnloaded: Section unloaded: [%i, %i]", pSection->GetSectionX(), pSection->GetSectionY());

    // do we have a render proxy for it?
    for (uint32 i = 0; i < m_sectionRenderProxies.GetSize(); i++)
    {
        TerrainSectionRenderProxy *pRenderProxy = m_sectionRenderProxies[i];
        if (pRenderProxy->GetSection() == pSection)
        {
            // remove it
            m_sectionRenderProxies.OrderedRemove(i);

            m_pMap->GetRenderWorld()->RemoveRenderable(pRenderProxy);
            pRenderProxy->Release();
            break;
        }
    }

    // shouldn't have any other proxies
    DebugAssert(GetSectionRenderProxy(pSection->GetSectionX(), pSection->GetSectionY()));
}

void EditorTerrainEditMode::OnSectionLayersModified(const TerrainSection *pSection)
{
    // have a proxy?
    TerrainSectionRenderProxy *pRenderProxy = GetSectionRenderProxy(pSection);
    if (pRenderProxy != nullptr)
        pRenderProxy->OnLayersModified();
}

void EditorTerrainEditMode::OnSectionPointHeightModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY)
{
    // have a proxy?
    TerrainSectionRenderProxy *pRenderProxy = GetSectionRenderProxy(pSection);
    if (pRenderProxy != nullptr)
        pRenderProxy->OnPointHeightModified(offsetX, offsetY);
}

void EditorTerrainEditMode::OnSectionPointLayersModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY)
{
    // have a proxy?
    TerrainSectionRenderProxy *pRenderProxy = GetSectionRenderProxy(pSection);
    if (pRenderProxy != nullptr)
        pRenderProxy->OnPointLayersModified(offsetX, offsetY);
}

TerrainSectionRenderProxy *EditorTerrainEditMode::GetSectionRenderProxy(const TerrainSection *pSection)
{
    for (uint32 i = 0; i < m_sectionRenderProxies.GetSize(); i++)
    {
        if (m_sectionRenderProxies[i]->GetSection() == pSection)
            return m_sectionRenderProxies[i];
    }

    return nullptr;
}

TerrainSectionRenderProxy *EditorTerrainEditMode::GetSectionRenderProxy(int32 sectionX, int32 sectionY)
{
    for (uint32 i = 0; i < m_sectionRenderProxies.GetSize(); i++)
    {
        if (m_sectionRenderProxies[i]->GetSectionX() == sectionX && m_sectionRenderProxies[i]->GetSectionY() == sectionY)
            return m_sectionRenderProxies[i];
    }

    return nullptr;
}

bool EditorTerrainEditMode::CreateSectionRenderProxy(int32 sectionX, int32 sectionY)
{
    DebugAssert(GetSectionRenderProxy(sectionX, sectionY) == nullptr);

    const TerrainSection *pSection = m_pTerrainData->GetSection(sectionX, sectionY);
    DebugAssert(pSection != nullptr);

    TerrainSectionRenderProxy *pRenderProxy = m_pTerrainRenderer->CreateSectionRenderProxy(0, pSection);
    if (pRenderProxy == nullptr)
        return false;

    m_pMap->GetRenderWorld()->AddRenderable(pRenderProxy);
    m_sectionRenderProxies.Add(pRenderProxy);
    return true;
}

void EditorTerrainEditMode::DeleteSectionRenderProxy(int32 sectionX, int32 sectionY)
{
    for (uint32 i = 0; i < m_sectionRenderProxies.GetSize(); i++)
    {
        TerrainSectionRenderProxy *pRenderProxy = m_sectionRenderProxies[i];

        if (pRenderProxy->GetSectionX() == sectionX && pRenderProxy->GetSectionY() == sectionY)
        {
            m_sectionRenderProxies.OrderedRemove(i);

            m_pMap->GetRenderWorld()->RemoveRenderable(pRenderProxy);
            pRenderProxy->Release();

            return;
        }
    }
}

bool EditorTerrainEditMode::CreateSectionRenderProxies()
{
    bool result = true;

    m_pTerrainData->EnumerateLoadedSections([this, &result](const TerrainSection *pSection)
    {
        if (GetSectionRenderProxy(pSection->GetSectionX(), pSection->GetSectionY()) != nullptr)
            return;

        TerrainSectionRenderProxy *pRenderProxy = m_pTerrainRenderer->CreateSectionRenderProxy(0, pSection);
        if (pRenderProxy == nullptr)
        {
            result = false;
            return;
        }

        m_pMap->GetRenderWorld()->AddRenderable(pRenderProxy);
        m_sectionRenderProxies.Add(pRenderProxy);
    });

    return result;
}

void EditorTerrainEditMode::DeleteSectionRenderProxies()
{
    for (uint32 i = 0; i < m_sectionRenderProxies.GetSize(); i++)
    {
        TerrainSectionRenderProxy *pRenderProxy = m_sectionRenderProxies[i];
        m_pMap->GetRenderWorld()->RemoveRenderable(pRenderProxy);
        pRenderProxy->Release();
    }
    m_sectionRenderProxies.Obliterate();
}
