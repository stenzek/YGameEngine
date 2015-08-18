#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorGeometryEditMode.h"

// TODO: MOVE ME
// static void CreateCubeBuilderBrush(BrushSource *pBrushSource, float Radius)
// {
// 	uint32 i;
// 	BrushSource::FaceSource *pFaceSources[6];
// 
// 	pBrushSource->ClearFaces();
// 	for (i = 0; i < 6; i++)
//     {
//         pFaceSources[i] = new BrushSource::FaceSource();
//         pFaceSources[i]->MaterialName = g_pEngine->GetDefaultMaterialName();
//         pFaceSources[i]->Vertices.Reserve(4);
//         pBrushSource->AddFace(pFaceSources[i]);
//     }
// 
//     float halfRadius = Radius * 0.5f;
// 
//     // top
//     pFaceSources[0]->TextureOrigin.Set(-halfRadius, halfRadius, -halfRadius);
//     pFaceSources[0]->TextureUDirection.Set(1.0f, 0.0f, 0.0f);
//     pFaceSources[0]->TextureVDirection.Set(0.0f, 0.0f, 1.0f);
//     pFaceSources[0]->Vertices.Add(Vector3(-halfRadius, halfRadius, -halfRadius));
//     pFaceSources[0]->Vertices.Add(Vector3(-halfRadius, halfRadius, halfRadius));
//     pFaceSources[0]->Vertices.Add(Vector3(halfRadius, halfRadius, halfRadius));
//     pFaceSources[0]->Vertices.Add(Vector3(halfRadius, halfRadius, -halfRadius));
// 
//     // bottom
//     pFaceSources[1]->TextureOrigin.Set(-halfRadius, -halfRadius, -halfRadius);
//     pFaceSources[1]->TextureUDirection.Set(1.0f, 0.0f, 0.0f);
//     pFaceSources[1]->TextureVDirection.Set(0.0f, 0.0f, 1.0f);
//     pFaceSources[1]->Vertices.Add(Vector3(-halfRadius, -halfRadius, -halfRadius));
//     pFaceSources[1]->Vertices.Add(Vector3(halfRadius, -halfRadius, -halfRadius));
//     pFaceSources[1]->Vertices.Add(Vector3(halfRadius, -halfRadius, halfRadius));
//     pFaceSources[1]->Vertices.Add(Vector3(-halfRadius, -halfRadius, halfRadius));
// 
//     // left side
//     pFaceSources[2]->TextureOrigin.Set(-halfRadius, halfRadius, -halfRadius);
//     pFaceSources[2]->TextureUDirection.Set(0.0f, 0.0f, 1.0f);
//     pFaceSources[2]->TextureVDirection.Set(0.0f, -1.0f, 0.0f);
//     pFaceSources[2]->Vertices.Add(Vector3(-halfRadius, halfRadius, -halfRadius));
//     pFaceSources[2]->Vertices.Add(Vector3(-halfRadius, -halfRadius, -halfRadius));
//     pFaceSources[2]->Vertices.Add(Vector3(-halfRadius, -halfRadius, halfRadius));
//     pFaceSources[2]->Vertices.Add(Vector3(-halfRadius, halfRadius, halfRadius));
// 
//     // right side
//     pFaceSources[3]->TextureOrigin.Set(halfRadius, halfRadius, -halfRadius);
//     pFaceSources[3]->TextureUDirection.Set(0.0f, 0.0f, 1.0f);
//     pFaceSources[3]->TextureVDirection.Set(0.0f, -1.0f, 0.0f);
//     pFaceSources[3]->Vertices.Add(Vector3(halfRadius, halfRadius, -halfRadius));
//     pFaceSources[3]->Vertices.Add(Vector3(halfRadius, halfRadius, halfRadius));
//     pFaceSources[3]->Vertices.Add(Vector3(halfRadius, -halfRadius, halfRadius));
//     pFaceSources[3]->Vertices.Add(Vector3(halfRadius, -halfRadius, -halfRadius));
// 
//     // front
//     pFaceSources[4]->TextureOrigin.Set(-halfRadius, halfRadius, halfRadius);
//     pFaceSources[4]->TextureUDirection.Set(1.0f, 0.0f, 0.0f);
//     pFaceSources[4]->TextureVDirection.Set(0.0f, -1.0f, 0.0f);
//     pFaceSources[4]->Vertices.Add(Vector3(-halfRadius, halfRadius, halfRadius));
//     pFaceSources[4]->Vertices.Add(Vector3(-halfRadius, -halfRadius, halfRadius));
//     pFaceSources[4]->Vertices.Add(Vector3(halfRadius, -halfRadius, halfRadius));
//     pFaceSources[4]->Vertices.Add(Vector3(halfRadius, halfRadius, halfRadius));
// 
//     // back
//     pFaceSources[5]->TextureOrigin.Set(-halfRadius, halfRadius, -halfRadius);
//     pFaceSources[5]->TextureUDirection.Set(1.0f, 0.0f, 0.0f);
//     pFaceSources[5]->TextureVDirection.Set(0.0f, -1.0f, 0.0f);
//     pFaceSources[5]->Vertices.Add(Vector3(-halfRadius, halfRadius, -halfRadius));
//     pFaceSources[5]->Vertices.Add(Vector3(halfRadius, halfRadius, -halfRadius));
//     pFaceSources[5]->Vertices.Add(Vector3(halfRadius, -halfRadius, -halfRadius));
//     pFaceSources[5]->Vertices.Add(Vector3(-halfRadius, -halfRadius, -halfRadius));
// }

