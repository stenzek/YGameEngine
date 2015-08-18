#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/FontGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/ZipArchive.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "Core/DDSReader.h"
#include "Core/DDSWriter.h"
#include "Core/ImageCodec.h"
#include "Core/TexturePacker.h"
Log_SetChannel(FontGenerator);

static const char *STORAGE_FONT_XML_FILENAME = "font.xml";
static const char *STORAGE_FONT_DATA_PREFIX = "codepoint_";
static const uint32 MAXIMUM_CHARACTERS_PER_TEXTURE = 256;

PIXEL_FORMAT FontGenerator::GetFontStoragePixelFormat(FONT_TYPE type)
{
    if (type == FONT_TYPE_BITMAP)
        return PIXEL_FORMAT_R8G8B8A8_UNORM;
    else if (type == FONT_TYPE_SIGNED_DISTANCE_FIELD)
        return PIXEL_FORMAT_R32_FLOAT;
    
    UnreachableCode();
    return PIXEL_FORMAT_COUNT;
}

PIXEL_FORMAT FontGenerator::GetCompiledPixelFormat(FONT_TYPE type)
{
    if (type == FONT_TYPE_BITMAP)
        return PIXEL_FORMAT_R8G8B8A8_UNORM;
    else if (type == FONT_TYPE_SIGNED_DISTANCE_FIELD)
        return PIXEL_FORMAT_R32_FLOAT;

    UnreachableCode();
    return PIXEL_FORMAT_COUNT;
}

uint32 FontGenerator::GetMaximumTextureDimension()
{
    return 16384;
}

FontGenerator::FontGenerator()
    : m_type(FONT_TYPE_COUNT),
      m_characterHeight(0),
      m_useTextureFiltering(false),
      m_generateMipmaps(false)
{

}

FontGenerator::~FontGenerator()
{
    for (uint32 i = 0; i < m_characters.GetSize(); i++)
        delete m_characters[i].pContents;
}

const FontGenerator::Character *FontGenerator::GetCharacterByCodePoint(uint32 codePoint) const
{
    for (uint32 i = 0; i < m_characters.GetSize(); i++)
    {
        if (m_characters[i].CodePoint == codePoint)
            return &m_characters[i];
    }

    return nullptr;
}

FontGenerator::Character *FontGenerator::GetCharacterByCodePoint(uint32 codePoint)
{
    for (uint32 i = 0; i < m_characters.GetSize(); i++)
    {
        if (m_characters[i].CodePoint == codePoint)
            return &m_characters[i];
    }

    return nullptr;
}

bool FontGenerator::AddCharacter(uint32 codePoint, uint32 paddingX, uint32 paddingY, int32 drawOffsetX, int32 drawOffsetY, float advance, const Image *pContents)
{
    if (GetCharacterByCodePoint(codePoint) != nullptr)
        return false;

    // ensure correct format
    if (!pContents->IsValidImage() || pContents->GetPixelFormat() != GetFontStoragePixelFormat(m_type))
        return false;

    Character character;
    character.CodePoint = codePoint;
    character.PaddingX = paddingX;
    character.PaddingY = paddingY;
    character.DrawOffsetX = drawOffsetX;
    character.DrawOffsetY = drawOffsetY;
    character.Advance = advance;
    character.pContents = new Image(*pContents);
    character.IsWhitespace = false;
    m_characters.Add(character);
    return true;
}

bool FontGenerator::AddWhitespaceCharacter(uint32 codePoint, float advance)
{
    if (GetCharacterByCodePoint(codePoint) != nullptr)
        return false;

    Character character;
    character.CodePoint = codePoint;
    character.PaddingX = 0;
    character.PaddingY = 0;
    character.DrawOffsetX = 0;
    character.DrawOffsetY = 0;
    character.Advance = advance;
    character.pContents = nullptr;
    character.IsWhitespace = true;
    m_characters.Add(character);
    return true;
}

bool FontGenerator::RemoveCharacter(uint32 codePoint)
{
    for (uint32 i = 0; i < m_characters.GetSize(); i++)
    {
        if (m_characters[i].CodePoint == codePoint)
        {
            delete m_characters[i].pContents;
            m_characters.OrderedRemove(i);
            return true;
        }
    }

    return false;
}

