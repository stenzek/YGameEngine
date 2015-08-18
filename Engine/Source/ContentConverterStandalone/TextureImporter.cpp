#include "PrecompiledHeader.h"
#include "ContentConverter.h"
#include "ContentConverter/TextureImporter.h"
Log_SetChannel(OBJImporter);

#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

static void PrintTextureImporterSyntax()
{
    Log_InfoPrint("Texture Importer options:");
    Log_InfoPrint("  -h, -help: Displays this text.");
    Log_InfoPrint("  -i <filename>: Specify source file name.");
    Log_InfoPrint("  -o <path>: Output file name.");
    Log_InfoPrint("  -append: append to the texture instead of creating a new one. usage/mip/format options will be reused");
    Log_InfoPrint("  -rebuild: rebuild the texture using the specified options");
    Log_InfoPrint("  -type <1D | 1DArray | 2D | 2DArray | Cube | CubeArray>: texture type");
    Log_InfoPrint("  -usage <diffuse, diffuseextra, decal, normalmap, normalmapextra, bumpmap, alphamap, specularmap>: Texture usage,");
    Log_InfoPrint("         used to determine format in automatic cases. Alphamap and bumpmap result in conversion to those formats.");
    Log_InfoPrint("  -filter <nearest | bilinear | trilinear | anisotropic>: set maximum allowed texture filter");
    Log_InfoPrint("  -address[uvw] <clamp|wrap>: texture address mode");
    Log_InfoPrint("  -resize <w>x<h>: resize the image");
    Log_InfoPrint("  -resizefilter <box | bilinear | bicubic | bspline | catmullrom | lanczos3>: texture filter for resize operations");
    Log_InfoPrint("  -[no]genmipmaps: Automatically generate mipmaps, default on");
    Log_InfoPrint("  -[no]sourcepremultipliedalpha: specifiy whether the source has premultiplied alpha channel");
    Log_InfoPrint("  -[no]premultipliedalpha: allow conversion to premultiplied alpha");
    Log_InfoPrint("  -oformat <format>: Explicit output pixel format.");
    Log_InfoPrint("  -[no]compress: Enable/disable texture compression, default on");
    Log_InfoPrint("  -[no]compile: Enable compilation after generation, default on");
    Log_InfoPrint("");
}

