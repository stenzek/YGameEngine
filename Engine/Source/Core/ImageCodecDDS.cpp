#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "Core/DDSFormat.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
Log_SetChannel(ImageCodecDDS);

class ImageCodecDDS : public ImageCodec
{
public:
    const char *GetCodecName() const
    {
        return "DDS";
    }

    bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyTable &decoderOptions = PropertyTable::EmptyPropertyList)
    {
        uint32 DDSMagic;
        DDS_HEADER DDSHeader;
        if (pInputStream->Read(&DDSMagic, sizeof(uint32)) != sizeof(uint32) || DDSMagic != DDS_MAGIC ||
            pInputStream->Read(&DDSHeader, sizeof(DDS_HEADER)) != sizeof(DDS_HEADER))
        {
            Log_ErrorPrintf("ImageCodecDDS::DecodeImage: Reading header failed.");
            return false;
        }

        // determine pixel format
        PIXEL_FORMAT Format = PIXEL_FORMAT_UNKNOWN;
        if (DDSHeader.ddspf.dwFlags & DDS_RGB)
        {
            Log_ErrorPrintf("ImageCodecDDS::DecodeImage: RGB currently unsupported.");
            return false;
        }
        else if (DDSHeader.ddspf.dwFlags & DDS_FOURCC)
        {
            if (DDSHeader.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '1'))
                Format = PIXEL_FORMAT_BC1_UNORM;
            else if (DDSHeader.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '3'))
                Format = PIXEL_FORMAT_BC2_UNORM;
            else if (DDSHeader.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '5'))
                Format = PIXEL_FORMAT_BC3_UNORM;
            else
            {
                Log_ErrorPrintf("ImageCodecDDS::DecodeImage: Unknown FOURCC %08X", DDSHeader.ddspf.dwFourCC);
                return false;
            }
        }

        if (Format == PIXEL_FORMAT_UNKNOWN)
        {
            Log_ErrorPrintf("ImageCodecDDS::DecodeImage: Could not determine pixel format of image.");
            return false;
        }

        // TODO fix up mipmapping
        uint32 CurWidth = DDSHeader.dwWidth;
        uint32 CurHeight = DDSHeader.dwHeight;
        DebugAssert(DDSHeader.dwDepth == 0);
        
        // create image
        pOutputImage->Create(Format, CurWidth, CurHeight, 1);
        if (pInputStream->Read(pOutputImage->GetData(), pOutputImage->GetDataSize()) != pOutputImage->GetDataSize())
        {
            Log_ErrorPrintf("ImageCodecDDS::DecodeImage: Failed to read %u bytes of image data.");
            return false;
        }

        return true;
    }

    bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyTable &encoderOptions = PropertyTable::EmptyPropertyList)
    {
        return false;
    }
};

static ImageCodecDDS g_ImageCodecDDS;

ImageCodec *__GetImageCodecDDS()
{
    return &g_ImageCodecDDS;
}