void FontGenerator::Create(FONT_TYPE type, uint32 characterHeight)
{
    DebugAssert(type < FONT_TYPE_COUNT);
    m_type = type;
    m_characterHeight = characterHeight;
}

bool FontGenerator::Load(const char *fileName, ByteStream *pStream)
{
    ZipArchive *pArchive = ZipArchive::OpenArchiveReadOnly(pStream);
    if (pArchive == nullptr)
    {
        Log_ErrorPrintf("FontGenerator::Load: Could not open '%s' as archive.", fileName);
        return false;
    }

    // load xml
    if (!LoadXML(pArchive))
    {
        delete pArchive;
        return false;
    }

    // load character data
    if (!LoadCharacters(pArchive))
    {
        delete pArchive;
        return false;
    }

    // done
    return true;
}

bool FontGenerator::LoadXML(ZipArchive *pArchive)
{
    AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile("font.xml", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        Log_ErrorPrintf("FontGenerator::Load: Could not open font.xml in archive");
        return false;
    }

    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, "font.xml") || !xmlReader.SkipToElement("font"))
        return false;

    // read attributes
    const char *fontTypeStr = xmlReader.FetchAttribute("type");
    const char *fontCharacterHeightStr = xmlReader.FetchAttribute("character-height");
    const char *fontUseTextureFilteringStr = xmlReader.FetchAttribute("use-texture-filtering");
    const char *fontGenerateMipmapsStr = xmlReader.FetchAttribute("generate-mipmaps");
    if (fontTypeStr == nullptr || fontCharacterHeightStr == nullptr ||
        fontUseTextureFilteringStr == nullptr || fontGenerateMipmapsStr == nullptr ||
        !NameTable_TranslateType(NameTables::FontType, fontTypeStr, &m_type, true) ||
        (m_characterHeight = StringConverter::StringToUInt32(fontCharacterHeightStr)) == 0)
    {
        xmlReader.PrintError("missing or invalid attributes");
        return false;
    }

    // parse remaining attributes
    m_useTextureFiltering = StringConverter::StringToBool(fontUseTextureFilteringStr);
    m_generateMipmaps = StringConverter::StringToBool(fontGenerateMipmapsStr);

    // parse the xml
    for (;;)
    {
        if (!xmlReader.NextToken())
            return false;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 fontSelection = xmlReader.Select("characters");
            if (fontSelection < 0)
                return false;

            switch (fontSelection)
            {
                // characters
            case 0:
                {
                    for (;;)
                    {
                        if (!xmlReader.NextToken())
                            return false;

                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                        {
                            int32 charactersSelection = xmlReader.Select("character");
                            if (charactersSelection < 0)
                                return false;

                            switch (charactersSelection)
                            {
                                // character
                            case 0:
                                {
                                    const char *codePointStr = xmlReader.FetchAttribute("codepoint");
                                    const char *paddingXStr = xmlReader.FetchAttribute("padding-x");
                                    const char *paddingYStr = xmlReader.FetchAttribute("padding-y");
                                    const char *drawOffsetXStr = xmlReader.FetchAttribute("draw-offset-x");
                                    const char *drawOffsetYStr = xmlReader.FetchAttribute("draw-offset-y");
                                    const char *advanceStr = xmlReader.FetchAttribute("advance");
                                    const char *whitespaceStr = xmlReader.FetchAttribute("whitespace");
                                    if (codePointStr == nullptr || paddingXStr == nullptr || paddingYStr == nullptr ||
                                        drawOffsetXStr == nullptr || drawOffsetYStr == nullptr || advanceStr == nullptr ||
                                        whitespaceStr == nullptr)
                                    {
                                        xmlReader.PrintError("missing attributes");
                                        return false;
                                    }

                                    // create character struct
                                    Character character;
                                    character.CodePoint = StringConverter::StringToUInt32(codePointStr);
                                    character.PaddingX = StringConverter::StringToUInt32(paddingXStr);
                                    character.PaddingY = StringConverter::StringToUInt32(paddingYStr);
                                    character.DrawOffsetX = StringConverter::StringToInt32(drawOffsetXStr);
                                    character.DrawOffsetY = StringConverter::StringToInt32(drawOffsetYStr);
                                    character.Advance = StringConverter::StringToFloat(advanceStr);
                                    character.IsWhitespace = StringConverter::StringToBool(whitespaceStr);
                                    character.pContents = nullptr;

                                    // ensure it doesn't exist
                                    for (uint32 i = 0; i < m_characters.GetSize(); i++)
                                    {
                                        if (m_characters[i].CodePoint == character.CodePoint)
                                        {
                                            xmlReader.PrintError("duplicate code point definition: %u", character.CodePoint);
                                            return false;
                                        }
                                    }

                                    // add to list
                                    m_characters.Add(character);

                                    // skip element
                                    if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                        return false;
                                }
                                break;
                            }
                        }
                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                        {
                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "characters") == 0);
                            break;
                        }
                        else
                        {
                            xmlReader.PrintError("parse error");
                            return false;
                        }
                    }
                }
                break;
            }

        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "font") == 0);
            break;
        }
        else
        {
            xmlReader.PrintError("parse error");
            return false;
        }
    }

    // ok
    return true;
}