EditorGeometryEditMode::EditorGeometryEditMode(EditorMap *pMap)
    : EditorEditMode(pMap)
{

}

EditorGeometryEditMode::~EditorGeometryEditMode()
{
    // release references to our objects
    //SAFE_RELEASE(m_pBuilderBrushEntity);
    //SAFE_RELEASE(m_pBuilderBrush);
}

void EditorGeometryEditMode::SetBuilderBrushCube(const float &Radius)
{

}

void EditorGeometryEditMode::PerformCSGAdd()
{
//     if (m_pBuilderBrush == NULL)
//         return;
// 
//     // generate new entity id
//     uint32 mapEntityId = m_pMapSource->GenerateObjectId();
//     uint32 worldEntityId = m_pWorld->AllocateEntityId();
// 
//     // create a new brush, cloning the state of the builder brush, and insert it into the csg tree
//     CSGCompilerBrush *pNewBrush = new CSGCompilerBrush();
//     pNewBrush->CopyBrush(m_pBuilderBrush, mapEntityId, CSG_OPERATOR_ADD, "Brush");
//     m_pCSGCompiler->AddBrush(pNewBrush, NULL);
//     
//     // create the source for the brush, and add it to the map source
//     EntitySource *pEntitySource = new EntitySource();
//     pEntitySource->SetBrushSource(new BrushSource());
//     pNewBrush->UpdateSource(pEntitySource);
//     m_pMapSource->AddEntityDefinition(pEntitySource);
// 
//     // create entity
//     EditorBrushEntity *pEditorBrushEntity = new EditorBrushEntity(worldEntityId, pNewBrush);
//     m_pWorld->AddEntity(pEditorBrushEntity);
//     AddEntityIdMapping(mapEntityId, worldEntityId);
//     pEditorBrushEntity->Release();
// 
//     // free mem
//     pNewBrush->Release();
// 
//     // redraw
//     RedrawAllViewports();
}

void EditorGeometryEditMode::PerformCSGSubtract()
{

}

void EditorGeometryEditMode::PerformCSGIntersection()
{

}

void EditorGeometryEditMode::PerformCSGDeintersection()
{

}

bool EditorGeometryEditMode::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    if (!EditorEditMode::Initialize(pProgressCallbacks))
        return false;

//     // builder brush
//     {
//         m_pBuilderBrush = m_pCSGCompiler->GetBrush(BUILDER_BRUSH_ENTITY_ID);
//         if (m_pBuilderBrush == NULL)
//         {
//             Log_ErrorPrintf("Could not find builder brush in CSG compiler.");
//             return false;
//         }
//     }
// 
//     // builder brush entity
//     {
//         uint32 worldEntityId = LookupWorldEntityId(BUILDER_BRUSH_ENTITY_ID);
//         if (worldEntityId == 0)
//         {
//             Log_ErrorPrintf("Could not find builder brush entity in mapping table.");
//             return false;
//         }
// 
//         Entity *pEntity = m_pWorld->GetEntityById(worldEntityId);
//         if (pEntity == NULL)
//         {
//             Log_ErrorPrintf("Could not find builder brush entity in world.");
//             return false;
//         }
// 
//         m_pBuilderBrushEntity = pEntity->SafeCast<EditorBuilderBrushEntity>();
//         if (m_pBuilderBrushEntity == NULL)
//         {
//             Log_ErrorPrintf("Builder brush is of incorrect type. (%s)", pEntity->GetTypeInfo()->GetName());
//             pEntity->Release();
//             return false;
//         }
//     }

    return true;
}

void EditorGeometryEditMode::Activate()
{
    EditorEditMode::Activate();
}

void EditorGeometryEditMode::Deactivate()
{
    EditorEditMode::Deactivate();
}

void EditorGeometryEditMode::Update(const float timeSinceLastUpdate)
{
    EditorEditMode::Update(timeSinceLastUpdate);
}

void EditorGeometryEditMode::OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport)
{
    EditorEditMode::OnActiveViewportChanged(pOldActiveViewport, pNewActiveViewport);
}

bool EditorGeometryEditMode::HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent)
{
    return EditorEditMode::HandleViewportKeyboardInputEvent(pViewport, pKeyboardEvent);
}

bool EditorGeometryEditMode::HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent)
{
    return EditorEditMode::HandleViewportMouseInputEvent(pViewport, pMouseEvent);
}

bool EditorGeometryEditMode::HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent)
{
    return EditorEditMode::HandleViewportWheelInputEvent(pViewport, pWheelEvent);
}

void EditorGeometryEditMode::OnViewportDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterWorld(pViewport);
}

void EditorGeometryEditMode::OnViewportDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawBeforeWorld(pViewport);
}

void EditorGeometryEditMode::OnViewportDrawAfterPost(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterPost(pViewport);
}

void EditorGeometryEditMode::OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawAfterWorld(pViewport);
}

void EditorGeometryEditMode::OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawBeforeWorld(pViewport);
}
