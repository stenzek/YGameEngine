#include "Editor/PrecompiledHeader.h"
#include "Editor/MaterialShaderEditor/EditorMaterialShaderEditor.h"
#include "Editor/Editor.h"
#include "Engine/Material.h"
#include "Engine/Entity.h"
#include "ResourceCompiler/ShaderGraph.h"
Log_SetChannel(EditorMaterialShaderEditor);

EditorMaterialShaderEditor::EditorMaterialShaderEditor(EditorMaterialShaderEditorCallbacks *pCallbacks)
{
    m_pCallbacks = pCallbacks;

    m_pShaderGenerator = NULL;

    m_pPreviewMaterialShader = NULL;
    m_pPreviewMaterial = NULL;
    m_bRegeneratePreviewMaterial = true;
}

EditorMaterialShaderEditor::~EditorMaterialShaderEditor()
{
    //if (m_pPreviewMaterialShader != NULL)
        //m_pPreviewMaterialShader->SetSource(NULL);

    SAFE_RELEASE(m_pPreviewMaterialShader);
    SAFE_RELEASE(m_pPreviewMaterial);

    delete m_pShaderGenerator;
}

bool EditorMaterialShaderEditor::LoadShader(const char *fileName, ByteStream *pStream)
{
    Assert(m_pShaderGenerator == NULL);

    MaterialShaderGenerator *pSource = new MaterialShaderGenerator();
    if (!pSource->LoadFromXML(nullptr, fileName, pStream))
    {
        delete pSource;
        return false;
    }

    m_pShaderGenerator = pSource;
    RegeneratePreviewMaterials();
    return true;
}

void EditorMaterialShaderEditor::CreateShader()
{
    Assert(m_pShaderGenerator == NULL);
    m_pShaderGenerator = new MaterialShaderGenerator();
    m_pShaderGenerator->Create(nullptr);
}

bool EditorMaterialShaderEditor::SaveShader(ByteStream *pStream)
{
    return m_pShaderGenerator->SaveToXML(pStream);
}

void EditorMaterialShaderEditor::SetBlendingMode(MATERIAL_BLENDING_MODE BlendingMode, bool SupressCallbacks /*= false*/)
{
    m_pShaderGenerator->SetBlendMode(BlendingMode);
    if (!SupressCallbacks)
        m_pCallbacks->OnMaterialPropertiesChanged();
}

void EditorMaterialShaderEditor::SetLightingMode(MATERIAL_LIGHTING_MODEL LightingMode, bool SupressCallbacks /*= false*/)
{
    m_pShaderGenerator->SetLightingModel(LightingMode);
    if (!SupressCallbacks)
        m_pCallbacks->OnMaterialPropertiesChanged();
}

void EditorMaterialShaderEditor::SetDoubleSidedLighting(bool Enabled, bool SupressCallbacks /*= false*/)
{
    m_pShaderGenerator->SetTwoSided(Enabled);
    if (!SupressCallbacks)
        m_pCallbacks->OnMaterialPropertiesChanged();
}

bool EditorMaterialShaderEditor::AddUniformParameter(const char *ParameterName, const char *ParameterDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RenameUniformParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::ChangeUniformParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RemoveUniformParameter(const char *ParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::AddTextureParameter(const char *ParameterName, TEXTURE_TYPE textureType, const char *ParameterDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RenameTextureParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::ChangeTextureParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RemoveTextureParameter(const char *ParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::AddStaticSwitchParameter(const char *ParameterName, const char *ParameterDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RenameStaticSwitchParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::ChangeStaticSwitchParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks /*= false*/)
{
    return false;
}

bool EditorMaterialShaderEditor::RemoveStaticSwitchParameter(const char *ParameterName, bool SupressCallbacks /*= false*/)
{
    return false;
}

const MaterialShader *EditorMaterialShaderEditor::GetPreviewMaterialShader() const
{
    if (m_bRegeneratePreviewMaterial && !RegeneratePreviewMaterials())
        return NULL;

    return m_pPreviewMaterialShader;
}

const Material *EditorMaterialShaderEditor::GetPreviewMaterial() const
{
    if (m_bRegeneratePreviewMaterial && !RegeneratePreviewMaterials())
        return NULL;

    return m_pPreviewMaterial;
}

bool EditorMaterialShaderEditor::RegeneratePreviewMaterials() const
{
//     if (m_pPreviewMaterialShader != NULL)
//         m_pPreviewMaterialShader->SetSource(NULL);
// 
//     SAFE_RELEASE(m_pPreviewMaterialShader);
//     SAFE_RELEASE(m_pPreviewMaterial);
//     m_bRegeneratePreviewMaterial = true;
// 
//     MaterialShader *pMaterialShader = new MaterialShader();
//     if (!pMaterialShader->Create("MATERIAL_SHADER_EDITOR_PREVIEW", m_pShaderGenerator))
//     {
//         pMaterialShader->Release();
//         return false;
//     }
// 
//     pMaterialShader->SetSource(m_pShaderGenerator);
// 
//     Material *pMaterial = new Material();
//     pMaterial->Create("MATERIAL_SHADER_EDITOR_PREVIEW", pMaterialShader);
//     
//     m_pPreviewMaterialShader = pMaterialShader;
//     m_pPreviewMaterial = pMaterial;
//     m_bRegeneratePreviewMaterial = false;
//     return true;
    return false;
}