bool FontGenerator::LoadCharacters(ZipArchive *pArchive)
{
    const char *imageExtension = (m_type == FONT_TYPE_SIGNED_DISTANCE_FIELD) ? ".dds" : ".png";
    PIXEL_FORMAT expectedPixelFormat = GetFontStoragePixelFormat(m_type);
    PathString fileName;

    // loop over characters
    for (uint32 characterIndex = 0; characterIndex < m_characters.GetSize(); characterIndex++)
    {
        Character &character = m_characters[characterIndex];
        if (character.IsWhitespace)
            continue;

        // construct filename
        fileName.Format("%s%u%s", STORAGE_FONT_DATA_PREFIX, character.CodePoint, imageExtension);

        // open it
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
        {
            Log_ErrorPrintf("FontGenerator::LoadCharacters: Can't open file '%s' in archive.", fileName.GetCharArray());
            return false;
        }

        // use dds for distance fields, otherwise png
        if (m_type == FONT_TYPE_SIGNED_DISTANCE_FIELD)
        {
            DDSReader ddsReader;
            if (!ddsReader.Open(fileName, pStream) ||
                !ddsReader.LoadMipLevel(0))
            {
                Log_ErrorPrintf("FontGenerator::LoadCharacters: Failed to parse DDS file '%s' in archive.", fileName.GetCharArray());
                return false;
            }

            // should be the correct pixel format
            if (expectedPixelFormat != ddsReader.GetPixelFormat())
            {
                Log_ErrorPrintf("FontGenerator::LoadCharacters: DDS file '%s' has incorrect pixel format (%s vs %s)", fileName.GetCharArray(), NameTable_GetNameString(NameTables::PixelFormat, ddsReader.GetPixelFormat()), NameTable_GetNameString(NameTables::PixelFormat, expectedPixelFormat));
                return false;
            }

            // load it up
            character.pContents = new Image();
            character.pContents->Create(expectedPixelFormat, ddsReader.GetWidth(), ddsReader.GetHeight(), 1);
            Y_memcpy_stride(character.pContents->GetData(), character.pContents->GetDataRowPitch(), 
                            ddsReader.GetMipLevelData(0, 0), ddsReader.GetMipPitch(0, 0),
                            Min(character.pContents->GetDataRowPitch(), ddsReader.GetMipPitch(0, 0)),
                            ddsReader.GetHeight());
        }
        else
        {
            // allocate image
            character.pContents = new Image();

            // decode the image
            ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(fileName, pStream);
            if (pCodec == nullptr || !pCodec->DecodeImage(character.pContents, fileName, pStream))
            {
                Log_ErrorPrintf("FontGenerator::LoadCharacters: Failed to parse image file '%s' in archive.", fileName.GetCharArray());
                return false;
            }

            // ensure pixel format matches, it's okay if it doesn't
            // freeimage likes to strip away alpha channels when all pixels are opaque..
            if (character.pContents->GetPixelFormat() != expectedPixelFormat && !character.pContents->ConvertPixelFormat(expectedPixelFormat))
            {
                Log_ErrorPrintf("FontGenerator::LoadCharacters: Image file '%s' has incorrect pixel format (%s vs %s) and could not be converted.", fileName.GetCharArray(), NameTable_GetNameString(NameTables::PixelFormat, character.pContents->GetPixelFormat()), NameTable_GetNameString(NameTables::PixelFormat, expectedPixelFormat));
                return false;
            }
        }            
    }

    // done
    return true;
}

