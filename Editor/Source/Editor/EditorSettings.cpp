#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorSettings.h"
#include "Editor/Editor.h"

static EditorSettings s_EditorSettings;
EditorSettings *g_pEditorSettings = &s_EditorSettings;

EditorSettings::EditorSettings()
{
    m_strBuilderBrushWireframeMaterialName = "materials/editor/builder_brush_wireframe";
    m_strBrushWireframeMaterialName = "materials/editor/brush_wireframe";
    m_strVolumeWireframeMaterialName = "materials/editor/volume_wireframe";
    m_strAxisMeshName = "models/editor/axis";
	m_eViewportLayout = EDITOR_VIEWPORT_LAYOUT_1X1;
}

EditorSettings::~EditorSettings()
{

}

