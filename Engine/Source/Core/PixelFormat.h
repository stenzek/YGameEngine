#pragma once
#include "Core/Common.h"
#include "YBaseLib/NameTable.h"
#include "MathLib/Vectorf.h"

//#define MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a) ( ((uint32)(r) << 24) | ((uint32)(g) << 16) | ((uint32)(b) << 8) | ((uint32)(a)) )
#define MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a) ( ((uint32)(a) << 24) | ((uint32)(b) << 16) | ((uint32)(g) << 8) | ((uint32)(r)) )
#define MAKE_COLOR_R8G8B8A8_UNORM_INLINE_HEX(rgba) ( ((uint32)((rgba) & 0xff) << 24) | ((uint32)((rgba) >> 8) & 0xff << 16) | ((uint32)((rgba) >> 16) & 0xff << 8) | ((uint32)((rgba) >> 24) & 0xff) )
#define MAKE_COLOR_R8G8B8_UNORM(r, g, b) ((uint32)0xFF000000 | ((uint32)(b) << 16) | ((uint32)(g) << 8) | ((uint32)(r)) )
#define MAKE_COLOR_R8G8B8_UNORM_INLINE_HEX(rgb) ( (uint32)0xFF000000 | ((uint32)(rgba) & 0xff << 16) | ((uint32)((rgba) >> 8) & 0xff << 8) | ((uint32)((rgba) >> 16) & 0xff) )

enum PIXEL_CHANNEL
{
    PIXEL_CHANNEL_RED,
    PIXEL_CHANNEL_GREEN,
    PIXEL_CHANNEL_BLUE,
    PIXEL_CHANNEL_ALPHA,
};

enum PIXEL_FORMAT
{
    //-------------------------------- Color Formats ------------------------------------   
    PIXEL_FORMAT_R8_UINT,
    PIXEL_FORMAT_R8_SINT,
    PIXEL_FORMAT_R8_UNORM,
    PIXEL_FORMAT_R8_SNORM,
    PIXEL_FORMAT_R8G8_UINT,
    PIXEL_FORMAT_R8G8_SINT,
    PIXEL_FORMAT_R8G8_UNORM,
    PIXEL_FORMAT_R8G8_SNORM,
    PIXEL_FORMAT_R8G8B8A8_UINT,
    PIXEL_FORMAT_R8G8B8A8_SINT,
    PIXEL_FORMAT_R8G8B8A8_UNORM,
    PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,
    PIXEL_FORMAT_R8G8B8A8_SNORM,
    PIXEL_FORMAT_R9G9B9E5_SHAREDEXP,
    PIXEL_FORMAT_R10G10B10A2_UINT,
    PIXEL_FORMAT_R10G10B10A2_UNORM,
    PIXEL_FORMAT_R11G11B10_FLOAT,
    PIXEL_FORMAT_R16_UINT,
    PIXEL_FORMAT_R16_SINT,
    PIXEL_FORMAT_R16_UNORM,
    PIXEL_FORMAT_R16_SNORM,
    PIXEL_FORMAT_R16_FLOAT,
    PIXEL_FORMAT_R16G16_UINT,
    PIXEL_FORMAT_R16G16_SINT,
    PIXEL_FORMAT_R16G16_UNORM,
    PIXEL_FORMAT_R16G16_SNORM,
    PIXEL_FORMAT_R16G16_FLOAT,
    PIXEL_FORMAT_R16G16B16A16_UINT,
    PIXEL_FORMAT_R16G16B16A16_SINT,
    PIXEL_FORMAT_R16G16B16A16_UNORM,
    PIXEL_FORMAT_R16G16B16A16_SNORM,
    PIXEL_FORMAT_R16G16B16A16_FLOAT,
    PIXEL_FORMAT_R32_UINT,
    PIXEL_FORMAT_R32_SINT,
    PIXEL_FORMAT_R32_FLOAT,
    PIXEL_FORMAT_R32G32_UINT,
    PIXEL_FORMAT_R32G32_SINT,
    PIXEL_FORMAT_R32G32_FLOAT,
    PIXEL_FORMAT_R32G32B32_UINT,
    PIXEL_FORMAT_R32G32B32_SINT,
    PIXEL_FORMAT_R32G32B32_FLOAT,
    PIXEL_FORMAT_R32G32B32A32_UINT,
    PIXEL_FORMAT_R32G32B32A32_SINT,
    PIXEL_FORMAT_R32G32B32A32_FLOAT,

    //----------------------- Color formats, swizzled for DX9 ---------------------------
    PIXEL_FORMAT_B8G8R8A8_UNORM,        // known as ARGB32
    PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,
    PIXEL_FORMAT_B8G8R8X8_UNORM,        // known as XRGB32
    PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB,
    PIXEL_FORMAT_B5G6R5_UNORM,
    PIXEL_FORMAT_B5G5R5A1_UNORM,

