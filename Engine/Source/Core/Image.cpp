#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Log.h"
Log_SetChannel(Image);

namespace NameTables {
    Y_Define_NameTable(ImageResizeFilter)
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_BOX, "Box")
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_BILINEAR, "Bilinear")
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_BICUBIC, "Bicubic")
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_BSPLINE, "BSpline")
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_CATMULLROM, "Catmullrom")
        Y_NameTable_VEntry(IMAGE_RESIZE_FILTER_LANCZOS3, "Lanczos3")
    Y_NameTable_End()
}

Image::Image()
{
    m_ePixelFormat = PIXEL_FORMAT_UNKNOWN;
    m_uWidth = 0;
    m_uHeight = 0;
    m_uDepth = 0;
    m_pData = NULL;
    m_uDataSize = 0;
    m_uDataRowPitch = 0;
    m_uDataSlicePitch = 0;
}

Image::Image(const Image &copy)
    : Image()
{
    Copy(copy);
}

Image::~Image()
{
    if (m_pData != NULL)
        Delete();
}

Image &Image::operator=(const Image &copy)
{
    Delete();
    Copy(copy);
    return *this;
}

void Image::Create(PIXEL_FORMAT PixelFormat, uint32 uWidth, uint32 uHeight, uint32 uDepth)
{
    if (m_pData != NULL)
        Delete();

    DebugAssert(uWidth > 0 && uHeight > 0);
    uint32 uRowPitch = PixelFormat_CalculateRowPitch(PixelFormat, uWidth);
    uint32 uSlicePitch = PixelFormat_CalculateSlicePitch(PixelFormat, uWidth, uHeight);
    uint32 uImageSize = PixelFormat_CalculateImageSize(PixelFormat, uWidth, uHeight, uDepth);
    DebugAssert(uImageSize > 0 && uRowPitch > 0 && uSlicePitch > 0);

    // Allocate the bytes for the image.
    m_pData = (byte *)Y_malloc(uImageSize);
    m_uDataSize = uImageSize;
    m_uDataRowPitch = uRowPitch;
    m_uDataSlicePitch = uSlicePitch;
    m_ePixelFormat = PixelFormat;
    m_uWidth = uWidth;
    m_uHeight = uHeight;
    m_uDepth = uDepth;
}

void Image::Copy(const Image &rCopy)
{
    if (!rCopy.IsValidImage())
    {
        Delete();
        return;
    }

    if (!IsValidImage() ||
        m_uWidth != rCopy.m_uWidth ||
        m_uHeight != rCopy.m_uHeight ||
        m_uDepth != rCopy.m_uDepth ||
        m_ePixelFormat != rCopy.m_ePixelFormat)
    {
        Create(rCopy.GetPixelFormat(), rCopy.GetWidth(), rCopy.GetHeight(), rCopy.GetDepth());
    }

    DebugAssert(m_uDataRowPitch == rCopy.m_uDataRowPitch);
    DebugAssert(m_uDataSlicePitch == rCopy.m_uDataSlicePitch);
    DebugAssert(m_uDataSize == rCopy.m_uDataSize);

    // Copy the data across.
    Y_memcpy(m_pData, rCopy.m_pData, rCopy.m_uDataSize);
}

bool Image::IsValidImage() const
{
    if (m_uWidth == 0 || m_uHeight == 0 || m_uDepth == 0)
        return false;

    const PIXEL_FORMAT_INFO *pPFInfo;
    if (m_ePixelFormat == PIXEL_FORMAT_UNKNOWN || ((pPFInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat)) == NULL) ||
        !pPFInfo->IsImageFormat)
    {
        return false;
    }

    return true;
}

void Image::Delete()
{
    if (m_pData != NULL)
    {
        m_uWidth = m_uHeight = m_uDepth = 0;

        Y_free(m_pData);
        m_pData = NULL;
        m_uDataSize = 0;
        m_uDataRowPitch = 0;
        m_uDataSlicePitch = 0;
    }
}

