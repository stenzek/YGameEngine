#pragma once
#include "Editor/Common.h"

class EditorSettings
{
public:
	EditorSettings();
	~EditorSettings();

	// getters
    const String &GetBuilderBrushWireframeMaterialName() const { return m_strBuilderBrushWireframeMaterialName; }
    const String &GetBrushWireframeMaterialName() const { return m_strBrushWireframeMaterialName; }
    const String &GetVolumeWireframeMaterialName() const { return m_strVolumeWireframeMaterialName; }
    const String &GetAxisMeshName() const { return m_strAxisMeshName; }
	EDITOR_VIEWPORT_LAYOUT GetViewportLayout() const { return m_eViewportLayout; }

	// setters
	void SetViewportLayout(EDITOR_VIEWPORT_LAYOUT ViewportLayout) { m_eViewportLayout = ViewportLayout; }

private:
    String m_strBuilderBrushWireframeMaterialName;
    String m_strBrushWireframeMaterialName;
    String m_strVolumeWireframeMaterialName;
    String m_strAxisMeshName;
	EDITOR_VIEWPORT_LAYOUT m_eViewportLayout;
};

extern EditorSettings *g_pEditorSettings;
