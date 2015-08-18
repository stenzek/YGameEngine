#include "PrecompiledHeader.h"
#include "ContentConverter.h"
#include "ContentConverter/FontImporter.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintFontImporterSyntax()
{
    Log_InfoPrint("Font Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -o <name>: Output name.");
    Log_InfoPrint("  -append: Don't create a new font, append to existing");
    Log_InfoPrint("  -width <width>: Raster pixel width (ignored for non-bitmap fonts), default is unspecified (uses width).");
    Log_InfoPrint("  -height <height>: Raster pixel height, default 16");
    Log_InfoPrint("  -bitmap: Generate a bitmap font");
    Log_InfoPrint("  -sdf: Generate a signed distance field font");
    Log_InfoPrint("  -charset <name>: Add this character set");
    Log_InfoPrint("");
}

bool ParseFontImporterOption(FontImporterOptions &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.InputFileName = argv[++i];
    else if (CHECK_ARG_PARAM("-subfont"))
        Options.SubFontIndex = StringConverter::StringToUInt32(argv[++i]);
    else if (CHECK_ARG_PARAM("-o"))
        Options.OutputName = argv[++i];
    else if (CHECK_ARG_PARAM("-width"))
    {
        if (!FontImporter::ParseFontSizeString(&Options.RenderWidth, argv[++i]))
        {
            Log_ErrorPrintf("Failed to parse font width string: '%s' (must be a digit followed by pt or px)", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-height"))
    {
        if (!FontImporter::ParseFontSizeString(&Options.RenderHeight, argv[++i]))
        {
            Log_ErrorPrintf("Failed to parse height size string: '%s' (must be a digit followed by pt or px)", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG("-listsubfonts"))
    {
        if (Options.InputFileName.GetLength() == 0)
        {
            Log_ErrorPrintf("Missing font file name");
            return false;
        }

        ConsoleProgressCallbacks progressCallbacks;
        FontImporter::ListSubFonts(Options.InputFileName, &progressCallbacks);
        return false;
    }
    else if (CHECK_ARG("-listfontsizes"))
    {
        if (Options.InputFileName.GetLength() == 0)
        {
            Log_ErrorPrintf("Missing font file name");
            return false;
        }

        ConsoleProgressCallbacks progressCallbacks;
        FontImporter::ListFontSizes(Options.InputFileName, Options.SubFontIndex, &progressCallbacks);
        return false;
    }
    else if (CHECK_ARG("-append"))
        Options.AppendToFile = true;
    else if (CHECK_ARG("-bitmap"))
        Options.Type = FONT_TYPE_BITMAP;
    else if (CHECK_ARG("-sdf"))
        Options.Type = FONT_TYPE_SIGNED_DISTANCE_FIELD;
    else if (CHECK_ARG_PARAM("-charset"))
    {
        if (!FontImporter::AddCharacterSet(&Options, argv[++i]))
        {
            Log_ErrorPrintf("Failed to add character set '%s'", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-codepoint"))
    {
        uint32 val = StringConverter::StringToUInt32(argv[++i]);
        if (val == 0)
        {
            Log_ErrorPrintf("Invalid code point: %s", argv[i]);
            return false;
        }

        Options.UnicodeCodePointSet.Add(val);
    }
    else if (CHECK_ARG("-h") || CHECK_ARG("-help"))
    {
        i = argc;
        PrintFontImporterSyntax();
        return false;
    }

    return true;
}

int RunFontImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintFontImporterSyntax();
        return 0;
    }

    FontImporterOptions Options;
    FontImporter::SetDefaultOptions(&Options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseFontImporterOption(Options, i, argc, argv))
            return 1;
    }

    if (Options.InputFileName.IsEmpty() || Options.OutputName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name or output package/name.");
        return 1;
    }

    if (Options.UnicodeCodePointSet.GetSize() == 0)
    {
        Log_WarningPrintf("No code point specified, assuming basic latin1");
        FontImporter::AddCharacterSet(&Options, "latin1");
    }
    
    {
        ConsoleProgressCallbacks progressCallbacks;
        FontImporter Importer(&progressCallbacks);

        if (!Importer.Execute(&Options))
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

