#pragma once
#include "ContentConverter/BaseImporter.h"
#include "Engine/Common.h"
#include "Core/Image.h"

class ByteStream;
class TextureGenerator;

struct TextureImporterOptions
{
    Array<String> InputFileNames;
    Array<String> InputMaskFileNames;
    String OutputName;
    bool AppendTexture;
    bool RebuildTexture;
    TEXTURE_TYPE Type;
    TEXTURE_USAGE Usage;
    TEXTURE_FILTER Filter;
    TEXTURE_ADDRESS_MODE AddressU;
    TEXTURE_ADDRESS_MODE AddressV;
    TEXTURE_ADDRESS_MODE AddressW;
    uint32 ResizeWidth;
    uint32 ResizeHeight;
    IMAGE_RESIZE_FILTER ResizeFilter;
    bool GenerateMipMaps;
    bool SourcePremultipliedAlpha;
    bool EnablePremultipliedAlpha;
    bool EnableTextureCompression;
    PIXEL_FORMAT OutputFormat;
    bool Optimize;
    bool Compile;
};

class TextureImporter : public BaseImporter
{
public:
    TextureImporter(ProgressCallbacks *pProgressCallbacks);
    ~TextureImporter();

    static void SetDefaultOptions(TextureImporterOptions *pOptions);

    static bool HasTextureFileExtension(const char *fileName);

    bool Execute(const TextureImporterOptions *pOptions);

private:
    const TextureImporterOptions *m_pOptions;

    PODArray<Image *> m_sourceImages;
    TextureGenerator *m_pGenerator;

    //
    bool LoadSources();
    bool LoadGenerator();

    //
    bool CreateGenerator();
    bool AppendImages();
    bool ResizeAndSetProperties();
    
    //
    bool WriteOutput();
    bool WriteCompiledOutput();
};