bool Image::ConvertPixelFormat(PIXEL_FORMAT NewFormat)
{
    if (!IsValidImage() || NewFormat == PIXEL_FORMAT_UNKNOWN || m_uDepth != 1)
        return false;

    if (NewFormat != m_ePixelFormat)
    {
        const PIXEL_FORMAT_INFO *pOldFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
        const PIXEL_FORMAT_INFO *pNewFormatInfo = PixelFormat_GetPixelFormatInfo(NewFormat);
        uint32 newFormatRowPitch = PixelFormat_CalculateRowPitch(NewFormat, m_uWidth);
        uint32 newFormatSlicePitch = PixelFormat_CalculateSlicePitch(NewFormat, m_uWidth, m_uHeight);
        uint32 newFormatSize = PixelFormat_CalculateImageSize(NewFormat, m_uWidth, m_uHeight, m_uDepth);
        DebugAssert(newFormatSize > 0 && newFormatRowPitch > 0);

        byte *pNewData = (byte *)Y_malloc(newFormatSize);
        if (!PixelFormat_ConvertPixels(m_uWidth, m_uHeight, m_pData, m_uDataRowPitch, m_ePixelFormat, pNewData, newFormatRowPitch, NewFormat, &newFormatSize))
        {
            Log_ErrorPrintf("Could not convert image from %s to %s, PixelFormat_ConvertPixels returned false.", pOldFormatInfo->Name, pNewFormatInfo->Name);
            Y_free(pNewData);
            return false;
        }

        Y_free(m_pData);
        m_pData = pNewData;
        m_uDataSize = newFormatSize;
        m_uDataRowPitch = newFormatRowPitch;
        m_uDataSlicePitch = newFormatSlicePitch;
        m_ePixelFormat = NewFormat;
    }

    return true;
}

bool Image::CopyAndConvertPixelFormat(const Image &rCopy, PIXEL_FORMAT NewFormat)
{
    // todo: optimize me
    Image tempImage;
    tempImage.Copy(rCopy);
    if (!tempImage.ConvertPixelFormat(NewFormat))
        return false;

    Copy(tempImage);
    return true;
}

bool Image::CopyAndResize(const Image &rCopy, IMAGE_RESIZE_FILTER resizeFilter, uint32 newWidth, uint32 newHeight, uint32 newDepth)
{
    // todo: optimize me
    Image tempImage;
    tempImage.Copy(rCopy);
    if (!tempImage.Resize(resizeFilter, newWidth, newHeight, newDepth))
        return false;

    Copy(tempImage);
    return true;
}

#ifdef HAVE_FREEIMAGE
extern bool __ImageCodecFreeImage_ResizeImage(IMAGE_RESIZE_FILTER resizeFilter, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 newWidth, uint32 newHeight, const byte *pInImageData, byte *pOutImageData);
#endif

bool Image::Resize(IMAGE_RESIZE_FILTER resizeFilter, uint32 newWidth, uint32 newHeight, uint32 newDepth)
{
#ifdef HAVE_FREEIMAGE
    DebugAssert(m_uDepth == 1);
    DebugAssert(IsValidImage());

    Image tempImage;
    tempImage.Create(m_ePixelFormat, newWidth, newHeight, newDepth);

    //if (!__ImageCodecDevIL_ResizeImage(resizeFilter, m_ePixelFormat, m_uWidth, m_uHeight, newWidth, newHeight, m_pData, tempImage.GetData()))
    if (!__ImageCodecFreeImage_ResizeImage(resizeFilter, m_ePixelFormat, m_uWidth, m_uHeight, newWidth, newHeight, m_pData, tempImage.GetData()))
        return false;

    Copy(tempImage);
    return true;
#else
    Log_ErrorPrintf("Image::Resize: Not compiled with FreeImage support.");
    return false;
#endif
}

bool Image::Blit(uint32 dx, uint32 dy, const Image &sourceImage, uint32 sx, uint32 sy, uint32 width, uint32 height)
{
    DebugAssert(sourceImage.IsValidImage());
    if (sourceImage.GetPixelFormat() != m_ePixelFormat ||
        (dx + width) > m_uWidth ||
        (dy + height) > m_uHeight ||
        (sx + width) > sourceImage.GetWidth() ||
        (sy + height) > sourceImage.GetHeight())
    {
        return false;
    }

    // can't blit compressed formats, or non-byte-aligned formats
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->IsBlockCompressed || (pPixelFormatInfo->BitsPerPixel % 8) != 0)
        return false;

    // calculate bytes per pixel
    uint32 bytesPerPixel = pPixelFormatInfo->BitsPerPixel / 8;

    // work out the pointer to the first pixel we are copying from
    const byte *pSourcePointer = sourceImage.GetData() + (sy * sourceImage.GetDataRowPitch()) + sx * bytesPerPixel;

    // work out the pointer to the first pixel we are copying to
    byte *pDestinationPointer = m_pData + (dy * m_uDataRowPitch) + dx * bytesPerPixel;

    // blit each row
    uint32 i;
    for (i = 0; i < height; i++)
    {
        DebugAssert(((pSourcePointer + width * bytesPerPixel) - sourceImage.GetData()) <= (ptrdiff_t)sourceImage.GetDataSize());
        DebugAssert(((pDestinationPointer + width * bytesPerPixel) - m_pData) <= (ptrdiff_t)m_uDataSize);

        Y_memcpy(pDestinationPointer, pSourcePointer, width * bytesPerPixel);
        pSourcePointer += sourceImage.GetDataRowPitch();
        pDestinationPointer += m_uDataRowPitch;
    }

    return true;
}

