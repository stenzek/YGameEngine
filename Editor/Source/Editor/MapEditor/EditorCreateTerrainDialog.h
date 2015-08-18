#pragma once
#include "Editor/Common.h"
#include "Editor/MapEditor/ui_EditorCreateTerrainDialog.h"
#include "Engine/TerrainTypes.h"
#include "Engine/TerrainLayerList.h"

class EditorCreateTerrainDialog : public QDialog
{
    Q_OBJECT

public:
    EditorCreateTerrainDialog(QWidget *pParent, Qt::WindowFlags windowFlags = 0);
    ~EditorCreateTerrainDialog();

    const TerrainLayerList *GetLayerList() const { return m_pLayerList; }
    const uint32 GetSectionSize() const { return m_sectionSize; }
    const uint32 GetUnitsPerPoint() const { return m_unitsPerPoint; }
    const uint32 GetQuadTreeNodeSize() const { return m_quadTreeNodeSize; }
    const uint32 GetTextureRepeatInterval() const { return m_textureRepeatInterval; }
    const TERRAIN_HEIGHT_STORAGE_FORMAT GetStorageFormat() const { return m_storageFormat; }
    const int32 GetMinHeight() const { return m_minHeight; }
    const int32 GetMaxHeight() const { return m_maxHeight; }
    const int32 GetBaseHeight() const { return m_baseHeight; }
    const bool GetCreateCenterSection() const { return m_createCenterSection; }

public Q_SLOTS:

private Q_SLOTS:
    bool Validate();
    void OnLayerListChanged(const QString &value);
    void OnHeightFormatChanged(int currentIndex);
    void OnCreateButtonTriggered();
    void OnCancelButtonTriggered();

private:
    Ui_EditorCreateTerrainDialog *m_ui;

    const TerrainLayerList *m_pLayerList;
    uint32 m_sectionSize;
    uint32 m_unitsPerPoint;
    uint32 m_quadTreeNodeSize;
    uint32 m_textureRepeatInterval;
    TERRAIN_HEIGHT_STORAGE_FORMAT m_storageFormat;
    int32 m_minHeight;
    int32 m_maxHeight;
    int32 m_baseHeight;
    bool m_createCenterSection;
};

