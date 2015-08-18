#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/FontImporter.h"
#include "ResourceCompiler/FontGenerator.h"

#if USE_FREETYPE2

#include "freetype2/ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#ifdef Y_COMPILER_MSVC
    #ifdef Y_BUILD_CONFIG_DEBUG
        #pragma comment(lib, "freetype26d")
    #else
        #pragma comment(lib, "freetype26")
    #endif
#endif

#else

#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#define STB_TRUETYPE_IMPLEMENTATION 1
#include "ContentConverter/stb_truetype.h"

#endif


FontImporter::FontImporter(ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pGenerator(nullptr),
      m_renderWidth(0),
      m_renderHeight(0)
#ifdef USE_FREETYPE2
      ,
      m_pFreeTypeLibrary(nullptr),
      m_pFreeTypeFace(nullptr),
      m_fontIsBitmap(false)
#else
      ,
      m_pInputFile(nullptr)
#endif
{

}

FontImporter::~FontImporter()
{
    delete m_pGenerator;

#ifdef USE_FREETYPE2
    if (m_pFreeTypeFace != nullptr)
        FT_Done_Face(reinterpret_cast<FT_Face>(m_pFreeTypeFace));
    if (m_pFreeTypeLibrary != nullptr)
        FT_Done_FreeType(reinterpret_cast<FT_Library>(m_pFreeTypeLibrary));
#else
    if (m_pInputFile != nullptr)
        m_pInputFile->Release();
#endif
}

uint32 FontImporter::ConvertPointsToPixels(float pointSize)
{
    const float POINTS_PER_INCH = 72;
    const float DOTS_PER_INCH = 96;

    return (uint32)Math::Truncate(Math::Round(pointSize * (DOTS_PER_INCH / POINTS_PER_INCH)));
}

bool FontImporter::ParseFontSizeString(uint32 *pOutPixels, const char *value)
{
    uint32 valueLen = Y_strlen(value);
    if (valueLen == 0)
        return false;

    char *temp = (char *)alloca(valueLen + 1);
    uint32 nDigits = 0;
    for (uint32 i = 0; i < valueLen; i++)
    {
        if (value[i] >= '0' && value[i] <= '9')
            temp[nDigits++] = value[i];
    }

    if (nDigits == 0)
        return false;

    temp[nDigits] = 0;
    float valueFloat = StringConverter::StringToFloat(temp);

    const char *suffix = value + nDigits;
    if (Y_stricmp(suffix, "pt") == 0)
    {
        *pOutPixels = ConvertPointsToPixels(valueFloat);
        return true;
    }
    else if (Y_stricmp(suffix, "px") == 0)
    {
        *pOutPixels = (uint32)Math::Truncate(Math::Round(valueFloat));
        return true;
    }
    else
    {
        return false;
    }
}


void FontImporter::ListSubFonts(const char *fileName, ProgressCallbacks *pProgressCallbacks)
{
    FT_Library ftLibrary;
    FT_Error ftError = FT_Init_FreeType(&ftLibrary);
    if (ftError != 0)
    {
        pProgressCallbacks->DisplayFormattedError("Failed to initialize FreeType: %d", ftError);
        return;
    }

    // load the font
    FT_Face ftFace;
    ftError = FT_New_Face(ftLibrary, fileName, 0, &ftFace);
    if (ftError != 0)
    {
        pProgressCallbacks->DisplayFormattedError("FT_New_Face failed: %d", ftError);
        FT_Done_FreeType(ftLibrary);
        return;
    }

    pProgressCallbacks->DisplayFormattedInformation("Font '%s' has %u sub fonts:", fileName, (uint32)ftFace->num_faces);

    // iterate through font faces
    for (FT_Long subFontIndex = 0; subFontIndex < ftFace->num_faces; subFontIndex++)
    {
        FT_Face subFace;
        ftError = FT_New_Face(ftLibrary, fileName, subFontIndex, &subFace);
        if (ftError != 0)
        {
            pProgressCallbacks->DisplayFormattedError("FT_New_Face for subfont %u failed: %d", (uint32)subFontIndex, ftError);
            break;
        }

        SmallString summaryString;
        uint32 summaryStringCount = 0;
        if (subFace->family_name != nullptr)
            summaryString.AppendFormattedString("%sfamily: %s", ((summaryStringCount++) > 0) ? ", " : "", subFace->family_name);
        if (subFace->style_name != nullptr)
            summaryString.AppendFormattedString("%sstyle: %s", ((summaryStringCount++) > 0) ? ", " : "", subFace->style_name);
        if (subFace->style_flags & FT_STYLE_FLAG_BOLD)
            summaryString.AppendFormattedString("%sbold", ((summaryStringCount++) > 0) ? ", " : "");
        if (subFace->style_flags & FT_STYLE_FLAG_ITALIC)
            summaryString.AppendFormattedString("%sitalic", ((summaryStringCount++) > 0) ? ", " : "");

        if (!(ftFace->face_flags & FT_FACE_FLAG_SCALABLE) && (ftFace->available_sizes > 0))
        {
            pProgressCallbacks->DisplayFormattedInformation("  Sub font %u (%s) is a bitmap font with %u sizes:", (uint32)subFontIndex, summaryString.GetCharArray(), (uint32)subFace->num_fixed_sizes);

            for (int i = 0; i < subFace->num_fixed_sizes; i++)
                pProgressCallbacks->DisplayFormattedInformation("    %u x %u pixels", (uint32)subFace->available_sizes[i].width, (uint32)subFace->available_sizes[i].height);
        }
        else
        {
            pProgressCallbacks->DisplayFormattedInformation("  Sub font %u (%s) is a outline font.", (uint32)subFontIndex, summaryString.GetCharArray(), (uint32)subFace->num_fixed_sizes);
        }

        FT_Done_Face(subFace);
    }

    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);
}