bool ParseTextureImporterOption(TextureImporterOptions &Options, int &i, int argc, char *argv[])
{
    if (CHECK_ARG_PARAM("-i"))
        Options.InputFileNames.Add(argv[++i]);
    else if (CHECK_ARG_PARAM("-o"))
        Options.OutputName = argv[++i];
    else if (CHECK_ARG("-append"))
        Options.AppendTexture = true;
    else if (CHECK_ARG("-rebuild"))
        Options.RebuildTexture = true;
    else if (CHECK_ARG_PARAM("-type"))
    {
        if (!NameTable_TranslateType(NameTables::TextureType, argv[++i], &Options.Type, true))
        {
            Log_ErrorPrintf("Invalid texture type '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-usage"))
    {
        if (!NameTable_TranslateType(NameTables::TextureUsage, argv[++i], &Options.Usage, true))
        {
            Log_ErrorPrintf("Invalid texture usage '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-filter"))
    {
        ++i;

        if (!Y_stricmp(argv[i], "nearest"))
            Options.Filter = TEXTURE_FILTER_MIN_MAG_MIP_POINT;
        else if (!Y_stricmp(argv[i], "bilinear"))
            Options.Filter = TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        else if (!Y_stricmp(argv[i], "trilinear"))
            Options.Filter = TEXTURE_FILTER_MIN_MAG_MIP_LINEAR;
        else if (!Y_stricmp(argv[i], "anisotropic"))
            Options.Filter = TEXTURE_FILTER_ANISOTROPIC;
        else
        {
            Log_ErrorPrintf("unknown texture filter: %s", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-addressu"))
    {
        if (!NameTable_TranslateType(NameTables::TextureAddressMode, argv[++i], &Options.AddressU, true))
        {
            Log_ErrorPrintf("Invalid texture address mode '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-addressv"))
    {
        if (!NameTable_TranslateType(NameTables::TextureAddressMode, argv[++i], &Options.AddressV, true))
        {
            Log_ErrorPrintf("Invalid texture address mode '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-addressw"))
    {
        if (NameTable_TranslateType(NameTables::TextureAddressMode, argv[++i], &Options.AddressW, true))
        {
            Log_ErrorPrintf("Invalid texture address mode '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG_PARAM("-resize"))
    {
        ++i;

        uint32 len = Y_strlen(argv[i]);
        char *temp = new char[len + 1];
        Y_memcpy(temp, argv[i], len + 1);

        char *dimensions[2];
        uint32 width;
        uint32 height;
        if (Y_strsplit(temp, 'x', dimensions, 2) != 2 ||
            (width = StringConverter::StringToUInt32(dimensions[0])) == 0 ||
            (height = StringConverter::StringToUInt32(dimensions[1])) == 0)
        {
            Log_ErrorPrintf("invalid dimensions: %s", argv[i]);
            delete[] temp;
            return false;
        }

        delete[] temp;
        Options.ResizeWidth = width;
        Options.ResizeHeight = height;
    }
    else if (CHECK_ARG_PARAM("-resizefilter"))
    {
        if (!NameTable_TranslateType(NameTables::ImageResizeFilter, argv[++i], &Options.ResizeFilter, true))
        {
            Log_ErrorPrintf("Invalid image resize filter '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG("-genmipmaps"))
        Options.GenerateMipMaps = true;
    else if (CHECK_ARG("-nogenmipmaps"))
        Options.GenerateMipMaps = false;
    else if (CHECK_ARG("-sourcepremultipliedalpha"))
        Options.SourcePremultipliedAlpha = true;
    else if (CHECK_ARG("-nosourcepremultipliedalpha"))
        Options.SourcePremultipliedAlpha = false;
    else if (CHECK_ARG("-premultipliedalpha"))
        Options.EnablePremultipliedAlpha = true;
    else if (CHECK_ARG("-nopremultipliedalpha"))
        Options.EnablePremultipliedAlpha = false;
    else if (CHECK_ARG("-compress"))
        Options.EnableTextureCompression = true;
    else if (CHECK_ARG("-nocompress"))
        Options.EnableTextureCompression = false;
    else if (CHECK_ARG_PARAM("-oformat"))
    {
        if (!NameTable_TranslateType(NameTables::PixelFormat, argv[++i], &Options.OutputFormat, true))
        {
            Log_ErrorPrintf("Invalid output pixel format '%s' specified.", argv[i]);
            return false;
        }
    }
    else if (CHECK_ARG("-optimize"))
        Options.Optimize = true;
    else if (CHECK_ARG("-nooptimize"))
        Options.Optimize = false;
    else if (CHECK_ARG("-compile"))
        Options.Compile = true;
    else if (CHECK_ARG("-nocompile"))
        Options.Compile = false;
    else if (CHECK_ARG("-h") || CHECK_ARG("-help"))
    {
        i = argc;
        PrintTextureImporterSyntax();
        return false;
    }
    else
    {
        Log_ErrorPrintf("Unknown option: %s", argv[i]);
        return false;
    }

    return true;
}

int RunTextureImporter(int argc, char *argv[])
{
    int i;

    if (argc == 0)
    {
        PrintTextureImporterSyntax();
        return 0;
    }

    TextureImporterOptions Options;
    TextureImporter::SetDefaultOptions(&Options);

    for (i = 0; i < argc; i++)
    {
        if (!ParseTextureImporterOption(Options, i, argc, argv))
            return 1;
    }

    if (Options.InputFileNames.GetSize() == 0 || Options.OutputName.IsEmpty())
    {
        Log_ErrorPrintf("Missing input file name or output package/name.");
        return 1;
    }

    {
        ConsoleProgressCallbacks progressCallbacks;
        TextureImporter Importer(&progressCallbacks);

        if (!Importer.Execute(&Options))
        {
            Log_ErrorPrintf("Import process failed.");
            return 2;
        }
    }
    
    Log_InfoPrintf("Import process successful.");
    return 0;
}

