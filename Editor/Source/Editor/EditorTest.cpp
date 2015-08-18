#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorTerrainEditMode.h"
#include "Editor/EditorResourceSelectionDialog.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/BlockMeshEditor/EditorBlockMeshEditor.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/StaticMeshEditor/EditorStaticMeshEditor.h"
#include "Editor/SkeletalAnimationEditor/EditorSkeletalAnimationEditor.h"
#include "Editor/ResourceBrowser/EditorStaticMeshImportDialog.h"
#include "Engine/ResourceManager.h"
#include "Engine/TerrainRendererCDLOD.h"
#include "Engine/TerrainQuadTree.h"
#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
Log_SetChannel(EditorTest);

#if Y_COMPILER_MSVC
    #pragma warning(disable: 4505)          // warning C4505: 'TestEditorResourceSelectionDialog' : unreferenced local function has been removed
#endif

static void TestEditorResourceSelectionDialog()
{
    EditorResourceSelectionDialog *dlg = new EditorResourceSelectionDialog(NULL);
    dlg->setModal(true);
    dlg->exec();

    if (dlg->GetReturnValueResourceType() != NULL)
        Log_DevPrintf("Selected %s %s", dlg->GetReturnValueResourceType()->GetTypeName(), dlg->GetReturnValueResourceName().GetCharArray());
    else
        Log_DevPrintf("Selected nothing");

    delete dlg;
}

static void OpenAMap()
{
    EditorMapWindow *pMapWindow = g_pEditor->GetMapWindow(0);
    DebugAssert(pMapWindow != NULL && pMapWindow->IsMapOpen());
    pMapWindow->CloseMap();

#if defined(Y_PLATFORM_WINDOWS)
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Data\\testgame\\maps\\empty.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Data\\testgame\\maps\\test_terrain.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Documentation\\MCWorlds\\maps\\mc_test7.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Data\\testgame\\maps\\sponza.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Data\\testgame\\maps\\blah.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\Data\\testgame\\maps\\test_terrain.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\test_entity.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\mctest3.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\mctest3_l.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\onemesh.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\test_terrain3.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\platformer-set1.map");
    //pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\jazz_o.map");
    pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\test_objects.map");
#elif defined(Y_PLATFORM_LINUX)
    //pMapWindow->OpenMap("/home/user/dev/yage/Data/testgame/maps/empty.map");
    //pMapWindow->OpenMap("/home/user/dev/yage/Documentation/MCWorlds/maps.mc_test7.map");
    //pMapWindow->OpenMap("/home/user/dev/yage/Data/testgame/maps/sponza.map");
#endif

    if (!pMapWindow->IsMapOpen())
        pMapWindow->NewMap();
}

static void OpenSponzaMap()
{
    EditorMapWindow *pMapWindow = g_pEditor->GetMapWindow(0);
    DebugAssert(pMapWindow != NULL && pMapWindow->IsMapOpen());
    pMapWindow->CloseMap();

#if defined(Y_PLATFORM_WINDOWS)
    pMapWindow->OpenMap("D:\\Projects\\GameEngineDev\\TestGame\\Data\\maps\\sponza.map");
#elif defined(Y_PLATFORM_LINUX)
    pMapWindow->OpenMap("/home/user/dev/yage/Data/testgame/maps/sponza.map");
#endif
}

static void SetRendererPlatform()
{
#if defined(Y_PLATFORM_WINDOWS)
    //g_pConsole->SetCVarByName("r_platform", "D3D11");
    //g_pConsole->SetCVarByName("r_d3d11_force_ref", "1");
    //g_pConsole->SetCVarByName("r_d3d11_force_warp", "1");
    //g_pConsole->SetCVarByName("r_platform", "OpenGL");

    //g_pConsole->SetCVarByName("r_dump_shaders", "1");
    g_pConsole->SetCVarByName("r_dump_shaders", "0");
#elif defined(Y_PLATFORM_LINUX)
    //g_pConsole->SetCVarByName("r_platform", "OpenGL");
    //g_pConsole->SetCVarByName("r_dump_shaders", "1");
    g_pConsole->SetCVarByName("r_dump_shaders", "0");
#endif
}

