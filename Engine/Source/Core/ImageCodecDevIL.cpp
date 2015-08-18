#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "Core/PixelFormat.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
Log_SetChannel(ImageCodecDevIL);

#if 0

#define IL_USE_PRAGMA_LIBS 1
#include <IL/il.h>
#include <IL/ilu.h>

struct ILImageFormatMapping
{
    ILint ilChannels;
    ILint ilFormat;
    ILint ilType;
    PIXEL_FORMAT OurPixelFormat;
};

static const ILImageFormatMapping s_ILImageFormatMapping[] =
{
    // chn  format          type                    pf
    { 4,    IL_RGBA,        IL_UNSIGNED_BYTE,       PIXEL_FORMAT_R8G8B8A8_UNORM     },      // default
    { 3,    IL_RGB,         IL_UNSIGNED_BYTE,       PIXEL_FORMAT_R8G8B8_UNORM       },
    { 4,    IL_RGBA,        IL_FLOAT,               PIXEL_FORMAT_R32G32B32A32_FLOAT },
};

static PIXEL_FORMAT GetPixelFormatforILFormat(ILint ilChannels, ILint ilFormat, ILint ilType)
{
    uint32 i;
    for (i = 0; i < countof(s_ILImageFormatMapping); i++)
    {
        const ILImageFormatMapping *m = &s_ILImageFormatMapping[i];
        if (m->ilChannels == ilChannels && m->ilFormat == ilFormat && m->ilType == ilType)
            return m->OurPixelFormat;
    }

    return PIXEL_FORMAT_UNKNOWN;
}

static bool GetILFormatForPixelFormat(ILint *ilChannels, ILint *ilFormat, ILint *ilType, PIXEL_FORMAT pixelFormat)
{
    uint32 i;
    for (i = 0; i < countof(s_ILImageFormatMapping); i++)
    {
        const ILImageFormatMapping *m = &s_ILImageFormatMapping[i];
        if (m->OurPixelFormat == pixelFormat)
        {
            *ilChannels = m->ilChannels;
            *ilFormat = m->ilFormat;
            *ilType = m->ilType;
            return true;
        }
    }

    return false;
}

static void InitializeIL()
{
    static bool ilInitialized = false;
    if (!ilInitialized)
    {
        ilInit();
        iluInit();
        ilInitialized = true;
    }
}

bool __ImageCodecDevIL_ResizeImage(IMAGE_RESIZE_FILTER resizeFilter, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 newWidth, uint32 newHeight, const byte *pInImageData, byte *pOutImageData)
{
    InitializeIL();

    ILint ilChannels, ilFormat, ilType;
    if (!GetILFormatForPixelFormat(&ilChannels, &ilFormat, &ilType, pixelFormat))
    {
        Log_ErrorPrintf("__ImageCodecDevIL_ResizeImage: Unknown pixel format %u (%s)", pixelFormat, PixelFormat_GetPixelFormatInfo(pixelFormat)->Name);
        return false;
    }

    ILuint imageName = 0;
    imageName = ilGenImage();
    if (imageName == 0)
        return false;

    ILenum ilResizeFilter;
    uint32 pixelsCopied;
    bool result = false;
    //uint32 srcImageSize = PixelFormat_CalculateImageSize(pixelFormat, width, height, 1);
    uint32 dstImageSize = PixelFormat_CalculateImageSize(pixelFormat, newWidth, newHeight, 1);

    // bind image
    ilBindImage(imageName);

    // copy image in
    if (!ilTexImage(width, height, 1, (ILubyte)ilChannels, ilFormat, ilType, (void *)pInImageData))
    {
        Log_ErrorPrintf("__ImageCodecDevIL_ResizeImage: Failed to copy image in: %s (%u)", iluGetString(ilGetError()), ilGetError());
        goto CLEANUP;
    }

    // resize it
    switch (resizeFilter)
    {
    case IMAGE_RESIZE_FILTER_BOX:       ilResizeFilter = ILU_SCALE_BOX;         break;
    case IMAGE_RESIZE_FILTER_BILINEAR:  ilResizeFilter = ILU_SCALE_TRIANGLE;    break;
    case IMAGE_RESIZE_FILTER_BICUBIC:   ilResizeFilter = ILU_SCALE_MITCHELL;    break;
    case IMAGE_RESIZE_FILTER_BSPLINE:   ilResizeFilter = ILU_SCALE_BSPLINE;     break;
    case IMAGE_RESIZE_FILTER_LANCZOS3:  ilResizeFilter = ILU_SCALE_LANCZOS3;    break;
    default:
        {
            Log_ErrorPrintf("__ImageCodecDevIL_ResizeImage: Unknown resize filter %u.", (uint32)resizeFilter);
            goto CLEANUP;
        }
        break;
    }

    // set filter and do resize
    iluImageParameter(ILU_FILTER, ilResizeFilter);
    if (!iluScale(newWidth, newHeight, 1))
    {
        Log_ErrorPrintf("__ImageCodecDevIL_ResizeImage: iluScale failed: %s (%u)", iluGetString(ilGetError()), ilGetError());
        goto CLEANUP;
    }

    // copy pixels out
    pixelsCopied = ilCopyPixels(0, 0, 0, newWidth, newHeight, 1, ilFormat, ilType, pOutImageData);
    if (pixelsCopied != dstImageSize)
    {
        Log_ErrorPrintf("__ImageCodecDevIL_ResizeImage: Unexpected pixel count (%u vs %u)", pixelsCopied, dstImageSize);
        goto CLEANUP;
    }

    // ok
    result = true;

CLEANUP:
    ilBindImage(0);
    if (imageName != 0)
        ilDeleteImage(imageName);

    return result;
}

