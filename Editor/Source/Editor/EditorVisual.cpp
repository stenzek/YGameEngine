#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorVisual.h"
#include "Editor/EditorVisualComponent.h"
#include "Editor/Editor.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "YBaseLib/XMLReader.h"
Log_SetChannel(EditorVisualDefinition);

static const char *MISSING_VISUAL_TYPE_NAME = "__missing__";
static const char *VISUAL_BASE_PATH = "resources/engine/editor_visuals";

EditorVisualDefinition::EditorVisualDefinition()
{

}

EditorVisualDefinition::~EditorVisualDefinition()
{
    for (uint32 i = 0; i < m_visualComponentDefinitions.GetSize(); i++)
        delete m_visualComponentDefinitions[i];
}

bool EditorVisualDefinition::CreateFromName(const char *name)
{
    bool loadMissing = false;
    PathString fileName;

    // init name
    m_strName = name;

    // load this name
    {
        // format filename
        fileName.Format("%s/%s.xml", VISUAL_BASE_PATH, name);

        // try opening it
        AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream != nullptr)
        {
            // try loading from stream
            if (!LoadComponentsFromStream(fileName, pStream))
                loadMissing = true;
        }
        else
        {
            Log_WarningPrintf("EditorVisualDefinition::CreateFromName: No visual template found for '%s'.", name);
            loadMissing = true;
        }
    }

    // have to load the missing type?
    if (loadMissing || m_visualComponentDefinitions.GetSize() == 0)
    {
        fileName.Format("%s/%s.xml", VISUAL_BASE_PATH, MISSING_VISUAL_TYPE_NAME);

        AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr || !LoadComponentsFromStream(fileName, pStream))
        {
            Log_ErrorPrintf("EditorVisualDefinition::CreateFromTemplate: Failed to load missing visual.");
            return false;
        }
    }

    Log_DevPrintf("Editor visual definition loaded: '%s' (%u visual elements)", m_strName.GetCharArray(), m_visualComponentDefinitions.GetSize());
    return true;
}

bool EditorVisualDefinition::CreateFromTemplate(const ObjectTemplate *pTemplate)
{
    bool loadMissing = false;
    PathString fileName;

    // init name
    m_strName = pTemplate->GetTypeName();

    // load each type in the inheritance chain
    {
        const ObjectTemplate *pCurrentType = pTemplate;
        while (pCurrentType != nullptr)
        {
            // format filename
            fileName.Format("%s/%s.xml", VISUAL_BASE_PATH, pCurrentType->GetTypeName().GetCharArray());

            // try opening it
            AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pStream != nullptr)
            {
                // try loading from stream
                if (!LoadComponentsFromStream(fileName, pStream))
                    loadMissing = true;
            }
            else
            {
                Log_WarningPrintf("EditorVisualDefinition::CreateFromTemplate: No visual template found for '%s'.", pCurrentType->GetTypeName().GetCharArray());
                loadMissing = true;
            }

            // go to next class in inheritance chain
            pCurrentType = pCurrentType->GetBaseClassTemplate();
        }
    }

    // have to load the missing type?
    if (loadMissing || m_visualComponentDefinitions.GetSize() == 0)
    {
        fileName.Format("%s/%s.xml", VISUAL_BASE_PATH, MISSING_VISUAL_TYPE_NAME);

        AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr || !LoadComponentsFromStream(fileName, pStream))
        {
            Log_ErrorPrintf("EditorVisualDefinition::CreateFromTemplate: Failed to load missing visual.");
            return false;
        }
    }

    Log_DevPrintf("Editor visual definition loaded: '%s' (%u visual elements)", m_strName.GetCharArray(), m_visualComponentDefinitions.GetSize());
    return true;
}

bool EditorVisualDefinition::LoadComponentsFromStream(const char *FileName, ByteStream *pStream)
{
    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        xmlReader.PrintError("could not create xml reader");
        return false;
    }

    // skip to entity-definition element
    if (!xmlReader.SkipToElement("editor-visual"))
    {
        xmlReader.PrintError("could not skip to editor-visual element");
        return false;
    }

