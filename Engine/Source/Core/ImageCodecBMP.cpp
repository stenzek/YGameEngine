#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
Log_SetChannel(ImageCodecBMP);

#pragma pack(push, 1)
struct BMPHeader
{
    uint16 Magic;
    uint32 FileLength;
    uint16 CreatorSpecific1;
    uint16 CreatorSpecific2;
    uint32 DataOffset;
    uint32 SecondHeaderSize;
};

struct BMPHeader2
{
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitCount;
    uint32 Compression;
};
#pragma pack(pop)

class ImageCodecBMP : public ImageCodec
{
public:
    const char *GetCodecName() const
    {
        return "BMP";
    }

    bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyTable &decoderOptions = PropertyTable::EmptyPropertyList)
    {
        BMPHeader Header;
        if (pInputStream->Read(&Header, sizeof(BMPHeader)) != sizeof(BMPHeader))
            return false;

        if (Header.SecondHeaderSize < sizeof(BMPHeader2))
            return false;

        BMPHeader2 Header2;
        if (pInputStream->Read(&Header2, sizeof(BMPHeader2)) != sizeof(BMPHeader2))
            return false;

        if (Header2.Width < 0 || Header2.Height < 0)
        {
            Log_ErrorPrintf("ImageCodecBMP::DecodeImage: Bottom-up currently unsupported.");
            return false;
        }

        // select appropriate pixel format
        PIXEL_FORMAT SelectedFormat;
        if (Header2.BitCount == 1)
            SelectedFormat = PIXEL_FORMAT_R8_UNORM;
        //else if (Header2.BitCount == 4)
        //else if (Header2.BitCount == 8)
        else if (Header2.BitCount == 24)
            SelectedFormat = PIXEL_FORMAT_R8G8B8_UNORM;
        else
        {
            Log_ErrorPrintf("ImageCodecBMP::DecodeImage: Unknown bpp %u", Header2.BitCount);
            return false;
        }

        // check compression format
        if (Header2.Compression != 0)
        {
            Log_ErrorPrintf("ImageCodecBMP::DecodeImage: Unknown compression %u", Header2.Compression);
            return false;
        }

        // seek to position in file
        if (!pInputStream->SeekAbsolute(sizeof(BMPHeader) + Header.DataOffset))
            return false;

        // create the image
        pOutputImage->Create(SelectedFormat, Header2.Width, Header2.Height, 1);

        // calculate pitches
        uint32 BMPPitch = (Header2.Width * Header2.BitCount + 7) / 8;
        if (BMPPitch != pOutputImage->GetDataRowPitch())
        {
            Log_ErrorPrintf("ImageCodecBMP::DecodeImage: Pitch mismatch between us and image: %u / %u", pOutputImage->GetDataRowPitch(), BMPPitch);
            return false;
        }

        byte *pOutData = pOutputImage->GetData();

        // read rows
        int32 i;
        for (i = 0; i < Header2.Height; i++)
        {
            if (pInputStream->Read(pOutData, BMPPitch) != BMPPitch)
                break;

            pOutData += BMPPitch;
        }

        if (i != Header2.Height)
            Log_WarningPrintf("ImageCodecBMP::DecodeImage: Not all rows read.");
        
        return true;
    }

    bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyTable &encoderOptions = PropertyTable::EmptyPropertyList)
    {
        return false;
    }
};

static ImageCodecBMP g_ImageCodecBMP;

ImageCodec *__GetImageCodecBMP()
{
    return &g_ImageCodecBMP;
}
