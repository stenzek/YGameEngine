#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Math.h"
#include <cstdio>
Log_SetChannel(ImageCodecFreeImage);

#ifdef HAVE_FREEIMAGE

#include <FreeImage.h>

// Include the .lib
#if Y_PLATFORM_WINDOWS
    #if Y_BUILD_CONFIG_DEBUG
        #pragma comment(lib, "FreeImaged.lib")
    #else
        #pragma comment(lib, "FreeImage.lib")
    #endif
#endif      // Y_PLATFORM_WINDOWS

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ByteStream IO Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned DLL_CALLCONV __FreeImageByteStreamIO_ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
    byte *pWritePointer = reinterpret_cast<byte *>(buffer);
    unsigned read = 0;
    for (unsigned i = 0; i < count; i++)
    {
        if (!reinterpret_cast<ByteStream *>(handle)->Read2(pWritePointer, size))
            break;

        pWritePointer += size;
        read++;
    }

    return read;
}

static unsigned DLL_CALLCONV __FreeImageByteStreamIO_WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
    const byte *pReadPointer = reinterpret_cast<byte *>(buffer);
    unsigned written = 0;
    for (unsigned i = 0; i < count; i++)
    {
        if (!reinterpret_cast<ByteStream *>(handle)->Write2(pReadPointer, size))
            break;

        pReadPointer += size;
        written++;
    }

    return written;
}

static int DLL_CALLCONV __FreeImageByteStreamIO_SeekProc(fi_handle handle, long offset, int origin)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(handle);

    bool res;
    if (origin == SEEK_SET)
        res = pStream->SeekAbsolute((uint64)offset);
    else if (origin == SEEK_CUR)
        res = pStream->SeekRelative((int64)offset);
    else if (origin == SEEK_END)
        res = pStream->SeekToEnd();
    else
        res = false;

    return (res) ? 0 : -1;
}

static long DLL_CALLCONV __FreeImageByteStreamIO_TellProc(fi_handle handle)
{
    return (long)reinterpret_cast<ByteStream *>(handle)->GetPosition();
}

static FreeImageIO s_freeImageByteStreamIO = { __FreeImageByteStreamIO_ReadProc, __FreeImageByteStreamIO_WriteProc, __FreeImageByteStreamIO_SeekProc, __FreeImageByteStreamIO_TellProc };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeImage <-> Pixel Format Conversion
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<PIXEL_FORMAT PIXELFORMAT, int RED_INDEX, int GREEN_INDEX, int BLUE_INDEX>
bool __FreeImagePC_ReadBitmapRGB(Image *pOutputImage, FIBITMAP *pInputBitmap)
{
    uint32 width = FreeImage_GetWidth(pInputBitmap);
    uint32 height = FreeImage_GetHeight(pInputBitmap);
    pOutputImage->Create(PIXELFORMAT, width, height, 1);

    const BYTE *pInputBits = FreeImage_GetBits(pInputBitmap);
    uint32 inputPitch = FreeImage_GetPitch(pInputBitmap);

    byte *pOutputBits = pOutputImage->GetData();
    uint32 outputPitch = pOutputImage->GetDataRowPitch();

    // freeimage coordinate system is upside down
    DebugAssert(height > 0);
    pInputBits += (height - 1) * inputPitch;

    for (uint32 y = 0; y < height; y++)
    {
        const BYTE *pInputPixels = reinterpret_cast<const byte *>(pInputBits);
        byte *pOutputPixels = pOutputBits;

        for (uint32 x = 0; x < width; x++)
        {
            pOutputPixels[BLUE_INDEX] = pInputPixels[FI_RGBA_BLUE];
            pOutputPixels[GREEN_INDEX] = pInputPixels[FI_RGBA_GREEN];
            pOutputPixels[RED_INDEX] = pInputPixels[FI_RGBA_RED];
            pInputPixels += 3;
            pOutputPixels += 3;
        }

        pInputBits -= inputPitch;
        pOutputBits += outputPitch;
    }

    return true;
}