bool FontGenerator::Save(ByteStream *pOutputStream) const
{
    ZipArchive *pArchive = ZipArchive::CreateArchive(pOutputStream);
    if (pArchive == nullptr)
    {
        Log_ErrorPrintf("TextureGenerator::Save: Could not create archive.");
        return false;
    }

    if (!SaveXML(pArchive))
    {
        pArchive->DiscardChanges();
        delete pArchive;
        return false;
    }

    if (!SaveCharacters(pArchive))
    {
        pArchive->DiscardChanges();
        delete pArchive;
        return false;
    }

    if (!pArchive->CommitChanges())
        return false;

    delete pArchive;
    return true;
}

bool FontGenerator::SaveXML(ZipArchive *pArchive) const
{
    AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile("font.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        Log_ErrorPrintf("FontGenerator::SaveXML: Could not open font.xml in archive");
        return false;
    }

    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("font");
    xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::FontType, m_type));
    xmlWriter.WriteAttribute("character-height", StringConverter::UInt32ToString(m_characterHeight));
    xmlWriter.WriteAttribute("use-texture-filtering", StringConverter::BoolToString(m_useTextureFiltering));
    xmlWriter.WriteAttribute("generate-mipmaps", StringConverter::BoolToString(m_generateMipmaps));
    {
        xmlWriter.StartElement("characters");
        {
            for (uint32 characterIndex = 0; characterIndex < m_characters.GetSize(); characterIndex++)
            {
                const Character &character = m_characters[characterIndex];

                xmlWriter.StartElement("character");
                xmlWriter.WriteAttribute("codepoint", StringConverter::UInt32ToString(character.CodePoint));
                xmlWriter.WriteAttribute("padding-x", StringConverter::UInt32ToString(character.PaddingX));
                xmlWriter.WriteAttribute("padding-y", StringConverter::UInt32ToString(character.PaddingY));
                xmlWriter.WriteAttribute("draw-offset-x", StringConverter::Int32ToString(character.DrawOffsetX));
                xmlWriter.WriteAttribute("draw-offset-y", StringConverter::Int32ToString(character.DrawOffsetY));
                xmlWriter.WriteAttribute("advance", StringConverter::FloatToString(character.Advance));
                xmlWriter.WriteAttribute("whitespace", StringConverter::BoolToString(character.IsWhitespace));
                xmlWriter.EndElement();     // </character>
            }
        }
        xmlWriter.EndElement();     // </characters>
    }
    xmlWriter.EndElement();     // </font>
    xmlWriter.Close();

    return (!pStream->InErrorState());
}

