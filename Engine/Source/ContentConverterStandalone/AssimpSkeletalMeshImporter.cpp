#include "PrecompiledHeader.h"
#include "ContentConverter.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintAssimpSkeletalMeshImporterSyntax()
{
    Log_InfoPrint("Assimp Skeletal Mesh Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -skeleton <path>: Skeleton to use.");
    Log_InfoPrint("  -o <path>: Output resource name.");
    Log_InfoPrint("  -[no]materials: Enable/disable material importing");
    Log_InfoPrint("  -materialdir: Directory for writing materials to");
    Log_InfoPrint("  -materialprefix: Prefix for material names");
    Log_InfoPrint("  -defaultmaterialname: Default material name");
    Log_InfoPrint("  -flipwinding: Flip the triangle winding order");
    Log_InfoPrint("  -cs <(yup|zup)_(lh|rh)>: OBJ uses specified coordinate system, convert if necessary.");
    Log_InfoPrint("  -collision: Collision shape type (box|sphere|trianglemesh|convexhull)");
    Log_InfoPrint("");
}

bool ParseAssimpSkeletalMeshImporterOption(AssimpSkeletalMeshImporter::Options &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.SourcePath = argv[++i];
    else if (CHECK_ARG_PARAM("-o"))
        Options.OutputResourceName = argv[++i];
    else if (CHECK_ARG_PARAM("-skeleton"))
        Options.SkeletonName = argv[++i];
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
    else if (CHECK_ARG("-flipwinding"))
        Options.FlipFaceOrder = !Options.FlipFaceOrder;
    else if (CHECK_ARG_PARAM("-collision"))
        Options.CollisionShapeType = argv[++i];
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
    else if (CHECK_ARG("-h") || CHECK_ARG("-help"))
    {
        i = argc;
        PrintAssimpSkeletalMeshImporterSyntax();
        return false;
    }
    else
    {
        Log_ErrorPrintf("Unknown option: %s", argv[i]);
        return false;
    }

    return true;
}

int RunAssimpSkeletalMeshImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintAssimpSkeletalMeshImporterSyntax();
        return 0;
    }

    AssimpSkeletalMeshImporter::Options options;
    AssimpSkeletalMeshImporter::SetDefaultOptions(&options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseAssimpSkeletalMeshImporterOption(options, i, argc, argv))
            return 1;
    }

    if (options.SourcePath.IsEmpty() || options.OutputResourceName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name or output name.");
        return 1;
    }

    {
        ConsoleProgressCallbacks progressCallbacks;
        AssimpSkeletalMeshImporter importer(&options, &progressCallbacks);

        if (!importer.Execute())
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