void FontImporter::ListFontSizes(const char *fileName, uint32 subFontIndex, ProgressCallbacks *pProgressCallbacks)
{
    FT_Library ftLibrary;
    FT_Error ftError = FT_Init_FreeType(&ftLibrary);
    if (ftError != 0)
    {
        pProgressCallbacks->DisplayFormattedError("Failed to initialize FreeType: %d", ftError);
        return;
    }

    // load the font
    FT_Face ftFace;
    ftError = FT_New_Face(ftLibrary, fileName, subFontIndex, &ftFace);
    if (ftError != 0)
    {
        pProgressCallbacks->DisplayFormattedError("FT_New_Face failed: %d", ftError);
        FT_Done_FreeType(ftLibrary);
        return;
    }

    if (ftFace->num_fixed_sizes == 0)
    {
        pProgressCallbacks->DisplayFormattedError("Font '%s' is not a bitmap font.", fileName);
        FT_Done_Face(ftFace);
        FT_Done_FreeType(ftLibrary);
        return;
    }

    pProgressCallbacks->DisplayFormattedInformation("Bitmap font with %d sizes detected.", ftFace->num_fixed_sizes);
    for (int i = 0; i < ftFace->num_fixed_sizes; i++)
        pProgressCallbacks->DisplayFormattedInformation("  Bitmap size: %u x %u pixels", (uint32)ftFace->available_sizes[i].width, (uint32)ftFace->available_sizes[i].height);

    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);
}


void FontImporter::SetDefaultOptions(FontImporterOptions *pOptions)
{
    pOptions->SubFontIndex = 0;
    pOptions->Type = FONT_TYPE_BITMAP;
    pOptions->AppendToFile = false;
    pOptions->RenderWidth = 0;
    pOptions->RenderHeight = 0;
}

bool FontImporter::AddCharacterSet(FontImporterOptions *pOptions, const char *blockName)
{
    // latin1 - 32->126, 160->255
    for (uint32 i = 32; i <= 126; i++)
        pOptions->UnicodeCodePointSet.Add(i);
    for (uint32 i = 160; i <= 255; i++)
        pOptions->UnicodeCodePointSet.Add(i);

    return true;
}