bool FontGenerator::SaveCharacters(ZipArchive *pArchive) const
{
    const char *imageExtension = (m_type == FONT_TYPE_SIGNED_DISTANCE_FIELD) ? ".dds" : ".png";
    PathString fileName;

    // loop over characters
    for (uint32 characterIndex = 0; characterIndex < m_characters.GetSize(); characterIndex++)
    {
        const Character &character = m_characters[characterIndex];
        if (character.IsWhitespace)
            continue;

        // construct filename
        const Image *pImage = character.pContents;
        fileName.Format("%s%u%s", STORAGE_FONT_DATA_PREFIX, character.CodePoint, imageExtension);

        // open it
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
        {
            Log_ErrorPrintf("FontGenerator::SaveCharacters: Can't open file '%s' in archive.", fileName.GetCharArray());
            return false;
        }

        // use dds for distance fields, otherwise png
        if (m_type == FONT_TYPE_SIGNED_DISTANCE_FIELD)
        {
            DDSWriter ddsWriter;
            ddsWriter.Initialize(pStream);
            if (!ddsWriter.WriteHeader(DDS_TEXTURE_TYPE_2D, pImage->GetPixelFormat(), pImage->GetWidth(), pImage->GetHeight(), 1, 1) ||
                !ddsWriter.WriteMipLevel(0, pImage->GetData(), pImage->GetDataSize()) ||
                !ddsWriter.Finalize())
            {
                Log_ErrorPrintf("FontGenerator::SaveCharacters: Failed to write DDS file '%s' in archive.", fileName.GetCharArray());
                return false;
            }
        }
        else
        {
            // encode the image
            ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(fileName, pStream);
            if (pCodec == nullptr || !pCodec->EncodeImage(fileName, pStream, pImage))
            {
                Log_ErrorPrintf("FontGenerator::SaveCharacters: Failed to write image file '%s' in archive.", fileName.GetCharArray());
                return false;
            }
        }
    }

    return true;
}

class FontCompiler
{
public:
    struct CharacterData
    {
        uint32 CodePoint;
        int32 DrawOffsetX, DrawOffsetY;
        float Advance;
        bool IsWhitespace;
        Image *pContents;

        int32 TextureIndex;
        uint32 AtlasStartX, AtlasEndX;
        uint32 AtlasStartY, AtlasEndY;
    };

private:
    const FontGenerator *m_pGenerator;
    TEXTURE_PLATFORM m_texturePlatform;

    typedef MemArray<CharacterData> CharacterDataArray;
    CharacterDataArray m_characters;

    typedef PODArray<Image *> AtlasImageArray;
    AtlasImageArray m_atlasImages;

    typedef PODArray<TextureGenerator *> TextureGeneratorArray;
    TextureGeneratorArray m_textures;

public:
    FontCompiler(const FontGenerator *pGenerator, TEXTURE_PLATFORM platform)
        : m_pGenerator(pGenerator),
          m_texturePlatform(platform)
    {
    }

    ~FontCompiler()
    {

    }

    bool Compile(ByteStream *pStream)
    {
        if (m_pGenerator->GetCharacterCount() == 0)
            return false;

        BuildCharacters();

        if (!BuildTextures())
            return false;

        if (!GenerateTextures())
            return false;

        if (!WriteCompiledStream(pStream))
            return false;

        return true;
    }

private:
    void BuildCharacters()
    {
        // add references to all images
        for (uint32 i = 0; i < m_pGenerator->GetCharacterCount(); i++)
        {
            const FontGenerator::Character *pCharacter = m_pGenerator->GetCharacterByIndex(i);

            CharacterData data;
            data.CodePoint = pCharacter->CodePoint;
            data.DrawOffsetX = pCharacter->DrawOffsetX;
            data.DrawOffsetY = pCharacter->DrawOffsetY;
            data.Advance = pCharacter->Advance;
            data.IsWhitespace = pCharacter->IsWhitespace;
            data.pContents = pCharacter->pContents;
            data.TextureIndex = -1;
            data.AtlasStartX = data.AtlasEndX = 0;
            data.AtlasStartY = data.AtlasEndY = 0;
            m_characters.Add(data);
        }
        
//         // re-align so that the origin is top-left instead of bottom-left
//         int32 lowestY = m_characters[0].DrawOffsetY;
//         for (uint32 i = 1; i < m_characters.GetSize(); i++)
//             lowestY = Max(lowestY, m_characters[i].DrawOffsetY);
//         for (uint32 i = 0; i < m_characters.GetSize(); i++)
//             m_characters[i].DrawOffsetY -= lowestY;
//         Log_DevPrintf("Moved characters up %d pixels", lowestY);

        // compute the largest overhang, and move everything up
        // this is a giant hack.. but ehh
        int32 largestOverhang = 0;
        for (uint32 i = 0; i < m_characters.GetSize(); i++)
        {
            const FontGenerator::Character *pCharacter = m_pGenerator->GetCharacterByIndex(i);
            if (pCharacter->pContents != nullptr)
                largestOverhang = Max(largestOverhang, m_characters[i].DrawOffsetY + (int32)pCharacter->pContents->GetHeight());
        }
        if (largestOverhang > (int32)m_pGenerator->GetCharacterHeight())
        {
            largestOverhang -= (int32)m_pGenerator->GetCharacterHeight();
            for (uint32 i = 0; i < m_characters.GetSize(); i++)
                m_characters[i].DrawOffsetY -= largestOverhang;
            
            //Log_DevPrintf("Moved characters up %d pixels", largestOverhang);
        }

        // sort the character list in ascending order
        m_characters.Sort([](const CharacterData *pLeft, const CharacterData *pRight) {
            return ((int32)pLeft->CodePoint) - ((int32)pRight->CodePoint);
        });
    }

