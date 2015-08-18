#pragma once
#include "ResourceCompiler/Common.h"
#include "ResourceCompiler/TextureGenerator.h"
#include "Engine/Font.h"
#include "Core/PixelFormat.h"
#include "Core/Image.h"

class ZipArchive;

class FontGenerator
{
public:
    struct Character
    {
        uint32 CodePoint;
        uint32 PaddingX, PaddingY;
        int32 DrawOffsetX, DrawOffsetY;
        float Advance;
        bool IsWhitespace;
        Image *pContents;
    };

    // helper to get the storage pixel format for the font type
    static PIXEL_FORMAT GetFontStoragePixelFormat(FONT_TYPE type);

    // helper to get the compiled pixel format for the font type
    static PIXEL_FORMAT GetCompiledPixelFormat(FONT_TYPE type);

    // helper for maximum texture dimensions
    static uint32 GetMaximumTextureDimension();

public:
    FontGenerator();
    ~FontGenerator();

    // accessors
    const FONT_TYPE GetType() const { return m_type; }
    const uint32 GetCharacterHeight() const { return m_characterHeight; }
    const bool GetUseTextureFiltering() const { return m_useTextureFiltering; }
    const bool GetGenerateMipmaps() const { return m_generateMipmaps; }

    // mutators
    void SetUseTextureFiltering(bool enabled) { m_useTextureFiltering = enabled; }
    void SetGenerateMipmaps(bool enabled) { m_generateMipmaps = enabled; }

    // getting character data
    const uint32 GetCharacterCount() const { return m_characters.GetSize(); }
    const Character *GetCharacterByIndex(uint32 index) const { return &m_characters[index]; }
    Character *GetCharacterByIndex(uint32 index) { return &m_characters[index]; }
    const Character *GetCharacterByCodePoint(uint32 codePoint) const;
    Character *GetCharacterByCodePoint(uint32 codePoint);

    // character management
    bool AddCharacter(uint32 codePoint, uint32 paddingX, uint32 paddingY, int32 drawOffsetX, int32 drawOffsetY, float advance, const Image *pContents);
    bool AddWhitespaceCharacter(uint32 codePoint, float advance);
    bool RemoveCharacter(uint32 codePoint);

    // loading/saving
    void Create(FONT_TYPE type, uint32 characterHeight);
    bool Load(const char *fileName, ByteStream *pStream);
    bool Save(ByteStream *pOutputStream) const;

    // compiling
    bool Compile(ByteStream *pOutputStream, TEXTURE_PLATFORM platform) const;

private:
    // loading/saving
    bool LoadXML(ZipArchive *pArchive);
    bool LoadCharacters(ZipArchive *pArchive);
    bool SaveXML(ZipArchive *pArchive) const;
    bool SaveCharacters(ZipArchive *pArchive) const;

    FONT_TYPE m_type;
    uint32 m_characterHeight;
    bool m_useTextureFiltering;
    bool m_generateMipmaps;

    MemArray<Character> m_characters;
};