class ImageCodecDevIL : public ImageCodec
{
public:
    const char *GetCodecName() const
    {
        return "DevIL";
    }

    bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyList &decoderOptions = PropertyList::EmptyPropertyList) 
    {
        ILuint imageName = 0;
        byte *pImageMemory = NULL;
        bool result = false;

        InitializeIL();

        // get stream length
        uint32 streamLength = (uint32)pInputStream->GetSize();
        if (streamLength == 0)
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: Zero image length.");
            return false;
        }

        // allocate memory for the entire stream
        pImageMemory = new byte[streamLength];
        if (!pInputStream->Read2(pImageMemory, streamLength))
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: Failed to read image.");
            return false;
        }

        // gen image name
        imageName = ilGenImage();
        if (imageName == 0)
            return false;

        // vars
        uint32 imageWidth, imageHeight, imageDepth;
        ILint ilChannels, ilFormat, ilType;
        PIXEL_FORMAT imagePixelFormat;
        uint32 pixelsCopied;

        // bind image
        ilBindImage(imageName);
        
        // load the image
        if (!ilLoadL(IL_TYPE_UNKNOWN, pImageMemory, streamLength))
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: ilLoadImage failed: %s (%u)", iluGetString(ilGetError()), ilGetError());
            goto CLEANUP;
        }

        // get image params
        imageWidth = ilGetInteger(IL_IMAGE_WIDTH);
        imageHeight = ilGetInteger(IL_IMAGE_HEIGHT);
        imageDepth = ilGetInteger(IL_IMAGE_DEPTH);
        ilChannels = ilGetInteger(IL_IMAGE_CHANNELS);
        ilFormat = ilGetInteger(IL_IMAGE_FORMAT);
        ilType = ilGetInteger(IL_IMAGE_TYPE);
        if (imageWidth == 0 || imageHeight == 0 || imageDepth == 0)
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: Invalid dimensions (%i, %i, %i)", imageWidth, imageHeight, imageDepth);
            goto CLEANUP;
        }

        // map pixelformat
        imagePixelFormat = GetPixelFormatforILFormat(ilChannels, ilFormat, ilType);
        if (imagePixelFormat == PIXEL_FORMAT_UNKNOWN)
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: Unknown pixel format (%i, %i, %i)", ilChannels, ilFormat, ilType);
            goto CLEANUP;
        }

        // setup image
        pOutputImage->Create(imagePixelFormat, imageWidth, imageHeight, imageDepth);

        // copy pixels
        pixelsCopied = ilCopyPixels(0, 0, 0, imageWidth, imageHeight, imageDepth, ilFormat, ilType, pOutputImage->GetData());
        if (pixelsCopied != pOutputImage->GetDataSize())
        {
            Log_ErrorPrintf("ImageCodecDevIL::DecodeImage: Failed to copy pixels (got %u, expected %u)", pixelsCopied, pOutputImage->GetDataSize());
            pOutputImage->Delete();
            goto CLEANUP;
        }

        // done
        result = true;

CLEANUP:
        if (pImageMemory != NULL)
            delete[] pImageMemory;
        if (imageName != 0)
        {
            ilBindImage(0);
            ilDeleteImage(imageName);
        }
        return result;
    }

    bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyList &encoderOptions = PropertyList::EmptyPropertyList) 
    {
        return false;
    }
};

static ImageCodecDevIL s_DevILImageCodec;

ImageCodec *__GetImageCodecDevIL()
{
    return &s_DevILImageCodec;
}

#else

ImageCodec *__GetImageCodecDevIL()
{
    return NULL;
}

#endif