template<PIXEL_FORMAT PIXELFORMAT, int RED_INDEX, int GREEN_INDEX, int BLUE_INDEX, int ALPHA_INDEX>
bool __FreeImagePC_ReadBitmapRGBA(Image *pOutputImage, FIBITMAP *pInputBitmap)
{
    uint32 width = FreeImage_GetWidth(pInputBitmap);
    uint32 height = FreeImage_GetHeight(pInputBitmap);
    pOutputImage->Create(PIXELFORMAT, width, height, 1);

    const BYTE *pInputBits = FreeImage_GetBits(pInputBitmap);
    uint32 inputPitch = FreeImage_GetPitch(pInputBitmap);

    byte *pOutputBits = pOutputImage->GetData();
    uint32 outputPitch = pOutputImage->GetDataRowPitch();

    // freeimage coordinate system is upside down
    DebugAssert(height > 0);
    pInputBits += (height - 1) * inputPitch;

    for (uint32 y = 0; y < height; y++)
    {
        const BYTE *pInputPixels = reinterpret_cast<const byte *>(pInputBits);
        byte *pOutputPixels = pOutputBits;

        for (uint32 x = 0; x < width; x++)
        {
            pOutputPixels[BLUE_INDEX] = pInputPixels[FI_RGBA_BLUE];
            pOutputPixels[GREEN_INDEX] = pInputPixels[FI_RGBA_GREEN];
            pOutputPixels[RED_INDEX] = pInputPixels[FI_RGBA_RED];
            pOutputPixels[ALPHA_INDEX] = pInputPixels[FI_RGBA_ALPHA];
            pInputPixels += 4;
            pOutputPixels += 4;
        }

        pInputBits -= inputPitch;
        pOutputBits += outputPitch;
    }

    return true;
}