bool FontImporter::IsWhitespace(uint32 codePoint)
{
    return (codePoint == '\r' || codePoint == '\n' ||
            codePoint == '\t' || codePoint == ' ');
}

bool FontImporter::Execute(const FontImporterOptions *pOptions)
{
    m_pProgressCallbacks->DisplayFormattedInformation("Requested render width: %u", pOptions->RenderWidth);
    m_pProgressCallbacks->DisplayFormattedInformation("Requested render height: %u", pOptions->RenderHeight);

    // open input file
    if (!ReadInputFile(pOptions->InputFileName, pOptions->SubFontIndex, pOptions->RenderWidth, pOptions->RenderHeight))
    {
        m_pProgressCallbacks->DisplayFormattedModalError("ReadInputFile() failed, the log may contain more information.");
        return false;
    }

    m_pProgressCallbacks->DisplayFormattedInformation("Actual render width: %u", m_renderWidth);
    m_pProgressCallbacks->DisplayFormattedInformation("Actual render height: %u", m_renderHeight);

    // load/open
    if (pOptions->AppendToFile)
    {
        if (!OpenExistingGenerator(pOptions->OutputName))
        {
            m_pProgressCallbacks->DisplayFormattedModalError("OpenExistingGenerator() failed, the log may contain more information.");
            return false;
        }
    }
    else
    {
        if (!CreateNewGenerator(pOptions->Type))
        {
            m_pProgressCallbacks->DisplayFormattedModalError("CreateNewGenerator() failed, the log may contain more information.");
            return false;
        }
    }

    // bake
    if (!AddCharacters(pOptions->UnicodeCodePointSet.GetBasePointer(), pOptions->UnicodeCodePointSet.GetSize()))
    {
        m_pProgressCallbacks->DisplayFormattedModalError("AddCharacters() failed, the log may contain more information.");
        return false;
    }

    // save
    if (!SaveGenerator(pOptions->OutputName))
    {
        m_pProgressCallbacks->DisplayFormattedModalError("SaveGenerator() failed, the log may contain more information.");
        return false;
    }

    return true;
}

bool FontImporter::ReadInputFile(const char *fileName, uint32 subFontIndex, uint32 characterWidth, uint32 characterHeight)
{
#ifdef USE_FREETYPE2
    FT_Library ftLibrary;
    FT_Error ftError = FT_Init_FreeType(&ftLibrary);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to initialize FreeType: %d", ftError);
        return false;
    }

    m_pFreeTypeLibrary = reinterpret_cast<void *>(ftLibrary);

    // load the font
    FT_Face ftFace;
    ftError = FT_New_Face(ftLibrary, fileName, subFontIndex, &ftFace);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to initialize face: %d", ftError);
        return false;
    }

    m_pFreeTypeFace = reinterpret_cast<void *>(ftFace);

    // for unspecified widths, set to the height
    if (characterHeight == 0)
        characterHeight = 16;
    if (characterWidth == 0)
        characterWidth = characterHeight;

    // is it a bitmap font?
    m_fontIsBitmap = !(ftFace->face_flags & FT_FACE_FLAG_SCALABLE) && (ftFace->available_sizes > 0);
    if (m_fontIsBitmap)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("Bitmap font with %d sizes detected.", ftFace->num_fixed_sizes);

        uint32 closestDiff = (uint32)Math::Abs(ftFace->available_sizes[0].width - (int)characterWidth) + (uint32)Math::Abs(ftFace->available_sizes[0].height - (int)characterHeight);
        m_renderWidth = ftFace->available_sizes[0].width;
        m_renderHeight = ftFace->available_sizes[0].height;
        for (int i = 1; i < ftFace->num_fixed_sizes; i++)
        {
            uint32 thisDiff = (uint32)Math::Abs(ftFace->available_sizes[i].width - (int)characterWidth) + (uint32)Math::Abs(ftFace->available_sizes[i].height - (int)characterHeight);
            if (thisDiff < closestDiff)
            {
                m_renderWidth = ftFace->available_sizes[1].width;
                m_renderHeight = ftFace->available_sizes[1].height;
                closestDiff = thisDiff;
            }
        }
    }
    else
    {
        // use the specified size
        m_renderWidth = characterWidth;
        m_renderHeight = characterHeight;
    }

    // freetype mesures font size in 1/64th of pixels, or 26.6 fixed point
    //ftError = FT_Set_Char_Size(reinterpret_cast<FT_Face>(m_pFreeTypeFace), m_renderWidth * 64, m_renderHeight * 64, 96, 96);
    ftError = FT_Set_Pixel_Sizes(reinterpret_cast<FT_Face>(m_pFreeTypeFace), m_renderWidth, m_renderHeight);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("FT_Set_Pixel_Sizes failed: %d", ftError);
        return false;
    }

    return true;