    //-------------------------- Block Compression Formats ------------------------------
    PIXEL_FORMAT_BC1_UNORM,                           // DXT1 in DX9
    PIXEL_FORMAT_BC1_UNORM_SRGB,
    PIXEL_FORMAT_BC2_UNORM,                           // DXT3 in DX9
    PIXEL_FORMAT_BC2_UNORM_SRGB,
    PIXEL_FORMAT_BC3_UNORM,                           // DXT5 in DX9    
    PIXEL_FORMAT_BC3_UNORM_SRGB,
    PIXEL_FORMAT_BC4_UNORM,
    PIXEL_FORMAT_BC4_SNORM,
    PIXEL_FORMAT_BC5_UNORM,
    PIXEL_FORMAT_BC5_SNORM,
    PIXEL_FORMAT_BC6H_UF16,
    PIXEL_FORMAT_BC6H_SF16,
    PIXEL_FORMAT_BC7_UNORM,
    PIXEL_FORMAT_BC7_UNORM_SRGB,

    //-------------------------------- Depth Format -------------------------------------
    PIXEL_FORMAT_D16_UNORM,
    PIXEL_FORMAT_D24_UNORM_S8_UINT,
    PIXEL_FORMAT_D32_FLOAT,
    PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,

    //----------------Other Formats, not necessarily available on GPU -------------------
    PIXEL_FORMAT_R8G8B8_UNORM,
    PIXEL_FORMAT_B8G8R8_UNORM,
    
    //-----------------------------------------------------------------------------------
    PIXEL_FORMAT_COUNT,
    PIXEL_FORMAT_UNKNOWN = 999,
};

// nametable
namespace NameTables {
    Y_Declare_NameTable(PixelFormat);
}

struct PIXEL_FORMAT_INFO
{
    const char *Name;                   // Name of pixel format
    uint32 BitsPerPixel;                // Number of bits per pixel
    bool IsImageFormat;                 // If it can store colour/image data, true. false for other formats, e.g. depth/stencil.
    bool HasAlpha;                      // Has alpha channel.
    bool IsBlockCompressed;             // True if the image uses block compression.
    uint32 BytesPerBlock;               // If using block compression, the number of bytes per block.
    uint32 BlockSize;                   // If using block compression, the number of pixels in both dimensions for each block.
    PIXEL_FORMAT UncompressedFormat;    // Internal format of compressed textures.
    PIXEL_FORMAT LinearFormat;          // Whether the image format contains pixels in SRGB colour space.
    uint32 ColorMaskRed;
    uint32 ColorMaskGreen;
    uint32 ColorMaskBlue;
    uint32 ColorMaskAlpha;
    uint32 ColorBits;
    uint32 DepthBits;
    uint32 StencilBits;
};

const char *PixelFormat_GetPixelFormatName(PIXEL_FORMAT Format);
const PIXEL_FORMAT_INFO *PixelFormat_GetPixelFormatInfo(PIXEL_FORMAT Format);
uint32 PixelFormat_CalculateRowPitch(PIXEL_FORMAT Format, uint32 uWidth);
uint32 PixelFormat_CalculateSlicePitch(PIXEL_FORMAT Format, uint32 uWidth, uint32 uHeight);
uint32 PixelFormat_CalculateImageNumRows(PIXEL_FORMAT Format, uint32 Width, uint32 Height);
uint32 PixelFormat_CalculateImageSize(PIXEL_FORMAT Format, uint32 uWidth, uint32 uHeight, uint32 uDepth);
bool PixelFormat_ConvertPixels(uint32 Width, uint32 Height, const void *SourcePixels, uint32 SourcePitch, PIXEL_FORMAT SourceFormat, void *DestinationPixels, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat, uint32 *DestinationPixelSize);
void PixelFormat_FlipImageInPlace(void *pPixels, uint32 rowPitch, uint32 rowCount);
void PixelFormat_FlipImage(void *pDestinationPixels, const void *pPixels, uint32 rowPitch, uint32 rowCount);

namespace PixelFormatHelpers {

bool IsSRGBFormat(PIXEL_FORMAT format);
PIXEL_FORMAT GetSRGBFormat(PIXEL_FORMAT format);
PIXEL_FORMAT GetLinearFormat(PIXEL_FORMAT format);
bool IsDepthFormat(PIXEL_FORMAT format);
Vector4f ConvertRGBAToFloat4(uint32 rgba);
uint32 ConvertFloat4ToRGBA(const Vector4f &rgba);
Vector3f DecodeNormalFromR8G8B8(uint32 rgb);
uint32 EncodeNormalAsRG8B8B8(const Vector3f &normal);
Vector3f ConvertLinearToSRGB(const Vector3f &color);
Vector4f ConvertLinearToSRGB(const Vector4f &color);
uint32 ConvertLinearToSRGB(const uint32 rgba);
Vector3f ConvertSRGBToLinear(const Vector3f &color);
Vector4f ConvertSRGBToLinear(const Vector4f &color);
uint32 ConvertSRGBToLinear(const uint32 rgba);


};