// bool Image::Pad(uint32 top, uint32 right, uint32 bottom, uint32 left, float r /* = 1.0f */, float g /* = 1.0f */, float b /* = 1.0f */, float a /* = 1.0f */)
// {
// 
// }
// 
// bool Image::Clip(uint32 startX, uint32 startY, uint32 endX, uint32 endY)
// {
// 
// }

bool Image::ReadPixels(void *pBuffer, uint32 cbBuffer, uint32 x, uint32 y, uint32 width, uint32 height) const
{
    DebugAssert(IsValidImage());
    
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->IsBlockCompressed || (pPixelFormatInfo->BitsPerPixel % 8) != 0)
        return false;

    // work out each (copied) row length
    const uint32 bytesPerPixel = pPixelFormatInfo->BitsPerPixel / 8;
    const uint32 rowOffset = bytesPerPixel * x;
    const uint32 rowLength = bytesPerPixel * width;

    // check size
    if ((rowLength * height) > cbBuffer)
        return false;
    
    // get base pointer
    const byte *pSourcePointer = m_pData + (y * m_uDataRowPitch) + rowOffset;
    byte *pDestinationPointer = (byte *)pBuffer;

    // copy rows
    uint32 i;
    for (i = 0; i < height; i++)
    {
        Y_memcpy(pDestinationPointer, pSourcePointer, rowLength);
        pSourcePointer += m_uDataRowPitch;
        pDestinationPointer += rowLength;
    }

    return true;
}

bool Image::WritePixels(const void *pBuffer, uint32 cbBuffer, uint32 x, uint32 y, uint32 width, uint32 height)
{
    DebugAssert(IsValidImage());

    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->IsBlockCompressed || (pPixelFormatInfo->BitsPerPixel % 8) != 0)
        return false;

    // work out each (copied) row length
    const uint32 bytesPerPixel = pPixelFormatInfo->BitsPerPixel / 8;
    const uint32 rowOffset = bytesPerPixel * x;
    const uint32 rowLength = bytesPerPixel * width;

    // check size
    if ((rowLength * height) > cbBuffer)
        return false;

    // get base pointer
    const byte *pSourcePointer = (const byte *)pBuffer;
    byte *pDestinationPointer = m_pData + (y * m_uDataRowPitch) + rowOffset;

    // copy rows
    uint32 i;
    for (i = 0; i < height; i++)
    {
        Y_memcpy(pDestinationPointer, pSourcePointer, rowLength);
        pSourcePointer += m_uDataRowPitch;
        pDestinationPointer += rowLength;
    }

    return true;
}

bool Image::FlipVertical()
{
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->IsBlockCompressed || m_uDepth != 1)
        return false;

    uint32 flipCount = m_uHeight / 2;
    for (uint32 flipRow = 0; flipRow < flipCount; flipRow++)
    {
        uint32 row1 = flipRow;
        uint32 row2 = m_uHeight - 1 - flipRow;
        if (row1 == row2)
            continue;

        byte *pRow1 = m_pData + (row1 * m_uDataRowPitch);
        byte *pRow2 = m_pData + (row2 * m_uDataRowPitch);

        for (uint32 flipColumn = 0; flipColumn < m_uWidth; flipColumn++)
            Swap(pRow1[flipColumn], pRow2[flipColumn]);
    }

    return true;
}

bool Image::FlipHorizontal()
{
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->IsBlockCompressed || m_uDepth != 1)
        return false;

    // fixme
    Panic("fixme");
    return false;
}

bool Image::SetPixelFormatWithoutConversion(PIXEL_FORMAT newFormat)
{
    const PIXEL_FORMAT_INFO *pOldPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    const PIXEL_FORMAT_INFO *pNewPixelFormatInfo = PixelFormat_GetPixelFormatInfo(newFormat);
    if (pOldPixelFormatInfo->IsBlockCompressed != pNewPixelFormatInfo->IsBlockCompressed ||
        pOldPixelFormatInfo->BytesPerBlock != pNewPixelFormatInfo->BytesPerBlock ||
        pOldPixelFormatInfo->BlockSize != pNewPixelFormatInfo->BlockSize ||
        pOldPixelFormatInfo->BitsPerPixel != pNewPixelFormatInfo->BitsPerPixel)
    {
        return false;
    }

    // simply change the format
    DebugAssert(PixelFormat_CalculateImageSize(m_ePixelFormat, m_uWidth, m_uHeight, m_uDepth) == PixelFormat_CalculateImageSize(newFormat, m_uWidth, m_uHeight, m_uDepth));
    m_ePixelFormat = newFormat;
    return true;
}