#else
    AutoReleasePtr<ByteStream> pStream = FileSystem::OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        m_pProgressCallbacks->DisplayFormattedError("Unable to open file '%s'", fileName);
        return false;
    }

    if ((m_pInputFile = BinaryBlob::CreateFromStream(pStream)) == nullptr)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to read file '%s'", fileName);
        return false;
    }

    return true;
#endif
}

bool FontImporter::CreateNewGenerator(FONT_TYPE type)
{
    m_pGenerator = new FontGenerator();
    m_pGenerator->Create(type, m_renderHeight);

    // mipmaps are off by default, texture filtering for non-bitmap sources only
    m_pGenerator->SetUseTextureFiltering(!m_fontIsBitmap);
    m_pGenerator->SetGenerateMipmaps(false);

    return true;
}

bool FontImporter::OpenExistingGenerator(const char *fontName)
{
    PathString fileName;
    fileName.Format("%s.font.zip", fontName);

    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
    if (pStream == nullptr)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to open existing font '%s'", fileName.GetCharArray());
        return false;
    }

    m_pGenerator = new FontGenerator();
    if (!m_pGenerator->Load(fileName, pStream))
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to load existing font '%s'", fileName.GetCharArray());
        return false;
    }

    // check the height
    if (m_pGenerator->GetCharacterHeight() != m_renderHeight)
    {
        m_pProgressCallbacks->DisplayFormattedError("Loaded font has a different height: %u vs %u", m_pGenerator->GetCharacterHeight(), m_renderHeight);
        return false;
    }

    return true;
}

bool FontImporter::SaveGenerator(const char *fontName)
{
    PathString fileName;
    fileName.Format("%s.font.zip", fontName);

    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == nullptr)
        return false;

    if (!m_pGenerator->Save(pStream))
    {
        pStream->Discard();
        return false;
    }

    return pStream->Commit();
}

bool FontImporter::AddCharacters(const uint32 *pCharacters, uint32 nCharacters)
{
    m_pProgressCallbacks->DisplayFormattedInformation("Rendering %u characters...", nCharacters);

    for (uint32 characterIndex = 0; characterIndex < nCharacters; characterIndex++)
    {
        uint32 codePoint = pCharacters[characterIndex];

        if (m_pGenerator->GetCharacterByCodePoint(codePoint) != nullptr)
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Code point %u already exists, skipping.", codePoint);
            continue;
        }

        if (m_pGenerator->GetType() == FONT_TYPE_BITMAP)
        {
            if (!AddBitmapCharacter(codePoint))
            {
                m_pProgressCallbacks->DisplayFormattedWarning("Failed to render code point %u.", codePoint);
                continue;
            }
        }
        else if (m_pGenerator->GetType() == FONT_TYPE_SIGNED_DISTANCE_FIELD)
        {
            if (!AddSignedDistanceFieldCharacter(codePoint))
            {
                m_pProgressCallbacks->DisplayFormattedWarning("Failed to render code point %u.", codePoint);
                continue;
            }
        }
    }

    if (m_pGenerator->GetCharacterCount() == 0)
    {
        m_pProgressCallbacks->DisplayError("No characters were rendered.");
        return false;
    }

    return true;
}