bool __FreeImagePC_ReadBitmap(Image *pOutputImage, FIBITMAP *pInputBitmap)
{
    // get image format, color format and bpp
    FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(pInputBitmap);
    FREE_IMAGE_COLOR_TYPE imageColorType = FreeImage_GetColorType(pInputBitmap);
    uint32 imageBPP = FreeImage_GetBPP(pInputBitmap);

    // handle known types
    bool result = false;
    if (imageType == FIT_BITMAP && imageColorType == FIC_RGB && imageBPP == 24)
        result = __FreeImagePC_ReadBitmapRGB<PIXEL_FORMAT_R8G8B8_UNORM, 0, 1, 2>(pOutputImage, pInputBitmap);
    else if (imageType == FIT_BITMAP && (imageColorType == FIC_RGB || imageColorType == FIC_RGBALPHA) && imageBPP == 32)    // freeimage returns FIC_RGB for RGBA images that are opaque
        result = __FreeImagePC_ReadBitmapRGBA<PIXEL_FORMAT_R8G8B8A8_UNORM, 0, 1, 2, 3>(pOutputImage, pInputBitmap);
    else if (imageType == FIT_BITMAP && imageColorType == FIC_PALETTE)
    {
        // transparent?
        if (FreeImage_IsTransparent(pInputBitmap))
        {
            // convert to 32bpp
            FIBITMAP *pConvertedBitmap = FreeImage_ConvertTo32Bits(pInputBitmap);
            if (pConvertedBitmap != nullptr)
            {
                result = __FreeImagePC_ReadBitmap(pOutputImage, pConvertedBitmap);
                FreeImage_Unload(pConvertedBitmap);
            }
        }
        else
        {
            // convert to 24bpp
            FIBITMAP *pConvertedBitmap = FreeImage_ConvertTo24Bits(pInputBitmap);
            if (pConvertedBitmap != nullptr)
            {
                result = __FreeImagePC_ReadBitmap(pOutputImage, pConvertedBitmap);
                FreeImage_Unload(pConvertedBitmap);
            }
        }
    }
    
    // error?
    if (!result)
        Log_ErrorPrintf("__FreeImagePC_ReadBitmap: Reading image failed (type %u, color type %u, bpp %u)", (uint32)imageType, (uint32)imageColorType, imageBPP);

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeImage <-> Pixel Format Conversion
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<int RED_INDEX, int GREEN_INDEX, int BLUE_INDEX>
bool __FreeImagePC_WriteBitmapRGB(FIBITMAP **ppOutputBitmap, const Image *pInputImage)
{
    uint32 width = pInputImage->GetWidth();
    uint32 height = pInputImage->GetHeight();

    // allocate bitmap
    FIBITMAP *pOutputBitmap = FreeImage_AllocateT(FIT_BITMAP, width, height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
    if (pOutputBitmap == NULL)
        return false;

    // get pointers
    BYTE *pOutputBits = FreeImage_GetBits(pOutputBitmap);
    uint32 outputPitch = FreeImage_GetPitch(pOutputBitmap);
    const byte *pInputBits = pInputImage->GetData();
    uint32 inputPitch = pInputImage->GetDataRowPitch();

    // freeimage coordinate system is upside down
    DebugAssert(height > 0);
    pOutputBits = pOutputBits + (height - 1) * outputPitch;

    // write lines
    for (uint32 y = 0; y < height; y++)
    {
        const BYTE *pInputPixels = reinterpret_cast<const byte *>(pInputBits);
        byte *pOutputPixels = pOutputBits;

        for (uint32 x = 0; x < width; x++)
        {
            pOutputPixels[FI_RGBA_BLUE] = pInputPixels[BLUE_INDEX];
            pOutputPixels[FI_RGBA_GREEN] = pInputPixels[GREEN_INDEX];
            pOutputPixels[FI_RGBA_RED] = pInputPixels[RED_INDEX];
            pInputPixels += 3;
            pOutputPixels += 3;
        }

        pOutputBits -= outputPitch;
        pInputBits += inputPitch;
    }

    *ppOutputBitmap = pOutputBitmap;
    return true;
}

template<int RED_INDEX, int GREEN_INDEX, int BLUE_INDEX>
bool __FreeImagePC_WriteBitmapRGBX(FIBITMAP **ppOutputBitmap, const Image *pInputImage)
{
    uint32 width = pInputImage->GetWidth();
    uint32 height = pInputImage->GetHeight();

    // allocate bitmap
    FIBITMAP *pOutputBitmap = FreeImage_AllocateT(FIT_BITMAP, width, height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
    if (pOutputBitmap == NULL)
        return false;

    // get pointers
    BYTE *pOutputBits = FreeImage_GetBits(pOutputBitmap);
    uint32 outputPitch = FreeImage_GetPitch(pOutputBitmap);
    const byte *pInputBits = pInputImage->GetData();
    uint32 inputPitch = pInputImage->GetDataRowPitch();

    // freeimage coordinate system is upside down
    DebugAssert(height > 0);
    pOutputBits = pOutputBits + (height - 1) * outputPitch;

    // write lines
    for (uint32 y = 0; y < height; y++)
    {
        const BYTE *pInputPixels = reinterpret_cast<const byte *>(pInputBits);
        byte *pOutputPixels = pOutputBits;

        for (uint32 x = 0; x < width; x++)
        {
            pOutputPixels[FI_RGBA_BLUE] = pInputPixels[BLUE_INDEX];
            pOutputPixels[FI_RGBA_GREEN] = pInputPixels[GREEN_INDEX];
            pOutputPixels[FI_RGBA_RED] = pInputPixels[RED_INDEX];
            pInputPixels += 4;
            pOutputPixels += 3;
        }

        pOutputBits -= outputPitch;
        pInputBits += inputPitch;
    }

    *ppOutputBitmap = pOutputBitmap;
    return true;
}

template<int RED_INDEX, int GREEN_INDEX, int BLUE_INDEX, int ALPHA_INDEX>
bool __FreeImagePC_WriteBitmapRGBA(FIBITMAP **ppOutputBitmap, const Image *pInputImage)
{
    uint32 width = pInputImage->GetWidth();
    uint32 height = pInputImage->GetHeight();

    // allocate bitmap
    FIBITMAP *pOutputBitmap = FreeImage_AllocateT(FIT_BITMAP, width, height, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
    if (pOutputBitmap == NULL)
        return false;

    // get pointers
    BYTE *pOutputBits = FreeImage_GetBits(pOutputBitmap);
    uint32 outputPitch = FreeImage_GetPitch(pOutputBitmap);
    const byte *pInputBits = pInputImage->GetData();
    uint32 inputPitch = pInputImage->GetDataRowPitch();

    // freeimage coordinate system is upside down
    DebugAssert(height > 0);
    pOutputBits = pOutputBits + (height - 1) * outputPitch;

    // write lines
    for (uint32 y = 0; y < height; y++)
    {
        const BYTE *pInputPixels = reinterpret_cast<const byte *>(pInputBits);
        byte *pOutputPixels = pOutputBits;

        for (uint32 x = 0; x < width; x++)
        {
            pOutputPixels[FI_RGBA_BLUE] = pInputPixels[BLUE_INDEX];
            pOutputPixels[FI_RGBA_GREEN] = pInputPixels[GREEN_INDEX];
            pOutputPixels[FI_RGBA_RED] = pInputPixels[RED_INDEX];
            pOutputPixels[FI_RGBA_ALPHA] = pInputPixels[ALPHA_INDEX];
            pInputPixels += 4;
            pOutputPixels += 4;
        }

        pOutputBits -= outputPitch;
        pInputBits += inputPitch;
    }

    *ppOutputBitmap = pOutputBitmap;
    return true;
}

bool __FreeImagePC_WriteBitmap(FIBITMAP **ppOutputBitmap, const Image *pInputImage)
{
    PIXEL_FORMAT pixelFormat = pInputImage->GetPixelFormat();

    // handle known types
    bool result = false;
    switch (pixelFormat)
    {
    case PIXEL_FORMAT_R8G8B8A8_UNORM:   result = __FreeImagePC_WriteBitmapRGBA<0, 1, 2, 3>(ppOutputBitmap, pInputImage);        break;
    case PIXEL_FORMAT_B8G8R8X8_UNORM:   result = __FreeImagePC_WriteBitmapRGBX<2, 1, 0>(ppOutputBitmap, pInputImage);           break;
    case PIXEL_FORMAT_B8G8R8A8_UNORM:   result = __FreeImagePC_WriteBitmapRGBA<2, 1, 0, 3>(ppOutputBitmap, pInputImage);        break;
    case PIXEL_FORMAT_R8G8B8_UNORM:     result = __FreeImagePC_WriteBitmapRGB<0, 1, 2>(ppOutputBitmap, pInputImage);            break;
    }
    
    // error?
    if (!result)
        Log_ErrorPrintf("__FreeImagePC_WriteBitmap: Writing image failed (PF %s)", PixelFormat_GetPixelFormatInfo(pixelFormat)->Name);

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resize wrapper
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool __ImageCodecFreeImage_ResizeImage(IMAGE_RESIZE_FILTER resizeFilter, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 newWidth, uint32 newHeight, const byte *pInImageData, byte *pOutImageData)
{
    static const FREE_IMAGE_FILTER imageResizeFilterToFIFilter[IMAGE_RESIZE_FILTER_COUNT] =
    {
        FILTER_BOX,         // IMAGE_RESIZE_FILTER_BOX
        FILTER_BILINEAR,    // IMAGE_RESIZE_FILTER_BILINEAR
        FILTER_BICUBIC,     // IMAGE_RESIZE_FILTER_BICUBIC
        FILTER_BSPLINE,     // IMAGE_RESIZE_FILTER_BSPLINE
        FILTER_CATMULLROM,  // IMAGE_RESIZE_FILTER_CATMULLROM
        FILTER_LANCZOS3,    // IMAGE_RESIZE_FILTER_LANCZOS3
    };

    // ugh, extra copies.. optimize me at some point?
    Image temporaryImage;
    temporaryImage.Create(pixelFormat, width, height, 1);
    Y_memcpy(temporaryImage.GetData(), pInImageData, temporaryImage.GetDataSize());

    // copy into a freeimage bitmap
    FIBITMAP *pInputBitmap;
    if (!__FreeImagePC_WriteBitmap(&pInputBitmap, &temporaryImage))
        return false;

    // invoke the resize
    DebugAssert(resizeFilter < IMAGE_RESIZE_FILTER_COUNT);
    FIBITMAP *pRescaledBitmap = FreeImage_Rescale(pInputBitmap, newWidth, newHeight, imageResizeFilterToFIFilter[resizeFilter]);

    // success?
    bool result = false;
    if (pRescaledBitmap != NULL)
    {
        temporaryImage.Delete();
        result = __FreeImagePC_ReadBitmap(&temporaryImage, pRescaledBitmap);
        if (result)
        {
            if (temporaryImage.GetPixelFormat() == pixelFormat)
                Y_memcpy(pOutImageData, temporaryImage.GetData(), temporaryImage.GetDataSize());
            else
                result = false;

            temporaryImage.Delete();
        }

        FreeImage_Unload(pRescaledBitmap);
    }

    // kill original bitmap
    FreeImage_Unload(pInputBitmap);

    // done
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Codec Class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ImageCodecFreeImage : public ImageCodec
{
public:
    virtual const char *GetCodecName() const;
    virtual bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyTable &decoderOptions = PropertyTable::EmptyPropertyList);
    virtual bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyTable &encoderOptions = PropertyTable::EmptyPropertyList);
};

const char *ImageCodecFreeImage::GetCodecName() const
{
    return "FreeImage";
}

bool ImageCodecFreeImage::DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyTable &decoderOptions /* = PropertyList::EmptyPropertyList */)
{
    // detect format
    FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFileTypeFromHandle(&s_freeImageByteStreamIO, reinterpret_cast<fi_handle>(pInputStream), 0);
    if (fiFormat == FIF_UNKNOWN)
    {
        // try by filename
        const char *extension = Y_strrchr(FileName, '.');
        if (extension == nullptr)
            extension = FileName;
        else
            extension++;

        fiFormat = FreeImage_GetFIFFromFormat(extension);
        if (fiFormat == FIF_UNKNOWN)
        {
            Log_ErrorPrintf("ImageCodecFreeImage::DecodeImage: Could not determine FIF for filename '%s'", FileName);
            return false;
        }
    }

    // seek back to start
    if (!pInputStream->SeekAbsolute(0))
        return false;

    // open the file
    FIBITMAP *pFIBitmap = FreeImage_LoadFromHandle(fiFormat, &s_freeImageByteStreamIO, reinterpret_cast<fi_handle>(pInputStream), 0);
    if (pFIBitmap == NULL)
    {
        Log_ErrorPrintf("ImageCodecFreeImage::DecodeImage: FreeImage_LoadFromHandle failed for filename '%s'", FileName);
        return false;
    }

    // dump error
    bool result = __FreeImagePC_ReadBitmap(pOutputImage, pFIBitmap);
    if (!result)
        Log_ErrorPrintf("ImageCodecFreeImage::DecodeImage: __FreeImagePC_ReadBitmap failed for filename '%s'", FileName);

    // free image resources
    FreeImage_Unload(pFIBitmap);
    return result;
}

bool ImageCodecFreeImage::EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyTable &encoderOptions /* = PropertyList::EmptyPropertyList */)
{
    FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFIFFromFilename(FileName);
    if (fiFormat == FIF_UNKNOWN)
    {
        Log_ErrorPrintf("ImageCodecFreeImage::EncodeImage: Could not determine image format for filename '%s'", FileName);
        return false;
    }
    
    FIBITMAP *pFIBitmap;
    if (!__FreeImagePC_WriteBitmap(&pFIBitmap, pInputImage))
    {
        Log_ErrorPrintf("ImageCodecFreeImage::EncodeImage: Failed to write bitmap.", FileName);
        return false;
    }

    // handle options
    uint32 flags = 0;
    switch (fiFormat)
    {
    case FIF_JPEG:
        {
            flags = JPEG_DEFAULT |
                    Math::Clamp<uint32>(encoderOptions.GetPropertyValueDefaultUInt32(ImageCodecEncoderOptions::JPEG_QUALITY_LEVEL, 75), 10, 100);       // jpeg quality
        }
        break;

    case FIF_PNG:
        {
            flags = PNG_DEFAULT |
                    Math::Clamp<uint32>(encoderOptions.GetPropertyValueDefaultUInt32(ImageCodecEncoderOptions::PNG_COMPRESSION_LEVEL), 1, 9);           // png compression speed
        }
        break;
    }
    
    // do the save
    BOOL result = FreeImage_SaveToHandle(fiFormat, pFIBitmap, &s_freeImageByteStreamIO, reinterpret_cast<fi_handle>(pOutputStream), flags);
    if (!result)
        Log_ErrorPrintf("ImageCodecFreeImage::DecodeImage: FreeImage_SaveToHandle failed for filename '%s'", FileName);

    // free bitmap
    FreeImage_Unload(pFIBitmap);
    return (result == TRUE);
}

static ImageCodecFreeImage s_freeImageCodec;

ImageCodec *__GetImageCodecFreeImage()
{
    return &s_freeImageCodec;
}

#else           // HAVE_FREEIMAGE

ImageCodec *__GetImageCodecFreeImage()
{
    return nullptr;
}

#endif          // HAVE_FREEIMAGE