    bool BuildTextures()
    {
        const uint32 maximumDimensions = FontGenerator::GetMaximumTextureDimension();
        const PIXEL_FORMAT pixelFormat = FontGenerator::GetFontStoragePixelFormat(m_pGenerator->GetType());
        const uint32 paddingAmount = 2;

        // start by trying to pack everything into a single texture
        uint32 charactersToPack = 0;
        uint32 remainingCharacters = 0;
        uint32 currentTextureWidth = 0;
        uint32 currentTextureHeight = 0;

        // count non-whitespace characters
        for (uint32 characterIndex = 0; characterIndex < m_characters.GetSize(); characterIndex++)
        {
            if (!m_characters[characterIndex].IsWhitespace)
            {
                charactersToPack++;
                remainingCharacters++;
            }
        }

        // keep going until we succeed
        while (remainingCharacters > 0)
        {
            // add images
            TexturePacker packer(pixelFormat);
            uint32 packCount = 0;
            for (uint32 i = 0; i < m_characters.GetSize() && packCount < charactersToPack; i++)
            {
                if (m_characters[i].TextureIndex < 0 && !m_characters[i].IsWhitespace)
                {
                    packer.AddImage(m_characters[i].pContents, paddingAmount, &m_characters[i]);
                    packCount++;
                }
            }

            // try packing this number of characters into a single texture
            for (;;)
            {
                // if cursize == 0, guess the dimensions
                if (currentTextureWidth == 0)
                {
                    packer.GuessPackedImageDimensions(&currentTextureWidth, &currentTextureHeight);
                    currentTextureWidth = Min(currentTextureWidth, maximumDimensions);
                    currentTextureHeight = Min(currentTextureHeight, maximumDimensions);
                }

                // attempt to pack them
                Log_DevPrintf("FontCompiler::BuildTextures: Trying to pack %u characters into a %u x %u texture", packCount, currentTextureWidth, currentTextureHeight);
                if (packer.Pack(currentTextureWidth, currentTextureHeight, 0.0f, 0.0f, 0.0f, 0.0f))
                {
                    // pack successful
                    remainingCharacters -= packCount;

                    // update data
                    for (uint32 i = 0; i < m_characters.GetSize(); i++)
                    {
                        CharacterData &data = m_characters[i];
                        if (data.TextureIndex < 0 && !data.IsWhitespace && packer.GetImageLocation(&data, &data.AtlasStartX, &data.AtlasEndX, &data.AtlasStartY, &data.AtlasEndY))
                            data.TextureIndex = (uint32)m_atlasImages.GetSize();
                    }

                    // add the image
                    m_atlasImages.Add(new Image(*packer.GetPackedImage()));
                    break;
                }
                else
                {
                    // pack failed, are we at the maximum texture dimensions?
                    if (currentTextureWidth == maximumDimensions && currentTextureHeight == maximumDimensions)
                    {
                        // try packing less characters, let's try half
                        charactersToPack /= 2;
                        if (charactersToPack == 0)
                        {
                            // we're never gonna be able to do this...
                            return false;
                        }

                        // break back to the outer loop
                        break;
                    }
                    else
                    {
                        // first expand horizontally, then vertically
                        if (currentTextureWidth == currentTextureHeight)
                            currentTextureWidth *= 2;
                        else
                            currentTextureHeight *= 2;

                        // try the pack operation again
                        DebugAssert(currentTextureWidth <= maximumDimensions && currentTextureHeight <= maximumDimensions);
                        continue;
                    }
                }
            }
        }

        // if we got here, everything was packed
        return true;
    }