bool FontImporter::AddBitmapCharacter(uint32 codePoint)
{
#if USE_FREETYPE2
    FT_Error ftError;

    FT_Face ftFace = reinterpret_cast<FT_Face>(m_pFreeTypeFace);
    FT_UInt ftCharIndex = FT_Get_Char_Index(ftFace, codePoint);
    if (ftCharIndex == 0)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("Code point %u does not exist in font", codePoint);
        return false;
    }

    // use mono?
    bool useMonoRenderMode = m_fontIsBitmap;

    // load glyph
    uint32 loadFlags = FT_LOAD_DEFAULT;
    if (useMonoRenderMode)
        loadFlags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;

    // load glyph
    ftError = FT_Load_Glyph(ftFace, ftCharIndex, loadFlags);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("Failed to load glyph: %d", ftError);
        return false;
    }

    // create glyph object
    FT_Glyph ftGlyph;
    ftError = FT_Get_Glyph(ftFace->glyph, &ftGlyph);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to load get glyph: %d", ftError);
        return false;
    }

    // convert to bitmap
    FT_BitmapGlyph ftBitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(ftGlyph);
    ftError = FT_Glyph_To_Bitmap(reinterpret_cast<FT_Glyph *>(&ftBitmapGlyph), (useMonoRenderMode) ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL, nullptr, 0);
    if (ftError != 0)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to convert glyph to bitmap: %d", ftError);
        FT_Done_Glyph(ftGlyph);
        return false;
    }
    
    // determine image dimensions
    uint32 imageWidth = ftBitmapGlyph->bitmap.width;
    uint32 imageHeight = ftBitmapGlyph->bitmap.rows;
    int32 baseLine = ftFace->size->metrics.height / 64;
    int32 lsBearing = ftFace->glyph->metrics.horiBearingX / 64;
    int32 tsBearing = ftFace->glyph->metrics.horiBearingY / 64;
    float advance = (float)ftFace->glyph->metrics.horiAdvance / 64.0f;

    // calculate draw offsets
    int32 drawOffsetX = lsBearing;
    int32 drawOffsetY = baseLine - tsBearing;
    m_pProgressCallbacks->DisplayFormattedDebugMessage("cp %u: %ux%u bitmap, bl %d, lsb %d, tsb %d, advance %.4f, ox %d, oy %d", codePoint, imageWidth, imageHeight, baseLine, lsBearing, tsBearing, advance, drawOffsetX, drawOffsetY);

    // check for whitespace
    if (IsWhitespace(codePoint) || imageWidth == 0 || imageHeight == 0)
    {
        // create a whitespace character
        return m_pGenerator->AddWhitespaceCharacter(codePoint, advance);
    }
    
    // create the image
    Image glyphBitmapImage;
    glyphBitmapImage.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, imageWidth, imageHeight, 1);
    Y_memzero(glyphBitmapImage.GetData(), glyphBitmapImage.GetDataSize());

    // set pixels
    for (uint32 imageY = 0; imageY < imageHeight; imageY++)
    {
        uint32 *pRow = reinterpret_cast<uint32 *>(glyphBitmapImage.GetData() + (imageY * glyphBitmapImage.GetDataRowPitch()));
        for (uint32 imageX = 0; imageX < imageWidth; imageX++)
        {
            // don't premultiply the colour, the texture generation later on will take care of that
            byte alpha;
            if (useMonoRenderMode)
            {
                // find the texel
                byte texel = ftBitmapGlyph->bitmap.buffer[imageY * ftBitmapGlyph->bitmap.pitch + (imageX / 8)];

                // find the bit
                uint32 bit = imageX % 8;
                alpha = (((texel >> (7 - bit)) & 0x1) != 0) ? 255 : 0;
            }
            else
            {
                // access 8-bit value directly
                alpha = ftBitmapGlyph->bitmap.buffer[imageY * ftBitmapGlyph->bitmap.pitch + imageX];
            }

            pRow[imageX] = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, alpha);
        }
    }

    // cleanup freetype stuff
    FT_Done_Glyph(reinterpret_cast<FT_Glyph>(ftBitmapGlyph));
    if (reinterpret_cast<FT_Glyph>(ftBitmapGlyph) != ftGlyph)
        FT_Done_Glyph(ftGlyph);

    // create character
    return m_pGenerator->AddCharacter(codePoint, 0, 0, drawOffsetX, drawOffsetY, advance, &glyphBitmapImage);

