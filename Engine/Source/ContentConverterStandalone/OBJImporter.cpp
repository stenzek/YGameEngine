#include "PrecompiledHeader.h"
#include "ContentConverter.h"
#include "ContentConverter/OBJImporter.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintOBJImporterSyntax()
{
    Log_InfoPrint("OBJ Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -basedir <name>: Output base directory.");
    Log_InfoPrint("  -name <name>: Output name. Required if -nogroupmeshes. A full path is required if you don't specify -meshpath.");
    Log_InfoPrint("  -meshpath <name>: Mesh path. Required if -groupmeshes or -genmap are specified.");
    Log_InfoPrint("  -materialpath <name>: Material output path. Defaults to mesh path.");
    Log_InfoPrint("  -texturepath <name>: Material output path. Defaults to mesh path.");
    Log_InfoPrint("  -[no]importmaterials: Generate materials for OBJ materials, default on");
    Log_InfoPrint("  -[no]overwritematerials: Overwrite existing materials when importing, default off");
    Log_InfoPrint("  -[no]importtextures: Convert textures for OBJ materials, default on");
    Log_InfoPrint("  -[no]overwritetextures: Overwrite existing textures when importing materials, default off");
    Log_InfoPrint("  -teximport [option] [option value]: Specify texture converter argument.");
    Log_InfoPrint("  -[no]groupmeshes: Use OBJ face groups as meshes, default off");
    Log_InfoPrint("  -[no]center: Center mesh at (0, 0, 0), default off");
    Log_InfoPrint("  -[no]genmap <name>: Generate a map with all meshes, default off. Implies -center.");
    Log_InfoPrint("  -[no]smoothinggroups: Generate normals using smoothing groups from obj, default on");
    Log_InfoPrint("  -scale: Global uniform scale all vertices.");
    Log_InfoPrint("  -rotate(X|Y|Z) <amount>: Rotate around the axis as a post-transform.");
    Log_InfoPrint("  -translate(X|Y|Z) <degrees>: Translate along the axis as a post-transform.");
    Log_InfoPrint("  -scale[X|Y|Z] <amount>: Uniform or nonuniform scale on axis as a post-transform.");
    Log_InfoPrint("  -flipwinding: Flip the triangle winding order");
    Log_InfoPrint("  -cs <(yup|zup)_(lh|rh)>: File uses specified coordinate system, convert if necessary.");
    Log_InfoPrint("  -collision: Defines collision shape type, defaults to none. (trianglemesh, convexhull)");
    Log_InfoPrint("");
}

bool ParseOBJConverterOption(OBJImporterOptions &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.InputFileName = argv[++i];
    else if (CHECK_ARG_PARAM("-meshdir"))
        Options.MeshDirectory = argv[++i];
    else if (CHECK_ARG_PARAM("-meshprefix"))
        Options.MeshPrefix = argv[++i];
    else if (CHECK_ARG_PARAM("-genmap"))
        Options.OutputMapName = argv[++i];
    else if (CHECK_ARG("-nomaterials"))
        Options.ImportMaterials = false;
    else if (CHECK_ARG("-materials"))
        Options.ImportMaterials = true;
    else if (CHECK_ARG_PARAM("-materialdir"))
        Options.MaterialDirectory = argv[++i];
    else if (CHECK_ARG_PARAM("-materialprefix"))
        Options.MaterialPrefix = argv[++i];
    else if (CHECK_ARG_PARAM("-defaultmaterialname"))
        Options.DefaultMaterialName = argv[++i];
    else if (CHECK_ARG_PARAM("-collision"))
        Options.BuildCollisionShapeType = argv[++i];
    else if (CHECK_ARG_PARAM("-rotateX"))
        Options.TransformMatrix = float4x4::MakeRotationMatrixX(StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-rotateY"))
        Options.TransformMatrix = float4x4::MakeRotationMatrixY(StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-rotateZ"))
        Options.TransformMatrix = float4x4::MakeRotationMatrixZ(StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-translateX"))
        Options.TransformMatrix = float4x4::MakeTranslationMatrix(StringConverter::StringToFloat(argv[++i]), 0.0f, 0.0f) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-translateY"))
        Options.TransformMatrix = float4x4::MakeTranslationMatrix(0.0f, StringConverter::StringToFloat(argv[++i]), 0.0f) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-translateZ"))
        Options.TransformMatrix = float4x4::MakeTranslationMatrix(0.0f, 0.0f, StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-scaleX"))
        Options.TransformMatrix = float4x4::MakeScaleMatrix(StringConverter::StringToFloat(argv[++i]), 0.0f, 0.0f) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-scaleY"))
        Options.TransformMatrix = float4x4::MakeScaleMatrix(0.0f, StringConverter::StringToFloat(argv[++i]), 0.0f) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-scaleZ"))
        Options.TransformMatrix = float4x4::MakeScaleMatrix(0.0f, 0.0f, StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG_PARAM("-scale"))
        Options.TransformMatrix = float4x4::MakeScaleMatrix(StringConverter::StringToFloat(argv[++i])) * Options.TransformMatrix;
    else if (CHECK_ARG("-center"))
        Options.CenterMeshes = StaticMeshGenerator::CenterOrigin_Center;
    else if (CHECK_ARG("-centerbase"))
        Options.CenterMeshes = StaticMeshGenerator::CenterOrigin_CenterBottom;
    else if (CHECK_ARG_PARAM("-cs"))
    {
        ++i;
        if (!Y_stricmp(argv[i], "yup_lh"))
            Options.CoordinateSystem = COORDINATE_SYSTEM_Y_UP_LH;
        else if (!Y_stricmp(argv[i], "yup_rh"))
            Options.CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
        else if (!Y_stricmp(argv[i], "zup_lh"))
            Options.CoordinateSystem = COORDINATE_SYSTEM_Z_UP_LH;
        else if (!Y_stricmp(argv[i], "zup_rh"))
            Options.CoordinateSystem = COORDINATE_SYSTEM_Z_UP_RH;
        else
        {
            Log_ErrorPrintf("Invalid coordinate system specified.");
            return false;
        }
    }
    else if (CHECK_ARG("-smoothinggroups"))
        Options.UseSmoothingGroups = true;
    else if (CHECK_ARG("-nosmoothinggroups"))
        Options.UseSmoothingGroups = false;
    else if (CHECK_ARG("-mergegroups"))
        Options.MergeGroups = true;
    else if (CHECK_ARG("-nomergegroups"))
        Options.MergeGroups = false;
    else if (CHECK_ARG_PARAM("-collision"))
        Options.BuildCollisionShapeType = argv[++i];
    else if (CHECK_ARG("-h") || CHECK_ARG("-help"))
    {
        i = argc;
        PrintOBJImporterSyntax();
        return false;
    }
    else
    {
        Log_ErrorPrintf("Unknown option: %s", argv[i]);
        return false;
    }

    return true;
}

int RunOBJImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintOBJImporterSyntax();
        return 0;
    }

    OBJImporterOptions Options;
    OBJImporter::SetDefaultOptions(&Options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseOBJConverterOption(Options, i, argc, argv))
            return 1;
    }

    if (Options.InputFileName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name.");
        return 1;
    }

    {
        ConsoleProgressCallbacks progressCallbacks;
        OBJImporter Importer(&progressCallbacks);

        if (!Importer.Execute(&Options))
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

