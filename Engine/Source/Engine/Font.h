#pragma once
#include "Core/Resource.h"

class Texture2D;

enum FONT_TYPE
{
    FONT_TYPE_BITMAP,
    FONT_TYPE_SIGNED_DISTANCE_FIELD,
    FONT_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(FontType);
}

class Font : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(Font, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(Font);
    
public:
    struct CharacterData
    {
        uint32 CodePoint;
        float StartU;
        float EndU;
        float StartV;
        float EndV;
        int32 DrawOffsetX;
        int32 DrawOffsetY;
        uint32 DrawWidth;
        uint32 DrawHeight;
        float Advance;
        uint32 TextureIndex;
        bool IsWhiteSpace;
    };

public:
    Font(const ResourceTypeInfo *pTypeInfo = &s_TypeInfo);
    ~Font();

    bool LoadFromStream(const char *fontName, ByteStream *pStream);

    uint32 GetHeight() const { return m_bestRenderingSize; }

    const CharacterData *GetCharacterDataForCodePoint(uint32 codePoint) const;
    const CharacterData *GetCharacterDataByIndex(uint32 index) const { return &m_characterData[index]; }
    const uint32 GetCharacterDataCount() const { return m_characterData.GetSize(); }

    const Texture2D *GetTexture(uint32 i) const { return m_textures[i]; }
    uint32 GetTextureCount() const { return m_textures.GetSize(); }

    bool CreateDeviceResources() const;
    void ReleaseDeviceResources() const;

    template<typename T>
    bool GetVertex(const CharacterData *pCharacterData,
                   int32 &x, int32 &y, const float &scale,
                   int32 maxX, int32 maxY, 
                   T vertices[6]) const
    {
        int32 x0, x1;
        int32 y0, y1;
        float fx, fy;

        // check it is valid
        if (pCharacterData->IsWhiteSpace)
        {
            int32 newX = Math::Truncate(Math::Round((float)x + pCharacterData->Advance * scale));
            if (newX > maxX)
                return false;

            x = newX;
            return false;
        }

        // get quad
        // the font expects the y coordinate to reference the base line, not the top, so adjust for this
//         x0 = (int32)Y_ceilf((float)x + pCharacterData->xOffset * scale);
//         y0 = (int32)Y_ceilf((float)y + pCharacterData->yOffset * scale);
//         x1 = (int32)Y_ceilf((float)x0 + (float)((pCharacterData->x1 - pCharacterData->x0) * scale));
//         y1 = (int32)Y_ceilf((float)y0 + (float)((pCharacterData->y1 - pCharacterData->y0) * scale));
//         x0 = (int32)Y_roundf((float)x + pCharacterData->xOffset * scale);
//         y0 = (int32)Y_roundf((float)y + pCharacterData->yOffset * scale);
//         x1 = x0 + (int32)Y_roundf((float)(pCharacterData->x1 - pCharacterData->x0) * scale);
//         y1 = y0 + (int32)Y_roundf((float)(pCharacterData->y1 - pCharacterData->y0) * scale);
        fx = (float)x + (float)pCharacterData->DrawOffsetX * scale;
        fy = (float)y + (float)pCharacterData->DrawOffsetY * scale;
        x0 = (int32)Y_roundf(fx);
        y0 = (int32)Y_roundf(fy);
        x1 = (int32)Y_roundf(fx + (float)pCharacterData->DrawWidth * scale);
        y1 = (int32)Y_roundf(fy + (float)pCharacterData->DrawHeight * scale);

        // if in range
        if (x0 > maxX || x1 > maxX || y0 > maxY || y1 > maxY)
            return false;

        //x += (int32)Y_ceilf(pCharacterData->Advance * scale);
        x += (int32)Y_roundf(pCharacterData->Advance * scale);
        if (pCharacterData->IsWhiteSpace)
            return false;

#define SET_VERTEX(i, px, py, u, v) vertices[i].Position.x = (float)(px); vertices[i].Position.y = (float)(py); \
                                        vertices[i].TexCoord.x = (u); vertices[i].TexCoord.y = (v);

        // t1
        SET_VERTEX(0, x0, y0, pCharacterData->StartU, pCharacterData->StartV);
        SET_VERTEX(1, x0, y1, pCharacterData->StartU, pCharacterData->EndV);
        SET_VERTEX(2, x1, y0, pCharacterData->EndU, pCharacterData->StartV);

        // t2
        SET_VERTEX(3, x0, y1, pCharacterData->StartU, pCharacterData->EndV);
        SET_VERTEX(4, x1, y1, pCharacterData->EndU, pCharacterData->EndV);
        SET_VERTEX(5, x1, y0, pCharacterData->EndU, pCharacterData->StartV);

#undef SET_VERTEX

        return true;
    }

    uint32 GetTextWidth(const char *Text, uint32 nCharacters, float Scale) const;

private:
    uint32 m_bestRenderingSize;

    MemArray<CharacterData> m_characterData;

    PODArray<const Texture2D *> m_textures;
};