#else
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, m_pInputFile->GetDataPointer(), 0))
    {
        m_pProgressCallbacks->DisplayFormattedDebugMessage("Font initialization failed");
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, (float)m_pGenerator->GetCharacterHeight());
    int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, codePoint);
    if (glyphIndex == 0)
    {
        m_pProgressCallbacks->DisplayFormattedDebugMessage("Glyph not found for code point %u", codePoint);
        return false;
    }

    int advance;
    int lsb;
    int x0, y0;
    int x1, y1;
    stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advance, &lsb);
    stbtt_GetGlyphBitmapBox(&fontInfo, glyphIndex, scale, scale, &x0, &y0, &x1, &y1);

    // check for whitespace
    if (IsWhitespace(codePoint) || x0 == x1 || y0 == y1)
    {
        // create a whitespace character
        return m_pGenerator->AddWhitespaceCharacter(codePoint, (float)advance * scale);
    }

    // allocate pixel buffer since this only writes alpha
    uint32 imageWidth = Math::Abs(x1 - x0) + 1;
    uint32 imageHeight = Math::Abs(y1 - y0) + 1;
    byte *pAlphaPixels = new byte[imageWidth * imageHeight];
    Y_memzero(pAlphaPixels, imageWidth * imageHeight);
    stbtt_MakeGlyphBitmap(&fontInfo, pAlphaPixels, imageWidth, imageHeight, imageWidth, scale, scale, glyphIndex);

    // allocate the real image
    Image image;
    image.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, imageWidth, imageHeight, 1);
    Y_memzero(image.GetData(), image.GetDataSize());

    // set pixels
    for (uint32 imageY = 0; imageY < imageHeight; imageY++)
    {
        uint32 *pRow = reinterpret_cast<uint32 *>(image.GetData() + (imageY * image.GetDataRowPitch()));
        for (uint32 imageX = 0; imageX < imageWidth; imageX++)
        {
            // don't premultiply the colour, the texture generation later on will take care of that
            byte alpha = pAlphaPixels[imageY * imageWidth + imageX];
            pRow[imageX] = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, alpha);
        }
    }

    // this buffer can go now
    delete[] pAlphaPixels;

    // create character
    return m_pGenerator->AddCharacter(codePoint, 0, 0, (uint32)x0, (uint32)y0, (float)advance * scale, &image);
#endif
}

bool FontImporter::AddSignedDistanceFieldCharacter(uint32 codePoint)
{
    return false;
}

#if 0