static void DecompressImageFileToB64(const char *path, const char *outPath)
{
    AutoReleasePtr<ByteStream> pStream = FileSystem::OpenFile(path, BYTESTREAM_OPEN_READ);
    if (pStream == NULL)
        return;

    ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(path, pStream);
    if (pCodec == NULL)
        return;

    Image outImage;
    if (!pCodec->DecodeImage(&outImage, path, pStream) ||
        !outImage.ConvertPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM))
    {
        return;
    }

    uint32 len = Y_getencodedbase64length(outImage.GetDataSize());
    String base64;
    base64.Resize(len);
    uint32 outlen = Y_makebase64(outImage.GetData(), outImage.GetDataSize(), base64, base64.GetLength() + 1);
    if (outlen != len)
        return;

    AutoReleasePtr<ByteStream> pOutStream = FileSystem::OpenFile(outPath, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE);
    if (pOutStream == NULL)
        return;

    pOutStream->Write2(base64.GetCharArray(), base64.GetLength());
}

static void EmptyTerrain()
{
    EditorMapWindow *pMapWindow = g_pEditor->GetMapWindow(0);
    DebugAssert(pMapWindow != NULL && pMapWindow->IsMapOpen());

    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 128, 4, Y_INT32_MIN, Y_INT32_MAX, 0))
    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 512, 5, -4000, 4000, 0))
    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 128, 4, -500, 500, 0))
    if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 128, 5, -500, 500, 0))
    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 512, 5, 0, 1000, 0))
        return;

    EditorTerrainEditMode *pTerrainEditMode = pMapWindow->GetMap()->GetTerrainEditMode();
    DebugAssert(pTerrainEditMode != nullptr);

    // create center section
    pTerrainEditMode->CreateSection(0, 0, 0.0f, 0);
    //pTerrainEditMode->CreateSection(1, 0, 0.0f, 0);
    //pTerrainEditMode->CreateSection(0, 1, 0.0f, 0);
    //pTerrainEditMode->CreateSection(1, 1, 0.0f, 0);

    //g_pConsole->SetCVarByName("r_terrain_show_nodes", "1");

    //pMapWindow->GetMap()->GetTerrainEditMode()->SetPointHeight(1, 1, 2.0f);
    //pMapWindow->GetMap()->GetTerrainEditMode()->SetPointHeight(1, 2, 2.0f);
    //pMapWindow->GetMap()->GetTerrainEditMode()->SetPointHeight(2, 1, 2.0f);
    //pMapWindow->GetMap()->GetTerrainEditMode()->SetPointHeight(2, 2, 2.0f);

    pMapWindow->GetViewport(0)->GetViewController().SetPerspectiveMaxSpeed(10.0f);
    pMapWindow->GetViewport(0)->GetViewController().SetDrawDistance(400.0f);
}