//     // read attributes
//     const char *nameStr = xmlReader.FetchAttribute("name");
//     const char *baseStr = xmlReader.FetchAttribute("base");
//     const char *canCreateStr = xmlReader.FetchAttribute("can-create");
//     if (nameStr == NULL || canCreateStr == NULL)
//     {
//         xmlReader.PrintError("incomplete entity definition");
//         return false;
//     }

    // read elements
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 editorVisualSelection = xmlReader.Select("visual");
                if (editorVisualSelection < 0)
                    return false;

                switch (editorVisualSelection)
                {
                    // visual
                case 0:
                    {
                        int32 typeSelection = xmlReader.SelectAttribute("type", "Sprite|StaticMesh|BlockMesh|PointLight|DirectionalLight");
                        if (typeSelection < 0)
                            return false;

                        EditorVisualComponentDefinition *pVisualDefinition = NULL;
                        switch (typeSelection)
                        {
                            // Sprite
                        case 0:
                            pVisualDefinition = new EditorVisualComponentDefinition_Sprite();
                            break;

                            // StaticMesh
                        case 1:
                            pVisualDefinition = new EditorVisualComponentDefinition_StaticMesh();
                            break;

                            // StaticBlockMesh
                        case 2:
                            pVisualDefinition = new EditorVisualComponentDefinition_BlockMesh();
                            break;

                            // PointLight
                        case 3:
                            pVisualDefinition = new EditorVisualComponentDefinition_PointLight();
                            break;

                            // DirectionalLight
                        case 4:
                            pVisualDefinition = new EditorVisualComponentDefinition_DirectionalLight();
                            break;

                        default:
                            UnreachableCode();
                            break;
                        }

                        DebugAssert(pVisualDefinition != NULL);
                        if (!pVisualDefinition->ParseXML(xmlReader))
                        {
                            delete pVisualDefinition;
                            return false;
                        }

                        m_visualComponentDefinitions.Add(pVisualDefinition);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "editor-visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}


EditorVisualInstance::EditorVisualInstance(const EditorVisualDefinition *pDefinition)
    : m_pDefinition(pDefinition),
      m_pRenderWorld(nullptr),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero)
{

}

EditorVisualInstance::~EditorVisualInstance()
{
    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
    {
        if (m_pRenderWorld != nullptr)
            m_visualInstances[i]->OnRemovedFromRenderWorld(m_pRenderWorld);

        delete m_visualInstances[i];
    }
}

EditorVisualInstance *EditorVisualInstance::Create(uint32 pickingID, const EditorVisualDefinition *pDefinition, const PropertyTable *pInitialProperties /* = &PropertyTable::EmptyPropertyList */)
{
    EditorVisualInstance *pInstance = new EditorVisualInstance(pDefinition);

    // create visual components
    for (uint32 i = 0; i < pDefinition->GetVisualDefinitionCount(); i++)
    {
        const EditorVisualComponentDefinition *pVisualDefinition = pDefinition->GetVisualDefinition(i);
        EditorVisualComponentInstance *pVisualInstance = pVisualDefinition->CreateVisual(pickingID);
        if (pVisualInstance == nullptr)
            continue;

        // notify of all property values
        pInitialProperties->EnumerateProperties([pVisualInstance](const char *propertyName, const char *propertyValue) {
            pVisualInstance->OnPropertyChange(propertyName, propertyValue);
        });

        // add to list
        pInstance->m_visualInstances.Add(pVisualInstance);
    }

    // if no visuals were created, bail out
    if (pInstance->m_visualInstances.GetSize() == 0)
    {
        delete pInstance;
        return nullptr;
    }

    // update bounds, and done
    pInstance->UpdateBounds();
    return pInstance;
}

void EditorVisualInstance::AddToRenderWorld(RenderWorld *pWorld)
{
    DebugAssert(m_pRenderWorld == nullptr);
    m_pRenderWorld = pWorld;

    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
        m_visualInstances[i]->OnAddedToRenderWorld(pWorld);
}

void EditorVisualInstance::RemoveFromRenderWorld(RenderWorld *pWorld)
{
    DebugAssert(m_pRenderWorld == pWorld);

    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
        m_visualInstances[i]->OnRemovedFromRenderWorld(pWorld);

    m_pRenderWorld = nullptr;
}

void EditorVisualInstance::OnPropertyModified(const char *propertyName, const char *propertyValue)
{
    // pass message to render proxies
    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
    {
        // notify visual
        m_visualInstances[i]->OnPropertyChange(propertyName, propertyValue);
    }

    // update bounds
    UpdateBounds();
}

void EditorVisualInstance::OnSelectedStateChanged(bool selected)
{
    // pass message to render proxies
    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
    {
        // notify visual
        m_visualInstances[i]->OnSelectionStateChange(selected);
    }
}

void EditorVisualInstance::UpdateBounds()
{
    bool boundsSet = false;
    m_boundingBox = AABox::Zero;

    for (uint32 i = 0; i < m_visualInstances.GetSize(); i++)
    {
        const AABox &visualBounds = m_visualInstances[i]->GetVisualBounds();

        // skip bounds that are max size, it throws everything off
        if (visualBounds != AABox::MaxSize || visualBounds.GetMinBounds() == float3::NegativeInfinite || visualBounds.GetMaxBounds() == float3::Infinite)
        {
            if (boundsSet)
            {
                m_boundingBox.Merge(visualBounds);
            }
            else
            {
                boundsSet = true;
                m_boundingBox = visualBounds;
            }
        }
    }

    m_boundingSphere = Sphere::FromAABox(m_boundingBox);
}
