#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/CString.h"

namespace ImageCodecEncoderOptions
{
    const char *JPEG_QUALITY_LEVEL = "JPEG_QUALITYLEVEL";
    const char *PNG_COMPRESSION_LEVEL = "PNG_COMPRESSION_LEVEL";
}

namespace ImageCodecDecoderOptions
{

}

// functions for retrieving interfaces
extern ImageCodec *__GetImageCodecDevIL();
extern ImageCodec *__GetImageCodecFreeImage();
//extern ImageCodec *__GetImageCodecJPEG();
//extern ImageCodec *__GetImageCodecDDS();
//extern ImageCodec *__GetImageCodecBMP();

ImageCodec *ImageCodec::GetImageCodecForFileName(const char *fileName)
{
    // find the extension
    const char *pExtension = Y_strrchr(fileName, '.');
    if (pExtension == NULL)
        //return __GetImageCodecDevIL();        
        return __GetImageCodecFreeImage();

    pExtension++;

    // jpeg
//     if (Y_stricmp(pExtension, "jpg") == 0 || Y_stricmp(pExtension, "jpeg") == 0)
//         return __GetImageCodecJPEG();
//     else if (Y_stricmp(pExtension, "dds") == 0)
//         return __GetImageCodecDDS();
//     else if (Y_stricmp(pExtension, "bmp") == 0)
//         return __GetImageCodecBMP();

    //return __GetImageCodecDevIL();
    return __GetImageCodecFreeImage();
}

ImageCodec *ImageCodec::GetImageCodecForStream(const char *fileName, ByteStream *pStream)
{
    return GetImageCodecForFileName(fileName);
}