bool FontImporter::Execute(const FontImporterOptions *pOptions)
{
    uint32 i;
    uint32 x, y;
    String fileName;
    DDSWriter ddsWriter;

    if (pOptions->InputFileName.IsEmpty() ||
        pOptions->OutputName.IsEmpty() ||
        pOptions->UnicodeCodePointSet.GetSize() == 0)
    {
        m_pProgressCallbacks->ModalError("One or more required parameters not set.");
        return false;
    }

    ByteStream *pStream;
    if ((pStream = FileSystem::OpenFile(pOptions->InputFileName, BYTESTREAM_OPEN_READ)) == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not open input file '%s'.", pOptions->InputFileName.GetCharArray());
        return false;
    }

    uint32 cbFontData = (uint32)pStream->GetSize();
    byte *pFontBytes = new byte[cbFontData];
    if (!pStream->Read2(pFontBytes, cbFontData))
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not read font data.");
        delete[] pFontBytes;
        pStream->Release();
        return false;
    }

    // close stream
    pStream->Release();

    uint32 CurTextureSize = 128;
    uint32 FirstCharacter = 0;
    uint32 LastCharacter = 255;
    uint32 CurrentCharacter = FirstCharacter;
    uint32 CurrentTextureIndex = 0;

    m_arrCharacters.Resize(LastCharacter - CurrentCharacter + 1);
    Y_memzero(m_arrCharacters.GetBasePointer(), sizeof(DF_FONT_CHARACTER) * m_arrCharacters.GetSize());
    for (i = 0; i < m_arrCharacters.GetSize(); i++)
    {
        DF_FONT_CHARACTER *pch = &m_arrCharacters[i];
        pch->x0 = pch->x1 = 0;
        pch->y0 = pch->y1 = 0;
        pch->xOffset = pch->yOffset = pch->xAdvance = 0.0f;
        pch->TextureIndex = -1;
        pch->IsWhiteSpace = false;
    }

    while (CurrentCharacter <= LastCharacter)
    {
        byte *pPixels = new byte[CurTextureSize * CurTextureSize];
        uint32 nCharacters = LastCharacter - CurrentCharacter + 1;
        stbtt_bakedchar *pBakedChars = new stbtt_bakedchar[nCharacters];

        m_pProgressCallbacks->DisplayFormattedInformation("Baking %u characters to %u * %u texture", nCharacters, CurTextureSize, CurTextureSize);
        
        int r = stbtt_BakeFontBitmap(pFontBytes, 0, (float)pOptions->Height, pPixels, CurTextureSize, CurTextureSize, CurrentCharacter, nCharacters, pBakedChars);
        if (r < 0)
        {
            if ((CurTextureSize * 2) > pOptions->MaxTextureSize)
            {
                m_pProgressCallbacks->DisplayFormattedInformation("Only wrote %d characters, moving to new texture", -r);
                CurTextureSize = pOptions->MaxTextureSize;
                nCharacters = (uint32)(-r);
                CurrentTextureIndex++;
            }
            else
            {
                m_pProgressCallbacks->DisplayFormattedInformation("Only wrote %d characters, expanding texture", -r);
                CurTextureSize *= 2;
                delete[] pBakedChars;
                delete[] pPixels;
                continue;
            }
        }

        // allocate rgba pixels and convert them
        FontTexture t;
        t.pPixels = new byte[CurTextureSize * CurTextureSize * 4];
        t.cbPixels = CurTextureSize * CurTextureSize * 4;
        t.pf = PIXEL_FORMAT_R8G8B8A8_UNORM;

        // apply alpha value, and premultiply the rgb values
        byte *pAPixel = pPixels;
        byte *pRGBAPixel = t.pPixels;
        for (y = 0; y < CurTextureSize; y++)
        {
            for (x = 0; x < CurTextureSize; x++)
            {
                *(pRGBAPixel++) = *(pAPixel);
                *(pRGBAPixel++) = *(pAPixel);
                *(pRGBAPixel++) = *(pAPixel);
                *(pRGBAPixel++) = *(pAPixel);
                pAPixel++;
            }
        }

        // compress textures?
        if (pOptions->CompressTextures)
        {
            static const PIXEL_FORMAT compressedPixelFormat = PIXEL_FORMAT_BC3_UNORM;
            uint32 cbCompressedPixels = PixelFormat_CalculateImageSize(compressedPixelFormat, CurTextureSize, CurTextureSize, 1);
            byte *pCompressedPixels = new byte[cbCompressedPixels];
            if (PixelFormat_ConvertPixels(CurTextureSize, CurTextureSize, t.pPixels, PixelFormat_CalculateRowPitch(t.pf, CurTextureSize), t.pf,
                                          pCompressedPixels, PixelFormat_CalculateRowPitch(compressedPixelFormat, CurTextureSize), compressedPixelFormat, &cbCompressedPixels))
            {
                m_pProgressCallbacks->DisplayFormattedInformation("Compressed font texture from %u bytes to %u bytes.", t.cbPixels, cbCompressedPixels);
                delete[] t.pPixels;
                t.pPixels = pCompressedPixels;
                t.cbPixels = cbCompressedPixels;
                t.pf = compressedPixelFormat;
            }
            else
            {
                m_pProgressCallbacks->DisplayFormattedError("Texture compression failed.");
                delete[] pCompressedPixels;
            }
        }

        m_arrTextures.Add(t);
#if 0
        FontTexture t;
        t.pPixels = pPixels;
        t.cbPixels = sizeof(byte) * CurTextureSize * CurTextureSize;
        t.pf = PIXEL_FORMAT_A8_UNORM;
        m_arrTextures.Add(t);
#endif

        for (i = 0; i < nCharacters; i++)
        {
            uint32 realCh = CurrentCharacter + i;
            DebugAssert(realCh < m_arrCharacters.GetSize());

            DF_FONT_CHARACTER *pch = &m_arrCharacters[realCh];
            const stbtt_bakedchar *pbch = &pBakedChars[i];
            pch->x0 = pbch->x0;
            pch->x1 = pbch->x1;
            pch->y0 = pbch->y0;
            pch->y1 = pbch->y1;
            pch->xOffset = pbch->xoff;
            pch->yOffset = pbch->yoff;
            pch->xAdvance = pbch->xadvance;
            pch->TextureIndex = CurrentTextureIndex;
            pch->IsWhiteSpace = (
                realCh == '\r' || realCh == '\n' ||
                realCh == '\t' || realCh == ' '
            );
        }

        CurrentCharacter += nCharacters;
        delete[] pBakedChars;
    }

    delete[] pFontBytes;
    
    // write output textures
    for (i = 0; i < m_arrTextures.GetSize(); i++)
    {
        fileName.Format("%s_%u.dds", pOptions->OutputName.GetCharArray(), i);
        FileSystem::BuildOSPath(fileName);
        if ((pStream = FileSystem::OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE)) == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Could not open output file '%s'", fileName.GetCharArray());
            return false;
        }

        ddsWriter.Initialize(pStream);
        if (!ddsWriter.WriteHeader(DDS_TEXTURE_TYPE_2D, m_arrTextures[i].pf, CurTextureSize, CurTextureSize, 1, 1) ||
            !ddsWriter.WriteMipLevel(0, m_arrTextures[i].pPixels, m_arrTextures[i].cbPixels) ||
            !ddsWriter.Finalize())
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to write DDS stream to '%s'", fileName.GetCharArray());
            return false;
        }

        pStream->Release();
    }

    // write output font file
    fileName.Format("%s.font", pOptions->OutputName.GetCharArray());
    FileSystem::BuildOSPath(fileName);
    if ((pStream = FileSystem::OpenFile(fileName.GetCharArray(), BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE)) == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedModalError("Could not open output file '%s'", fileName.GetCharArray());
        return false;
    }

    DF_FONT_HEADER outHeader;
    outHeader.Magic = DF_FONT_HEADER_MAGIC;
    outHeader.Size = sizeof(DF_FONT_HEADER);
    outHeader.FontHeight = pOptions->Height;
    outHeader.TextureCount = m_arrTextures.GetSize();
    outHeader.FirstCharacter = FirstCharacter;
    outHeader.LastCharacter = LastCharacter;
    if (!pStream->Write2(&outHeader, sizeof(outHeader)) ||
        !pStream->Write2(m_arrCharacters.GetBasePointer(), sizeof(DF_FONT_CHARACTER) * m_arrCharacters.GetSize()))
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not write output file '%s'", fileName.GetCharArray());
        pStream->Release();
        return false;
    }

    pStream->Release();
    return true;
}

#endif