static void SetHeights()
{
    const float heightStep = 1.0f;
    const int32 layerCountToSplit = 3;

    EditorMapWindow *pMapWindow = g_pEditor->GetMapWindow(0);
    DebugAssert(pMapWindow != NULL && pMapWindow->IsMapOpen());

    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 128, 4, Y_INT32_MIN, Y_INT32_MAX, 0))
    if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 512, 5, -4000, 4000, 0))
    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 128, 4, -500, 500, 0))
    //if (!pMapWindow->GetMap()->CreateTerrain(nullptr, TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, 1, 8, 1, -100, 100, 0))
        return;

    MapSourceTerrainData *pTerrainData = pMapWindow->GetMap()->GetMapSource()->GetTerrainData();
    if (pTerrainData == NULL)
        return;

    EditorProgressDialog progressDialog(pMapWindow);
    progressDialog.show();

    // create center section
    pTerrainData->CreateSection(0, 0, 0.0f, 0, &progressDialog);
    pTerrainData->CreateSection(1, 0, 0.0f, 0, &progressDialog);
    /*for (uint32 i = 0; i < 4; i++)
    {
        for (uint32 j = 0; j < 4; j++)
            pTerrainManager->CreateSection(j, i, &progressDialog);
    }*/

    int32 terrainMinX = pTerrainData->GetMinSectionX() * (int32)pTerrainData->GetParameters()->SectionSize;
    int32 terrainMinY = pTerrainData->GetMinSectionY() * (int32)pTerrainData->GetParameters()->SectionSize;
    int32 terrainSizeX = pTerrainData->GetSectionCountX() * (int32)pTerrainData->GetParameters()->SectionSize;
    int32 terrainSizeY = pTerrainData->GetSectionCountY() * (int32)pTerrainData->GetParameters()->SectionSize;
    int32 layerInterval = terrainSizeY / layerCountToSplit + 1;

    float currentHeight = 0.0f;
    uint8 currentLayer = 0;

    progressDialog.SetStatusText("Filling terrain...");
    progressDialog.SetProgressRange(terrainSizeY);
    progressDialog.SetProgressValue(0);

    
    for (int32 y = 0; y < terrainSizeY; y++)
    {
        for (int32 x = 0; x < terrainSizeX; x++)
        {
            //int32 x =0;
            pTerrainData->SetPointHeight(x + terrainMinX, y + terrainMinY, currentHeight);
            pTerrainData->AddPointLayerWeight(x + terrainMinX, y + terrainMinY, currentLayer, 1.0f);
        }

        if (currentLayer != 1)
            currentHeight += heightStep;

        if (y > 0 && (y % layerInterval) == 0)
            currentLayer++;

        progressDialog.IncrementProgressValue();
    }
    

    pTerrainData->RebuildQuadTree(pTerrainData->GetParameters()->LODCount, &progressDialog);

    g_pConsole->SetCVarByName("r_terrain_show_nodes", "1");

    pMapWindow->GetViewport(0)->GetViewController().SetPerspectiveMaxSpeed(10.0f);
}

static void TestStaticBlockMeshEditor()
{
    EditorBlockMeshEditor *pEditor = new EditorBlockMeshEditor();
    //if (!pEditor->Create(g_pResourceManager->GetDefaultBlockMeshBlockList()))
        //Panic("fail");

    //EditorProgressDialog progressDialog(pEditor);
    //progressDialog.show();

    if (!pEditor->Load("models/test/house"))
    //if (!pEditor->Load("models/test/test2", &progressDialog))
        Panic("Fail");

    pEditor->show();
    pEditor->SetWidget(EditorBlockMeshEditor::WIDGET_PLACE_BLOCKS);
    pEditor->SetPlaceBlockWidgetBlockType(1);
}

static void TestStaticMeshEditor()
{
    EditorStaticMeshEditor *pEditor = new EditorStaticMeshEditor();
    //pEditor->Create("models/test/test");
    //pEditor->Create("models/test/house");
    if (pEditor->Load("models/suburb_assets/house_mid"))
        pEditor->show();
    //else
        //delete pEditor;
}

static void TestSkeletalAnimationEditor()
{
    EditorSkeletalAnimationEditor *editor = new EditorSkeletalAnimationEditor();
    if (editor->Load("models/ninja/ninja_jump"))
    //if (editor->Load("models/mario/mario"))
        editor->show();
    else
        delete editor;
}

static void TestStaticMeshImporter()
{
    EditorStaticMeshImportDialog *id = new EditorStaticMeshImportDialog(nullptr);
    id->show();
}

void RunEditorTestBedsPreStartup()
{
    SetRendererPlatform();
}

void RunEditorTestBeds()
{
    //TestEditorResourceSelectionDialog();

    //DecompressImageFileToB64("D:\\Untitled.png", "D:\\untitled.txt");
    //DecompressImageFileToB64("D:\\Untitled2.png", "D:\\untitled2.txt");
    //DecompressImageFileToB64("D:\\Untitled3.png", "D:\\untitled3.txt");

    //OpenAMap();
    //EmptyTerrain();
    //SetHeights();


    //TestStaticBlockMeshEditor();
    //TestStaticMeshEditor();
    //TestSkeletalAnimationEditor();
    //OpenSponzaMap();
}

