#include "Engine/PrecompiledHeader.h"
#include "Engine/Font.h"
#include "Engine/Texture.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
#include "Renderer/Renderer.h"
Log_SetChannel(Font);

namespace NameTables {
    Y_Define_NameTable(FontType)
        Y_NameTable_VEntry(FONT_TYPE_BITMAP, "Bitmap")
        Y_NameTable_VEntry(FONT_TYPE_SIGNED_DISTANCE_FIELD, "SignedDistanceField")
    Y_NameTable_End()
}

DEFINE_RESOURCE_TYPE_INFO(Font);
DEFINE_RESOURCE_GENERIC_FACTORY(Font);

Font::Font(const ResourceTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_bestRenderingSize(0)
{

}

Font::~Font()
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i] != nullptr)
            m_textures[i]->Release();
    }
}

bool Font::LoadFromStream(const char *fontName, ByteStream *pStream)
{
    uint64 headerOffset = pStream->GetPosition();

    // read header
    DF_FONT_HEADER header;
    if (!pStream->Read2(&header, sizeof(header)) ||
        header.Magic != DF_FONT_HEADER_MAGIC ||
        header.Size < sizeof(header) ||
        header.CharacterDataCount == 0)
    {
        Log_ErrorPrintf("Font file '%s' is invalid or corrupted.", fontName);
        return false;
    }

    // alloc characters and textures
    m_strName = fontName;
    m_bestRenderingSize = header.BestRenderingSize;

    // allocate characters
    m_characterData.Resize(header.CharacterDataCount);

    // read characters
    {
        // seek to characters
        DF_FONT_CHARACTER *pFileCharacters = new DF_FONT_CHARACTER[m_characterData.GetSize()];
        if (!pStream->SeekAbsolute(headerOffset + (uint64)header.CharacterDataOffset) ||
            !pStream->Read2(pFileCharacters, sizeof(DF_FONT_CHARACTER) * m_characterData.GetSize()))
        {
            Log_ErrorPrintf("Font file '%s' is invalid or corrupted.", fontName);
            delete[] pFileCharacters;
            return false;
        }

        // parse them
        for (uint32 characterIndex = 0; characterIndex < m_characterData.GetSize(); characterIndex++)
        {
            const DF_FONT_CHARACTER *pSourceCharacter = &pFileCharacters[characterIndex];
            CharacterData *pDestCharacter = &m_characterData[characterIndex];

            pDestCharacter->CodePoint = pSourceCharacter->CodePoint;
            pDestCharacter->StartU = pSourceCharacter->StartU;
            pDestCharacter->EndU = pSourceCharacter->EndU;
            pDestCharacter->StartV = pSourceCharacter->StartV;
            pDestCharacter->EndV = pSourceCharacter->EndV;
            pDestCharacter->DrawOffsetX = pSourceCharacter->DrawOffsetX;
            pDestCharacter->DrawOffsetY = pSourceCharacter->DrawOffsetY;
            pDestCharacter->DrawWidth = pSourceCharacter->DrawWidth;
            pDestCharacter->DrawHeight = pSourceCharacter->DrawHeight;
            pDestCharacter->Advance = pSourceCharacter->Advance;
            pDestCharacter->TextureIndex = pSourceCharacter->TextureIndex;
            pDestCharacter->IsWhiteSpace = pSourceCharacter->IsWhiteSpace;
        }

        // cleanup
        delete[] pFileCharacters;
    }

    // allocate textures
    if (header.TextureCount > 0)
    {
        SmallString textureName;
        m_textures.Resize(header.TextureCount);
        m_textures.ZeroContents();

        uint64 nextTextureOffset = headerOffset + (uint64)header.TextureOffset;

        for (uint32 textureIndex = 0; textureIndex < m_textures.GetSize(); textureIndex++)
        {
            if (!pStream->SeekAbsolute(nextTextureOffset))
                return false;

            DF_FONT_TEXTURE fontTexture;
            if (!pStream->Read2(&fontTexture, sizeof(fontTexture)) || fontTexture.TextureType != TEXTURE_TYPE_2D)
                return false;

            // calc next texture offset
            nextTextureOffset += sizeof(fontTexture) + (uint64)fontTexture.TextureSize;

            // read texture
            Texture2D *pTexture = new Texture2D();
            textureName.Format("%s_tex%u", fontName, textureIndex);
            if (!pTexture->Load(textureName, pStream))
            {
                Log_ErrorPrintf("Failed to load font texture %u", textureIndex);
                pTexture->Release();
                return false;
            }

            m_textures[textureIndex] = pTexture;
        }
    }

    return true;
}

const Font::CharacterData *Font::GetCharacterDataForCodePoint(uint32 codePoint) const
{
    // binary search the character array
    const CharacterData *pFoundData = m_characterData.BinarySearchKey<uint32>(codePoint, [](const uint32 *pCodePoint, const CharacterData *pCharacterData) {
        return (int32)*pCodePoint - (int32)pCharacterData->CodePoint;
    });

    DebugAssert(pFoundData == nullptr || pFoundData->CodePoint == codePoint);
    return pFoundData;
}

uint32 Font::GetTextWidth(const char *Text, uint32 nCharacters, float Scale) const
{
//     uint32 i;
//     const CharacterData *pCharacterData;
//     float Width = 0.0f;
// 
//     for (i = 0; i < nCharacters; i++)
//     {
//         pCharacterData = GetCharacterData(Text[i]);
//         if (pCharacterData != NULL)
//         {
//             float f = Y_ceilf((float)pCharacterData->x1 - (float)pCharacterData->x0);
//             if (f > Y_ceilf(pCharacterData->xAdvance))
//                 Width += Y_ceilf(f * Scale);
//             else
//                 Width += Y_ceilf(pCharacterData->xAdvance * Scale);
//         }
//     }
// 
//     return (uint32)Y_ceilf(Width);

    if (nCharacters == 0)
        return 0;

    uint32 i;
    const CharacterData *pCharacterData;
    uint32 Width = 0;

    for (i = 0; i < nCharacters; i++)
    {
        pCharacterData = GetCharacterDataForCodePoint(Text[i]);
        if (pCharacterData != nullptr)
        {
            // use max of scaled glyph width and xadvance for last character
            if (i == (nCharacters - 1) && !pCharacterData->IsWhiteSpace)
            {
                float scaledWidth = (pCharacterData->DrawOffsetX + pCharacterData->DrawWidth) * Scale;
                Width += (uint32)Y_roundf(Max(scaledWidth, pCharacterData->Advance * Scale));
            }
            else
            {
                // as normal
                Width += (uint32)Y_roundf(pCharacterData->Advance * Scale);
            }
        }
    }

    return Width;

}

bool Font::CreateDeviceResources() const
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (!m_textures[i]->CreateDeviceResources())
            return false;
    }

    return true;
}

void Font::ReleaseDeviceResources() const
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
        m_textures[i]->ReleaseDeviceResources();
}