    bool GenerateTextures()
    {
        const PIXEL_FORMAT finalPixelFormat = FontGenerator::GetCompiledPixelFormat(m_pGenerator->GetType());

        for (uint32 textureIndex = 0; textureIndex < m_atlasImages.GetSize(); textureIndex++)
        {
            // initialize it
            const Image *pSourceImage = m_atlasImages[textureIndex];
            TextureGenerator *pGenerator = new TextureGenerator();
            if (!pGenerator->Create(TEXTURE_TYPE_2D, pSourceImage->GetPixelFormat(), pSourceImage->GetWidth(), pSourceImage->GetHeight(), 1, 1, TEXTURE_USAGE_UI_ASSET))
            {
                Log_ErrorPrintf("FontCompiler::GenerateTextures: Failed to create texture %u", textureIndex);
                delete pGenerator;
                return false;
            }

            // set parameters
            pGenerator->SetTextureFilter((m_pGenerator->GetUseTextureFiltering()) == FONT_TYPE_BITMAP ? TEXTURE_FILTER_ANISOTROPIC : TEXTURE_FILTER_MIN_MAG_MIP_POINT);
            pGenerator->SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_CLAMP);
            pGenerator->SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_CLAMP);
            pGenerator->SetTextureAddressModeW(TEXTURE_ADDRESS_MODE_CLAMP);
            pGenerator->SetGenerateMipmaps(m_pGenerator->GetGenerateMipmaps());
            pGenerator->SetEnablePremultipliedAlpha((m_pGenerator->GetType()) == FONT_TYPE_BITMAP);
            pGenerator->SetSourcePremultipliedAlpha(false);
            pGenerator->SetTextureUsage(TEXTURE_USAGE_UI_ASSET);
            pGenerator->SetEnableTextureCompression(true);
            pGenerator->SetSourceSRGB(false);
            pGenerator->SetEnableSRGB(false);

            // add the image
            if (!pGenerator->SetImage(0, pSourceImage))
            {
                Log_ErrorPrintf("FontCompiler::GenerateTextures: Failed to set texture %u", textureIndex);
                delete pGenerator;
                return false;
            }

            // convert pixel formats
            if (pGenerator->GetPixelFormat() != finalPixelFormat && !pGenerator->ConvertToPixelFormat(finalPixelFormat))
            {
                Log_ErrorPrintf("FontCompiler::GenerateTextures: Failed to convert to final pixel format %s", NameTable_GetNameString(NameTables::PixelFormat, finalPixelFormat));
                delete pGenerator;
                return false;
            }

