#include "PrecompiledHeader.h"
#include "ContentConverter.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintAssimpSkeletonImporterSyntax()
{
    Log_InfoPrint("Assimp Skeleton Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -o <path>: Output resource name.");
    Log_InfoPrint("  -rotate(X|Y|Z) <amount>: Rotate around the axis as a post-transform.");
    Log_InfoPrint("  -translate(X|Y|Z) <degrees>: Translate along the axis as a post-transform.");
    Log_InfoPrint("  -scale[X|Y|Z] <amount>: Uniform or nonuniform scale on axis as a post-transform.");
    Log_InfoPrint("  -cs <(yup|zup)_(lh|rh)>: File uses specified coordinate system, convert if necessary.");
    Log_InfoPrint("");
}

bool ParseAssimpSkeletonImporterOption(AssimpSkeletonImporter::Options &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.SourcePath = argv[++i];
    else if (CHECK_ARG_PARAM("-o"))
        Options.OutputResourceName = argv[++i];
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
        PrintAssimpSkeletonImporterSyntax();
        return false;
    }
    else
    {
        Log_ErrorPrintf("Unknown option: %s", argv[i]);
        return false;
    }

    return true;
}

int RunAssimpSkeletonImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintAssimpSkeletonImporterSyntax();
        return 0;
    }

    AssimpSkeletonImporter::Options options;
    AssimpSkeletonImporter::SetDefaultOptions(&options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseAssimpSkeletonImporterOption(options, i, argc, argv))
            return 1;
    }

    if (options.SourcePath.IsEmpty() || options.OutputResourceName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name or output name.");
        return 1;
    }

    {
        ConsoleProgressCallbacks progressCallbacks;
        AssimpSkeletonImporter importer(&options, &progressCallbacks);

        if (!importer.Execute())
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

