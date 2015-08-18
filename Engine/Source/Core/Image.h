#pragma once
#include "Core/Common.h"
#include "Core/PixelFormat.h"

enum IMAGE_RESIZE_FILTER
{
    IMAGE_RESIZE_FILTER_BOX,
    IMAGE_RESIZE_FILTER_BILINEAR,
    IMAGE_RESIZE_FILTER_BICUBIC,
    IMAGE_RESIZE_FILTER_BSPLINE,
    IMAGE_RESIZE_FILTER_CATMULLROM,
    IMAGE_RESIZE_FILTER_LANCZOS3,
    IMAGE_RESIZE_FILTER_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(ImageResizeFilter);
}

class Image
{
public:
    Image();
    Image(const Image &copy);
    ~Image();

    // Creates a new image with the specified pixel format and dimensions.
    void Create(PIXEL_FORMAT PixelFormat, uint32 uWidth, uint32 uHeight, uint32 uDepth);

    // Copies another image.
    void Copy(const Image &rCopy);
    bool CopyAndConvertPixelFormat(const Image &rCopy, PIXEL_FORMAT NewFormat);
    bool CopyAndResize(const Image &rCopy, IMAGE_RESIZE_FILTER resizeFilter, uint32 newWidth, uint32 newHeight, uint32 newDepth);

    // Resizes this image.
    bool Resize(IMAGE_RESIZE_FILTER resizeFilter, uint32 newWidth, uint32 newHeight, uint32 newDepth);

    // Flips the image. Cannot be used on compressed formats.
    bool FlipVertical();
    bool FlipHorizontal();

    // Copies part of the contents of another image to this image.
    bool Blit(uint32 dx, uint32 dy, const Image &sourceImage, uint32 sx, uint32 sy, uint32 width, uint32 height);

    // operations. these don't work on compressed formats.
    bool Pad(uint32 top, uint32 right, uint32 bottom, uint32 left, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    bool Clip(uint32 startX, uint32 startY, uint32 endX, uint32 endY);

    // Read/write pixels.
    bool ReadPixels(void *pBuffer, uint32 cbBuffer, uint32 x, uint32 y, uint32 width, uint32 height) const;
    bool WritePixels(const void *pBuffer, uint32 cbBuffer, uint32 x, uint32 y, uint32 width, uint32 height);

    // Retrieve information about the image.
    bool IsValidImage() const;
    PIXEL_FORMAT GetPixelFormat() const { return m_ePixelFormat; }
    uint32 GetWidth() const { return m_uWidth; }
    uint32 GetHeight() const { return m_uHeight; }
    uint32 GetDepth() const { return m_uDepth; }

    // Retrieves pointers to the image data for this image.
    uint32 GetDataSize() const { return m_uDataSize; }
    uint32 GetDataRowPitch() const { return m_uDataRowPitch; }
    uint32 GetDataSlicePitch() const { return m_uDataSlicePitch; }
    const byte *GetData() const { return m_pData; }
    byte *GetData() { return m_pData; }

    // Frees data associated with this image.
    void Delete();

    // Converts the format of this image.
    bool ConvertPixelFormat(PIXEL_FORMAT NewFormat);

    // Changes the pixel format of this image without converting any data. Types must have identical bpp.
    bool SetPixelFormatWithoutConversion(PIXEL_FORMAT newFormat);

    // overloaded operators
    Image &operator=(const Image &copy);

private:
    PIXEL_FORMAT m_ePixelFormat;
    uint32 m_uWidth;
    uint32 m_uHeight;
    uint32 m_uDepth;
    byte *m_pData;
    uint32 m_uDataSize;
    uint32 m_uDataRowPitch;
    uint32 m_uDataSlicePitch;
};