            // add generator
            m_textures.Add(pGenerator);
        }

        return true;
    }

    bool WriteCompiledStream(ByteStream *pStream)
    {
        // write the header
        uint64 headerOffset = pStream->GetPosition();
        DF_FONT_HEADER header;
        header.Magic = DF_FONT_HEADER_MAGIC;
        header.Size = sizeof(header);
        header.Type = m_pGenerator->GetType();
        header.BestRenderingSize = m_pGenerator->GetCharacterHeight();
        header.CharacterDataCount = 0;
        header.CharacterDataOffset = 0;
        header.TextureCount = 0;
        header.TextureOffset = 0;
        if (!pStream->Write(&header, sizeof(header)))
            return false;

        // write character info
        if (m_characters.GetSize() > 0)
        {
            header.CharacterDataCount = m_characters.GetSize();
            header.CharacterDataOffset = (uint32)(pStream->GetPosition() - headerOffset);
            if (!WriteCompiledCharacterInfo(pStream))
                return false;
        }

        // write textures
        if (m_textures.GetSize() > 0)
        {
            header.TextureCount = m_textures.GetSize();
            header.TextureOffset = (uint32)(pStream->GetPosition() - headerOffset);
            if (!WriteCompiledTextures(pStream))
                return false;
        }

        // rewrite header
        uint64 endOffset = pStream->GetPosition();
        if (!pStream->SeekAbsolute(headerOffset) || !pStream->Write2(&header, sizeof(header)) || !pStream->SeekAbsolute(endOffset))
            return false;

        return true;
    }

    bool WriteCompiledCharacterInfo(ByteStream *pStream)
    {
        for (uint32 characterIndex = 0; characterIndex < m_characters.GetSize(); characterIndex++)
        {
            const CharacterData &cdata = m_characters[characterIndex];

            DF_FONT_CHARACTER odata;
            odata.CodePoint = cdata.CodePoint;
            odata.DrawOffsetX = cdata.DrawOffsetX;
            odata.DrawOffsetY = cdata.DrawOffsetY;
            odata.Advance = cdata.Advance;
            odata.IsWhiteSpace = cdata.IsWhitespace;

            if (!cdata.IsWhitespace)
            {
                DebugAssert(cdata.TextureIndex >= 0);
                const TextureGenerator *pTextureForCharacter = m_textures[cdata.TextureIndex];
                
                odata.DrawWidth = cdata.pContents->GetWidth();
                odata.DrawHeight = cdata.pContents->GetHeight();
                odata.StartU = (float)cdata.AtlasStartX / (float)pTextureForCharacter->GetWidth();
                odata.EndU = (float)cdata.AtlasEndX / (float)pTextureForCharacter->GetWidth();
                odata.StartV = (float)cdata.AtlasStartY / (float)pTextureForCharacter->GetHeight();
                odata.EndV = (float)cdata.AtlasEndY / (float)pTextureForCharacter->GetHeight();
                odata.TextureIndex = (uint32)cdata.TextureIndex;
            }
            else
            {
                odata.DrawWidth = (uint32)Math::Truncate(Math::Round(cdata.Advance));
                odata.DrawHeight = m_pGenerator->GetCharacterHeight();
                odata.StartU = odata.EndU = 0.0f;
                odata.StartV = odata.EndV = 0.0f;
                odata.TextureIndex = 0xFFFFFFFF;
            }

            if (!pStream->Write(&odata, sizeof(odata)))
                return false;
        }

        return true;
    }

    bool WriteCompiledTextures(ByteStream *pStream)
    {
        // compile each texture and write it to the stream
        for (uint32 textureIndex = 0; textureIndex < m_textures.GetSize(); textureIndex++)
        {
//             SmallString foo;
//             foo.Format("dbg%u.tex.zip", textureIndex);
//             ByteStream *str = g_pVirtualFileSystem->OpenFile(foo, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_WRITE);
//             m_textures[textureIndex]->Save(str);
//             str->Release();

            AutoReleasePtr<ByteStream> pMemoryStream = ByteStream_CreateGrowableMemoryStream();
            if (!m_textures[textureIndex]->Compile(pMemoryStream, m_texturePlatform))
            {
                Log_ErrorPrintf("FontCompiler::WriteCompiledTextures: Failed to compile texture %u", textureIndex);
                return false;
            }

            DF_FONT_TEXTURE fontTex;
            fontTex.TextureType = TEXTURE_TYPE_2D;
            fontTex.TextureSize = (uint32)pMemoryStream->GetSize();
            if (!pStream->Write2(&fontTex, sizeof(fontTex)) || !ByteStream_AppendStream(pMemoryStream, pStream))
                return false;
        }

        return true;
    }
};

bool FontGenerator::Compile(ByteStream *pOutputStream, TEXTURE_PLATFORM platform) const
{
    // invoke compiler
    FontCompiler compiler(this, platform);
    return compiler.Compile(pOutputStream);
}

// Interface
BinaryBlob *ResourceCompiler::CompileFont(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.font.zip", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileFont: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    FontGenerator *pGenerator = new FontGenerator();
    if (!pGenerator->Load(sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    pStream->Release();
    pSourceData->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pOutputStream, platform))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
