#include "Engine/Common.h"
#include "ResourceCompiler/FontGenerator.h"

int main_mpfi(int argc, char *argv[])
{
    g_pVirtualFileSystem->Initialize();

    unsigned char g_Font[] = 
    {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x10,0x78,0x38,0x78,0x7c,0x7c,0x3c,0x44,0x38,0x04,0x44,0x40,0x44,0x44,0x38,0x78,
        0x38,0x78,0x38,0x7c,0x44,0x44,0x44,0x44,0x44,0x7c,0x00,0x00,0x40,0x00,0x04,0x00,
        0x18,0x00,0x40,0x10,0x08,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x10,0x38,0x7c,0x08,0x7c,0x1c,0x7c,0x38,0x38,
        0x10,0x28,0x28,0x10,0x00,0x20,0x10,0x08,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x04,0x00,0x20,0x38,0x38,0x70,0x00,0x1c,0x10,0x00,0x1c,0x10,0x70,0x30,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x28,0x44,0x44,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x48,0x40,0x6c,0x44,0x44,0x44,
        0x44,0x44,0x44,0x10,0x44,0x44,0x44,0x44,0x44,0x04,0x00,0x00,0x40,0x00,0x04,0x00,
        0x24,0x00,0x40,0x00,0x00,0x40,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x30,0x44,0x04,0x18,0x40,0x20,0x04,0x44,0x44,
        0x10,0x28,0x28,0x3c,0x44,0x50,0x10,0x10,0x08,0x54,0x10,0x00,0x00,0x00,0x04,0x00,
        0x00,0x08,0x00,0x10,0x44,0x44,0x40,0x40,0x04,0x28,0x00,0x30,0x10,0x18,0x58,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x44,0x44,0x40,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x50,0x40,0x54,0x64,0x44,0x44,
        0x44,0x44,0x40,0x10,0x44,0x44,0x44,0x28,0x28,0x08,0x00,0x38,0x78,0x3c,0x3c,0x38,
        0x20,0x38,0x78,0x30,0x18,0x44,0x10,0x6c,0x78,0x38,0x78,0x3c,0x5c,0x3c,0x3c,0x44,
        0x44,0x44,0x44,0x44,0x7c,0x00,0x4c,0x10,0x04,0x08,0x28,0x78,0x40,0x08,0x44,0x44,
        0x10,0x00,0x7c,0x50,0x08,0x50,0x00,0x20,0x04,0x38,0x10,0x00,0x00,0x00,0x08,0x10,
        0x10,0x10,0x7c,0x08,0x08,0x54,0x40,0x20,0x04,0x44,0x00,0x30,0x10,0x18,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x44,0x78,0x40,0x44,0x78,0x78,0x40,0x7c,0x10,0x04,0x60,0x40,0x54,0x54,0x44,0x78,
        0x44,0x78,0x38,0x10,0x44,0x44,0x54,0x10,0x10,0x10,0x00,0x04,0x44,0x40,0x44,0x44,
        0x78,0x44,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x60,0x40,0x10,0x44,
        0x44,0x44,0x28,0x44,0x08,0x00,0x54,0x10,0x18,0x18,0x48,0x04,0x78,0x10,0x38,0x3c,
        0x10,0x00,0x28,0x38,0x10,0x20,0x00,0x20,0x04,0x10,0x7c,0x00,0x7c,0x00,0x10,0x00,
        0x00,0x20,0x00,0x04,0x10,0x5c,0x40,0x10,0x04,0x00,0x00,0x60,0x10,0x0c,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x7c,0x44,0x40,0x44,0x40,0x40,0x4c,0x44,0x10,0x04,0x50,0x40,0x44,0x4c,0x44,0x40,
        0x54,0x50,0x04,0x10,0x44,0x44,0x54,0x28,0x10,0x20,0x00,0x3c,0x44,0x40,0x44,0x7c,
        0x20,0x44,0x44,0x10,0x08,0x70,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x38,0x10,0x44,
        0x44,0x54,0x10,0x44,0x10,0x00,0x64,0x10,0x20,0x04,0x7c,0x04,0x44,0x20,0x44,0x04,
        0x10,0x00,0x7c,0x14,0x20,0x54,0x00,0x20,0x04,0x38,0x10,0x10,0x00,0x00,0x20,0x10,
        0x10,0x10,0x7c,0x08,0x10,0x58,0x40,0x08,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x44,0x44,0x44,0x44,0x40,0x40,0x44,0x44,0x10,0x44,0x48,0x40,0x44,0x44,0x44,0x40,
        0x48,0x48,0x44,0x10,0x44,0x28,0x6c,0x44,0x10,0x40,0x00,0x44,0x44,0x40,0x44,0x40,
        0x20,0x3c,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x04,0x12,0x4c,
        0x28,0x54,0x28,0x3c,0x20,0x00,0x44,0x10,0x40,0x44,0x08,0x44,0x44,0x20,0x44,0x08,
        0x00,0x00,0x28,0x78,0x44,0x48,0x00,0x10,0x08,0x54,0x10,0x10,0x00,0x00,0x40,0x00,
        0x10,0x08,0x00,0x10,0x00,0x40,0x40,0x04,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x44,0x78,0x38,0x78,0x7c,0x40,0x3c,0x44,0x38,0x38,0x44,0x7c,0x44,0x44,0x38,0x40,
        0x34,0x44,0x38,0x10,0x38,0x10,0x44,0x44,0x10,0x7c,0x00,0x3c,0x78,0x3c,0x3c,0x3c,
        0x20,0x04,0x44,0x38,0x48,0x44,0x38,0x44,0x44,0x38,0x78,0x3c,0x40,0x78,0x0c,0x34,
        0x10,0x6c,0x44,0x04,0x7c,0x00,0x38,0x38,0x7c,0x38,0x08,0x38,0x38,0x20,0x38,0x70,
        0x10,0x00,0x28,0x10,0x00,0x34,0x00,0x08,0x10,0x10,0x00,0x20,0x00,0x10,0x00,0x00,
        0x20,0x04,0x00,0x20,0x10,0x3c,0x70,0x00,0x1c,0x00,0x7c,0x1c,0x10,0x70,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x38,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x40,0x04,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    Image sourceImage;
    sourceImage.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, 1024, 9, 1);
    int idx = 0;
    int end = 1024 * 9 / 8;
    for (int i = 0; i < end; i++)
    {
        unsigned char pValue = g_Font[i];
        for (int j = 0; j < 8; ++j)
        {
            ((uint32 *)sourceImage.GetData())[idx++] = pValue & 0x80 ? (uint32)-1 : 0;
            pValue <<= 1;
        }
    }

    uint32 offsets[256] = { 0 };
    for (uint32 i = 0; i < 256; ++i)
    {
        offsets[i] = 206;
    }
    for (uint32 i = 'A'; i <= 'Z'; ++i)
    {
        offsets[i] = (i - 'A') * 8 + 1;
    }
    for (uint32 i = 'a'; i <= 'z'; ++i)
    {
        offsets[i] = (i - 'a') * 8 + 217;
    }
    for (uint32 i = '0'; i <= '9'; ++i)
    {
        offsets[i] = (i - '0') * 8 + 433;
    }
    for (uint32 i = '!'; i <= '/'; ++i)
    {
        offsets[i] = (i - '!') * 8 + 513;
    }
    for (uint32 i = ':'; i <= '@'; ++i)
    {
        offsets[i] = (i - ':') * 8 + 625 + 8;
    }
    for (uint32 i = '['; i <= '_'; ++i)
    {
        offsets[i] = (i - '[') * 8 + 681 + 8;
    }
    for (uint32 i = '{'; i <= '~'; ++i)
    {
        offsets[i] = (i - '{') * 8 + 721 + 8;
    }

    FontGenerator *gen = new FontGenerator();
    gen->Create(FONT_TYPE_BITMAP, 9);

    Image img;
    img.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, 5, 9, 1);

    for (uint32 i = 0; i < 256; i++)
    {
        if (offsets[i] == 206)
        {
            gen->AddWhitespaceCharacter(i, 5.0f);
            continue;
        }

        img.Blit(0, 0, sourceImage, offsets[i], 0, 5, 9);
        gen->AddCharacter(i, 0, 0, 0, 0, 5.0f, &img);
    }

    ByteStream *pStream = g_pVirtualFileSystem->OpenFile("microprofile.font.zip", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_WRITE);
    gen->Save(pStream);
    pStream->Release();
    return 0;
}