#pragma once
#include "ContentConverter/BaseImporter.h"
#include "Engine/DataFormats.h"
#include "Engine/Font.h"

#define USE_FREETYPE2 1

struct FontImporterOptions
{
    String InputFileName;
    uint32 SubFontIndex;
    String OutputName;
    bool AppendToFile;
    FONT_TYPE Type;
    uint32 RenderWidth;
    uint32 RenderHeight;
    PODArray<uint32> UnicodeCodePointSet;
};

class BinaryBlob;
class FontGenerator;

class FontImporter : public BaseImporter
{
public:
    FontImporter(ProgressCallbacks *pProgressCallbacks);
    virtual ~FontImporter();

    // assuming 96 dpi, convert points to pixels
    static uint32 ConvertPointsToPixels(float pointSize);

    // parse a font size with suffix, e.g. 12pt or 16px
    static bool ParseFontSizeString(uint32 *pOutPixels, const char *value);

    // list all the sub fonts in a font
    static void ListSubFonts(const char *fileName, ProgressCallbacks *pProgressCallbacks);

    // list available sizes in a bitmap font
    static void ListFontSizes(const char *fileName, uint32 subFontIndex, ProgressCallbacks *pProgressCallbacks);

    static void SetDefaultOptions(FontImporterOptions *pOptions);
    static bool AddCharacterSet(FontImporterOptions *pOptions, const char *blockName);
    static bool IsWhitespace(uint32 codePoint);

    bool Execute(const FontImporterOptions *pOptions);

private:
    bool ReadInputFile(const char *fileName, uint32 subFontIndex, uint32 characterWidth, uint32 characterHeight);
    bool CreateNewGenerator(FONT_TYPE type);
    bool OpenExistingGenerator(const char *fontName);
    bool AddCharacters(const uint32 *pCharacters, uint32 nCharacters);
    bool AddBitmapCharacter(uint32 codePoint);
    bool AddSignedDistanceFieldCharacter(uint32 codePoint);
    bool SaveGenerator(const char *fontName);

    FontGenerator *m_pGenerator;
    uint32 m_renderWidth;
    uint32 m_renderHeight;

#ifdef USE_FREETYPE2
    void *m_pFreeTypeLibrary;
    void *m_pFreeTypeFace;
    bool m_fontIsBitmap;
#else
    BinaryBlob *m_pInputFile;
#endif
};

