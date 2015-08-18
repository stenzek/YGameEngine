#include "PrecompiledHeader.h"
#include "ContentConverter.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintAssimpSkeletalAnimationImporterSyntax()
{
    Log_InfoPrint("Assimp Skeletal Animation Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -listanim: List animations, don't import anything");
    Log_InfoPrint("  -createskeleton <path>: Create skeleton from this animation.");
    Log_InfoPrint("  -skeleton <path>: Specify skeleton resource name. Required if not using -createskeleton.");
    Log_InfoPrint("  -oname <path>: Output resource name. Required if not using -all.");
    Log_InfoPrint("  -odir <path>: Output resource directory. Required if using -all.");
    Log_InfoPrint("  -oprefix <prefix>: Output resource prefix. Used for -all.");
    Log_InfoPrint("  -tps <tps>: Number of animation ticks per second if not specified in file. Default of 30.");
    Log_InfoPrint("  -forcetps <tps>: Force this number of ticks per second irregardless of what is specified in file.");
    Log_InfoPrint("  -all: Import all animations from file.");
    Log_InfoPrint("  -animname <name>: Import single animation, specify the name.");
    Log_InfoPrint("  -clip start:end: Clip animation to these frame numbers.");
    Log_InfoPrint("");
}

bool ParseAssimpSkeletalAnimationImporterOption(AssimpSkeletalAnimationImporter::Options &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.SourcePath = argv[++i];
    else if (CHECK_ARG("-listanim"))
        Options.ListOnly = true;
    else if (CHECK_ARG_PARAM("-createskeleton"))
    {
        Options.SkeletonName = argv[++i];
        Options.CreateSkeleton = true;
    }
    else if (CHECK_ARG_PARAM("-skeleton"))
    {
        Options.SkeletonName = argv[++i];
        Options.CreateSkeleton = false;
    }
    else if (CHECK_ARG_PARAM("-oname"))
        Options.OutputResourceName = argv[++i];
    else if (CHECK_ARG_PARAM("-odir"))
        Options.OutputResourceDirectory = argv[++i];
    else if (CHECK_ARG_PARAM("-oprefix"))
        Options.OutputResourcePrefix = argv[++i];
    else if (CHECK_ARG_PARAM("-tps"))
        Options.DefaultAnimationTicksPerSecond = StringConverter::StringToFloat(argv[++i]);
    else if (CHECK_ARG_PARAM("-forcetps"))
        Options.OverrideTicksPerSecond = StringConverter::StringToFloat(argv[++i]);
    else if (CHECK_ARG("-all"))
        Options.AllAnimations = true;
    else if (CHECK_ARG_PARAM("-animname"))
        Options.SingleAnimationName = argv[++i];
    else if (CHECK_ARG_PARAM("-clip"))
    {
        Options.ClipAnimation = true;
        if (Y_sscanf(argv[++i], "%u:%u", &Options.ClipRangeStart, &Options.ClipRangeEnd) != 2)
        {
            Log_ErrorPrint("Invalid clip range specified: must be in format 'start:end'");
            return false;
        }
    }
    else if (CHECK_ARG("-optimize"))
        Options.OptimizeAnimation = true;
    else if (CHECK_ARG("-h") || CHECK_ARG("-help"))
    {
        i = argc;
        PrintAssimpSkeletalAnimationImporterSyntax();
        return false;
    }
    else
    {
        Log_ErrorPrintf("Unknown option: %s", argv[i]);
        return false;
    }

    return true;
}

int RunAssimpSkeletalAnimationImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintAssimpSkeletalAnimationImporterSyntax();
        return 0;
    }

    AssimpSkeletalAnimationImporter::Options options;
    AssimpSkeletalAnimationImporter::SetDefaultOptions(&options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseAssimpSkeletalAnimationImporterOption(options, i, argc, argv))
            return 1;
    }

    if (options.SourcePath.IsEmpty() || options.OutputResourceName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name or output name.");
        return 1;
    }

    {
        ConsoleProgressCallbacks progressCallbacks;
        AssimpSkeletalAnimationImporter importer(&options, &progressCallbacks);

        if (!importer.Execute())
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

