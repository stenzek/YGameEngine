#include "Core/PrecompiledHeader.h"
#include "Core/PixelFormat.h"
#include "YBaseLib/Assert.h"

static const PIXEL_FORMAT_INFO g_PixelFormatInfo[PIXEL_FORMAT_COUNT] =
{
    // Name                                     BitsPerPixel    IsImageFormat   HasAlpha    IsBlockCompressed   BytesPerBlock   BlockSize   UncompressedFormat                  LinearFormat                        ColorMaskRed    ColorMaskGreen  ColorMaskBlue   ColorMaskAlpha  ColorBits   DepthBits   StencilBits
    { "PIXEL_FORMAT_R8_UINT",                   8,              true,           false,      false,              0,              0,          PIXEL_FORMAT_R8_UINT,               PIXEL_FORMAT_R8_UINT,               0x000000FF,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_R8_SINT",                   8,              true,           false,      false,              0,              0,          PIXEL_FORMAT_R8_SINT,               PIXEL_FORMAT_R8_SINT,               0x000000FF,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_R8_UNORM",                  8,              true,           false,      false,              0,              0,          PIXEL_FORMAT_R8_UNORM,              PIXEL_FORMAT_R8_UNORM,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_R8_SNORM",                  8,              true,           false,      false,              0,              0,          PIXEL_FORMAT_R8_SNORM,              PIXEL_FORMAT_R8_SNORM,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_R8G8_UINT",                 16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R8G8_UINT,             PIXEL_FORMAT_R8G8_UINT,             0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R8G8_SINT",                 16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R8G8_SINT,             PIXEL_FORMAT_R8G8_SINT,             0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R8G8_UNORM",                16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R8G8_UNORM,            PIXEL_FORMAT_R8G8_UNORM,            0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R8G8_SNORM",                16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R8G8_SNORM,            PIXEL_FORMAT_R8G8_SNORM,            0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R8G8B8A8_UINT",             32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R8G8B8A8_UINT,         PIXEL_FORMAT_R8G8B8A8_UINT,         0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R8G8B8A8_SINT",             32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R8G8B8A8_SINT,         PIXEL_FORMAT_R8G8B8A8_SINT,         0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R8G8B8A8_UNORM",            32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R8G8B8A8_UNORM,        PIXEL_FORMAT_R8G8B8A8_UNORM,        0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB",       32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,   PIXEL_FORMAT_R8G8B8A8_UNORM,        0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R8G8B8A8_SNORM",            32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R8G8B8A8_SNORM,        PIXEL_FORMAT_R8G8B8A8_SNORM,        0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R9G9B9E5_SHAREDEXP",        32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R9G9B9E5_SHAREDEXP,    PIXEL_FORMAT_R9G9B9E5_SHAREDEXP,    0x000001FF,     0x0003FE00,     0x07FC0000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R10G10B10A2_UINT",          32,             false,          true,       false,              0,              0,          PIXEL_FORMAT_R10G10B10A2_UINT,      PIXEL_FORMAT_R10G10B10A2_UINT,      0x000003FF,     0x000FFC00,     0x3FF00000,     0xC0000000,     30,         0,          0           },
    { "PIXEL_FORMAT_R10G10B10A2_UNORM",         32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R10G10B10A2_UNORM,     PIXEL_FORMAT_R10G10B10A2_UNORM,     0x000003FF,     0x000FFC00,     0x3FF00000,     0xC0000000,     30,         0,          0           },
    { "PIXEL_FORMAT_R11G11B10_FLOAT",           32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R11G11B10_FLOAT,       PIXEL_FORMAT_R11G11B10_FLOAT,       0x000007FF,     0x003FF800,     0xFFC00000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16_UINT",                  16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16_UINT,              PIXEL_FORMAT_R16_UINT,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R16_SINT",                  16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16_SINT,              PIXEL_FORMAT_R16_SINT,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R16_UNORM",                 16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16_UNORM,             PIXEL_FORMAT_R16_UNORM,             0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R16_SNORM",                 16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16_SNORM,             PIXEL_FORMAT_R16_SNORM,             0x000000FF,     0x00000000,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R16_FLOAT",                 16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16_FLOAT,             PIXEL_FORMAT_R16_FLOAT,             0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_R16G16_UINT",               32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16G16_UINT,           PIXEL_FORMAT_R16G16_UINT,           0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16G16_SINT",               32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16G16_SINT,           PIXEL_FORMAT_R16G16_SINT,           0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16G16_UNORM",              32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16G16_UNORM,          PIXEL_FORMAT_R16G16_UNORM,          0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16G16_SNORM",              32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16G16_SNORM,          PIXEL_FORMAT_R16G16_SNORM,          0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16G16_FLOAT",              32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R16G16_FLOAT,          PIXEL_FORMAT_R16G16_FLOAT,          0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R16G16B16A16_UINT",         64,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R16G16B16A16_UINT,     PIXEL_FORMAT_R16G16B16A16_UINT,     0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R16G16B16A16_SINT",         64,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R16G16B16A16_SINT,     PIXEL_FORMAT_R16G16B16A16_SINT,     0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R16G16B16A16_UNORM",        64,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R16G16B16A16_UNORM,    PIXEL_FORMAT_R16G16B16A16_UNORM,    0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R16G16B16A16_SNORM",        64,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R16G16B16A16_SNORM,    PIXEL_FORMAT_R16G16B16A16_SNORM,    0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R16G16B16A16_FLOAT",        64,             true,           true,       false,              0,              0,          PIXEL_FORMAT_R16G16B16A16_FLOAT,    PIXEL_FORMAT_R16G16B16A16_FLOAT,    0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R32_UINT",                  32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32_UINT,              PIXEL_FORMAT_R32_UINT,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R32_SINT",                  32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32_SINT,              PIXEL_FORMAT_R32_SINT,              0x000000FF,     0x00000000,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R32_FLOAT",                 32,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32_FLOAT,             PIXEL_FORMAT_R32_FLOAT,             0x000000FF,     0x00000000,     0x00000000,     0x00000000,     32,         0,          0           },
    { "PIXEL_FORMAT_R32G32_UINT",               64,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32_UINT,           PIXEL_FORMAT_R32G32_UINT,           0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R32G32_SINT",               64,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32_SINT,           PIXEL_FORMAT_R32G32_SINT,           0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R32G32_FLOAT",              64,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32_FLOAT,          PIXEL_FORMAT_R32G32_FLOAT,          0x000000FF,     0x0000FF00,     0x00000000,     0x00000000,     64,         0,          0           },
    { "PIXEL_FORMAT_R32G32B32_UINT",            96,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32B32_UINT,        PIXEL_FORMAT_R32G32B32_UINT,        0x000000FF,     0x0000FF00,     0x00FF0000,     0x00000000,     96,         0,          0           },
    { "PIXEL_FORMAT_R32G32B32_SINT",            96,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32B32_SINT,        PIXEL_FORMAT_R32G32B32_SINT,        0x000000FF,     0x0000FF00,     0x00FF0000,     0x00000000,     96,         0,          0           },
    { "PIXEL_FORMAT_R32G32B32_FLOAT",           96,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R32G32B32_FLOAT,       PIXEL_FORMAT_R32G32B32_FLOAT,       0x000000FF,     0x0000FF00,     0x00FF0000,     0x00000000,     96,         0,          0           },
    { "PIXEL_FORMAT_R32G32B32A32_UINT",         128,            true,           true,       false,              0,              0,          PIXEL_FORMAT_R32G32B32A32_UINT,     PIXEL_FORMAT_R32G32B32A32_UINT,     0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     128,        0,          0           },
    { "PIXEL_FORMAT_R32G32B32A32_SINT",         128,            true,           true,       false,              0,              0,          PIXEL_FORMAT_R32G32B32A32_SINT,     PIXEL_FORMAT_R32G32B32A32_SINT,     0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     128,        0,          0           },
    { "PIXEL_FORMAT_R32G32B32A32_FLOAT",        128,            true,           true,       false,              0,              0,          PIXEL_FORMAT_R32G32B32A32_FLOAT,    PIXEL_FORMAT_R32G32B32A32_FLOAT,    0x000000FF,     0x0000FF00,     0x00FF0000,     0xFF000000,     128,        0,          0           },
    { "PIXEL_FORMAT_B8G8R8A8_UNORM",            32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_B8G8R8A8_UNORM,        PIXEL_FORMAT_B8G8R8A8_UNORM,        0x00FF0000,     0x0000FF00,     0x000000FF,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB",       32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,   PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,   0x00FF0000,     0x0000FF00,     0x000000FF,     0xFF000000,     32,         0,          0           },
    { "PIXEL_FORMAT_B8G8R8X8_UNORM",            32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_B8G8R8X8_UNORM,        PIXEL_FORMAT_B8G8R8X8_UNORM,        0x00FF0000,     0x0000FF00,     0x000000FF,     0x00000000,     24,         0,          0           },
    { "PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB",       32,             true,           true,       false,              0,              0,          PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB,   PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB,   0x00FF0000,     0x0000FF00,     0x000000FF,     0x00000000,     24,         0,          0           },
    { "PIXEL_FORMAT_B5G6R5_UNORM",              16,             true,           false,      false,              0,              0,          PIXEL_FORMAT_B5G6R5_UNORM,          PIXEL_FORMAT_B5G6R5_UNORM,          0x0000F800,     0x000007E0,     0x0000001F,     0x00000000,     16,         0,          0           },
    { "PIXEL_FORMAT_B5G5R5A1_UNORM",            16,             true,           true,       false,              0,              0,          PIXEL_FORMAT_B5G5R5A1_UNORM,        PIXEL_FORMAT_B5G5R5A1_UNORM,        0x0000F800,     0x000007C0,     0x0000003E,     0x00000001,     15,         0,          0           },
    { "PIXEL_FORMAT_BC1_UNORM",                 4,              true,           true,       true,               8,              4,          PIXEL_FORMAT_R8G8B8A8_UNORM,        PIXEL_FORMAT_BC1_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     4,          0,          0           },
    { "PIXEL_FORMAT_BC1_UNORM_SRGB",            4,              true,           true,       true,               8,              4,          PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,   PIXEL_FORMAT_BC1_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     4,          0,          0           },
    { "PIXEL_FORMAT_BC2_UNORM",                 4,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM,        PIXEL_FORMAT_BC2_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     4,          0,          0           },
    { "PIXEL_FORMAT_BC2_UNORM_SRGB",            4,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,   PIXEL_FORMAT_BC2_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     4,          0,          0           },
    { "PIXEL_FORMAT_BC3_UNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM,        PIXEL_FORMAT_BC3_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC3_UNORM_SRGB",            8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,   PIXEL_FORMAT_BC3_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC4_UNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R16_UNORM,             PIXEL_FORMAT_BC4_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC4_SNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R16_SNORM,             PIXEL_FORMAT_BC4_SNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC5_UNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R16G16_UNORM,          PIXEL_FORMAT_BC5_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC5_SNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R16G16_SNORM,          PIXEL_FORMAT_BC5_SNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC6H_UF16",                 8,              false,          true,       true,               16,             4,          PIXEL_FORMAT_R16G16_UNORM,          PIXEL_FORMAT_BC6H_UF16,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC6H_SF16",                 8,              false,          true,       true,               16,             4,          PIXEL_FORMAT_R16G16_SNORM,          PIXEL_FORMAT_BC6H_SF16,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC7_UNORM",                 8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM,        PIXEL_FORMAT_BC7_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_BC7_UNORM_SRGB",            8,              true,           true,       true,               16,             4,          PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,   PIXEL_FORMAT_BC7_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     8,          0,          0           },
    { "PIXEL_FORMAT_D16_UNORM",                 16,             false,          false,      false,              0,              0,          PIXEL_FORMAT_D16_UNORM,             PIXEL_FORMAT_D16_UNORM,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     0,          16,         0           },
    { "PIXEL_FORMAT_D24_UNORM_S8_UINT",         32,             false,          false,      false,              0,              0,          PIXEL_FORMAT_D24_UNORM_S8_UINT,     PIXEL_FORMAT_D24_UNORM_S8_UINT,     0x00000000,     0x00000000,     0x00000000,     0x00000000,     0,          24,         8           },
    { "PIXEL_FORMAT_D32_FLOAT",                 32,             false,          false,      false,              0,              0,          PIXEL_FORMAT_D32_FLOAT,             PIXEL_FORMAT_D32_FLOAT,             0x00000000,     0x00000000,     0x00000000,     0x00000000,     0,          32,         0           },
    { "PIXEL_FORMAT_D32_FLOAT_S8X24_UINT",      64,             false,          false,      false,              0,              0,          PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,  PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,  0x00000000,     0x00000000,     0x00000000,     0x00000000,     0,          32,         8           },
    { "PIXEL_FORMAT_R8G8B8_UNORM",              24,             true,           false,      false,              0,              0,          PIXEL_FORMAT_R8G8B8_UNORM,          PIXEL_FORMAT_R8G8B8_UNORM,          0x000000FF,     0x0000FF00,     0x00FF0000,     0x00000000,     24,         0,          0           },
    { "PIXEL_FORMAT_B8G8R8_UNORM",              24,             true,           false,      false,              0,              0,          PIXEL_FORMAT_B8G8R8_UNORM,          PIXEL_FORMAT_B8G8R8_UNORM,          0x00FF0000,     0x0000FF00,     0x000000FF,     0x00000000,     24,         0,          0           },
};

Y_Define_NameTable(NameTables::PixelFormat)
    Y_NameTable_VEntry(PIXEL_FORMAT_R8_SINT,                       "R8_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8_UINT,                       "R8_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8_UNORM,                      "R8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8_SNORM,                      "R8_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8_SINT,                     "R8G8_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8_UINT,                     "R8G8_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8_UNORM,                    "R8G8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8_SNORM,                    "R8G8_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8A8_SINT,                 "R8G8B8A8_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8A8_UINT,                 "R8G8B8A8_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8A8_UNORM,                "R8G8B8A8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,           "R8G8B8A8_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8A8_SNORM,                "R8G8B8A8_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R9G9B9E5_SHAREDEXP,            "R9G9B9E5_SHAREDEXP")
    Y_NameTable_VEntry(PIXEL_FORMAT_R10G10B10A2_UINT,              "R10G10B10A2_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R10G10B10A2_UNORM,             "R10G10B10A2_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R11G11B10_FLOAT,               "R11G11B10_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16_SINT,                      "R16_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16_UINT,                      "R16_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16_UNORM,                     "R16_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16_SNORM,                     "R16_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16_FLOAT,                     "R16_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16_SINT,                   "R16G16_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16_UINT,                   "R16G16_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16_UNORM,                  "R16G16_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16_SNORM,                  "R16G16_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16_FLOAT,                  "R16G16_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16B16A16_SINT,             "R16G16B16A16_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16B16A16_UINT,             "R16G16B16A16_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16B16A16_UNORM,            "R16G16B16A16_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16B16A16_SNORM,            "R16G16B16A16_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_R16G16B16A16_FLOAT,            "R16G16B16A16_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32_SINT,                      "R32_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32_UINT,                      "R32_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32_FLOAT,                     "R32_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32_SINT,                   "R32G32_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32_UINT,                   "R32G32_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32_FLOAT,                  "R32G32_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32_SINT,                "R32G32B32_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32_UINT,                "R32G32B32_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32_FLOAT,               "R32G32B32_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32A32_SINT,             "R32G32B32A32_SINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32A32_UINT,             "R32G32B32A32_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R32G32B32A32_FLOAT,            "R32G32B32A32_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_B8G8R8A8_UNORM,                "B8G8R8A8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,           "B8G8R8A8_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_B8G8R8X8_UNORM,                "B8G8R8X8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB,           "B8G8R8X8_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_B5G6R5_UNORM,                  "B5G6R5_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_B5G5R5A1_UNORM,                "B5G5R5A1_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC1_UNORM,                     "BC1_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC1_UNORM_SRGB,                "BC1_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC2_UNORM,                     "BC2_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC2_UNORM_SRGB,                "BC2_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC3_UNORM,                     "BC3_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC3_UNORM_SRGB,                "BC3_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC4_UNORM,                     "BC4_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC4_SNORM,                     "BC4_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC5_UNORM,                     "BC5_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC5_SNORM,                     "BC5_SNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC6H_UF16,                     "BC6H_UF16")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC6H_SF16,                     "BC6H_SF16")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC7_UNORM,                     "BC7_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_BC7_UNORM_SRGB,                "BC7_UNORM_SRGB")
    Y_NameTable_VEntry(PIXEL_FORMAT_D16_UNORM,                     "D16_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_D24_UNORM_S8_UINT,             "D24_UNORM_S8_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_D32_FLOAT,                     "D32_FLOAT")
    Y_NameTable_VEntry(PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,          "D32_FLOAT_S8X24_UINT")
    Y_NameTable_VEntry(PIXEL_FORMAT_R8G8B8_UNORM,                  "R8G8B8_UNORM")
    Y_NameTable_VEntry(PIXEL_FORMAT_B8G8R8_UNORM,                  "B8G8R8_UNORM")
Y_NameTable_End()

const char *PixelFormat_GetPixelFormatName(PIXEL_FORMAT Format)
{
    if (Format == PIXEL_FORMAT_UNKNOWN)
        return "PIXEL_FORMAT_UNKNOWN";

    DebugAssert(Format < PIXEL_FORMAT_COUNT);
    return g_PixelFormatInfo[Format].Name;
}

const PIXEL_FORMAT_INFO *PixelFormat_GetPixelFormatInfo(PIXEL_FORMAT Format)
{
    DebugAssert(Format < PIXEL_FORMAT_COUNT);
    return &g_PixelFormatInfo[Format];
}

uint32 PixelFormat_CalculateRowPitch(PIXEL_FORMAT Format, uint32 uWidth)
{
    const PIXEL_FORMAT_INFO *pInfo = PixelFormat_GetPixelFormatInfo(Format);
    if (pInfo->IsBlockCompressed)
    {
        uint32 NumBlocksWide = Max((uint32)1, uWidth / pInfo->BlockSize);
        return NumBlocksWide * pInfo->BytesPerBlock;
    }
    else
    {
        // ensure 32bit alignment
        return ((uWidth * pInfo->BitsPerPixel + 31) / 32) * 4;
    }
}

uint32 PixelFormat_CalculateSlicePitch(PIXEL_FORMAT Format, uint32 uWidth, uint32 uHeight)
{
    const PIXEL_FORMAT_INFO *pInfo = PixelFormat_GetPixelFormatInfo(Format);
    if (pInfo->IsBlockCompressed)
    {
        uint32 NumBlocksWide = Max((uint32)1, uWidth / pInfo->BlockSize);
        uint32 NumBlocksHigh = Max((uint32)1, uHeight / pInfo->BlockSize);
        return NumBlocksWide * pInfo->BytesPerBlock * NumBlocksHigh;
    }
    else
    {
        // ensure 32bit alignment
        return ((uWidth * pInfo->BitsPerPixel + 31) / 32) * 4 * uHeight;
    }
}

uint32 PixelFormat_CalculateImageSize(PIXEL_FORMAT Format, uint32 uWidth, uint32 uHeight, uint32 uDepth)
{
    return PixelFormat_CalculateSlicePitch(Format, uWidth, uHeight) * uDepth;
}

Vector4f PixelFormatHelpers::ConvertRGBAToFloat4(uint32 rgba)
{
    union
    {
        uint32 alignedRGBA;
        uint8 pRGBABytes[4];
    };

    alignedRGBA = rgba;
    
    return Vector4f(float(pRGBABytes[0]) / 255.0f,
                  float(pRGBABytes[1]) / 255.0f,
                  float(pRGBABytes[2]) / 255.0f,
                  float(pRGBABytes[3]) / 255.0f);
}

uint32 PixelFormatHelpers::ConvertFloat4ToRGBA(const Vector4f &rgba)
{   
    return ((uint8)Math::Clamp(Math::Truncate(rgba.r * 255.0f), 0, 255)) |
           ((uint8)Math::Clamp(Math::Truncate(rgba.g * 255.0f), 0, 255) << 8) |
           ((uint8)Math::Clamp(Math::Truncate(rgba.b * 255.0f), 0, 255) << 16) |
           ((uint8)Math::Clamp(Math::Truncate(rgba.a * 255.0f), 0, 255) << 24);
}

void PixelFormat_FlipImageInPlace(void *pPixels, uint32 rowPitch, uint32 rowCount)
{
    DebugAssert(rowPitch > 0 && rowCount > 0);
    byte *pRows = (byte *)pPixels;

    // if greater than 64KB, use a malloced buffer
    byte *pRowBuffer;
    byte *pHeapBuffer = NULL;
    if (rowPitch > 65536)
        pRowBuffer = pHeapBuffer = (byte *)Y_malloc(rowPitch);
    else
        pRowBuffer = (byte *)alloca(rowPitch);

    // divide the rows by 2, we only need to do this many swaps
    uint32 flipCount = rowCount / 2;
    uint32 rowCountMinusOne = rowCount - 1;
    for (uint32 i = 0; i < flipCount; i++)
    {
        byte *pRow1 = pRows + (i * rowPitch);
        byte *pRow2 = pRows + ((rowCountMinusOne - i) * rowPitch);
        Y_memcpy(pRowBuffer, pRow1, rowPitch);
        Y_memcpy(pRow1, pRow2, rowPitch);
        Y_memcpy(pRow2, pRowBuffer, rowPitch);
    }

    // free temp buffer
    Y_free(pHeapBuffer);
}

void PixelFormat_FlipImage(void *pDestinationPixels, const void *pPixels, uint32 rowPitch, uint32 rowCount)
{
    DebugAssert(rowPitch > 0 && rowCount > 0);

    const byte *pSourceRows = (const byte *)pPixels;
    byte *pDestinationRows = (byte *)pDestinationPixels;

    // divide the rows by 2, we only need to do this many swaps
    uint32 flipCount = rowCount / 2;
    uint32 rowCountMinusOne = rowCount - 1;
    for (uint32 i = 0; i < flipCount; i++)
    {
        const byte *pSourceRow1 = pSourceRows + (i * rowPitch);
        const byte *pSourceRow2 = pSourceRows + ((rowCountMinusOne - i) * rowPitch);
        byte *pDestinationRow1 = pDestinationRows + (i * rowPitch);
        byte *pDestinationRow2 = pDestinationRows + ((rowCountMinusOne - i) * rowPitch);
        Y_memcpy(pDestinationRow1, pSourceRow2, rowPitch);
        Y_memcpy(pDestinationRow2, pSourceRow1, rowPitch);
    }

    // handle the middle line in odd sized images
    if (flipCount & 1)
    {
        uint32 oddIndex = flipCount + 1;
        const byte *pSourceRow = pSourceRows + (oddIndex * rowPitch);
        byte *pDestinationRow = pDestinationRows + (oddIndex * rowPitch);
        Y_memcpy(pDestinationRow, pSourceRow, rowPitch);
    }
}

PIXEL_FORMAT PixelFormatHelpers::GetSRGBFormat(PIXEL_FORMAT format)
{
    switch (format)
    {
    case PIXEL_FORMAT_R8G8B8A8_UNORM:   return PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB;
    case PIXEL_FORMAT_B8G8R8A8_UNORM:   return PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB;
    case PIXEL_FORMAT_B8G8R8X8_UNORM:   return PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB;
    case PIXEL_FORMAT_BC1_UNORM:        return PIXEL_FORMAT_BC1_UNORM_SRGB;
    case PIXEL_FORMAT_BC2_UNORM:        return PIXEL_FORMAT_BC2_UNORM_SRGB;
    case PIXEL_FORMAT_BC3_UNORM:        return PIXEL_FORMAT_BC3_UNORM_SRGB;
    case PIXEL_FORMAT_BC7_UNORM:        return PIXEL_FORMAT_BC7_UNORM_SRGB;
    }

    return PIXEL_FORMAT_UNKNOWN;        
}

PIXEL_FORMAT PixelFormatHelpers::GetLinearFormat(PIXEL_FORMAT format)
{
    return PixelFormat_GetPixelFormatInfo(format)->LinearFormat;
}

bool PixelFormatHelpers::IsSRGBFormat(PIXEL_FORMAT format)
{
    return PixelFormat_GetPixelFormatInfo(format)->LinearFormat != format;
}

bool PixelFormatHelpers::IsDepthFormat(PIXEL_FORMAT format)
{
    return (format >= PIXEL_FORMAT_D16_UNORM && format <= PIXEL_FORMAT_D32_FLOAT_S8X24_UINT);
}

Vector3f PixelFormatHelpers::DecodeNormalFromR8G8B8(uint32 rgb)
{
    union
    {
        uint32 alignedRGB;
        uint8 pRGBBytes[4];
    };

    alignedRGB = rgb;
    
    return (Vector3f(float(pRGBBytes[0]) / 255.0f,
                   float(pRGBBytes[1]) / 255.0f,
                   float(pRGBBytes[2]) / 255.0f) - 0.5f) * 2.0f;
}

uint32 PixelFormatHelpers::EncodeNormalAsRG8B8B8(const Vector3f &normal)
{
    union
    {
        uint32 alignedRGB;
        uint8 pRGBBytes[4];
    };

    Vector3f positiveNormal(normal * 0.5f + 0.5f);
    pRGBBytes[0] = (uint8)Math::Clamp(Math::Truncate(normal.x * 255.0f), 0, 255);
    pRGBBytes[1] = (uint8)Math::Clamp(Math::Truncate(normal.y * 255.0f), 0, 255);
    pRGBBytes[2] = (uint8)Math::Clamp(Math::Truncate(normal.z * 255.0f), 0, 255);
    pRGBBytes[3] = 0;
    
    return alignedRGB;
}

Vector3f PixelFormatHelpers::ConvertLinearToSRGB(const Vector3f &color)
{
    //static const float LINEAR_TO_SRGB_EXPONENT = 1.0f / 2.2f;
    //return Vector3f(Math::Pow(color.x, LINEAR_TO_SRGB_EXPONENT), Math::Pow(color.y, LINEAR_TO_SRGB_EXPONENT), Math::Pow(color.z, LINEAR_TO_SRGB_EXPONENT));

    // https://www.opengl.org/registry/specs/EXT/framebuffer_sRGB.txt
    Vector3f outColor;
    for (uint32 i = 0; i < 3; i++)
    {
        float cs;
        float cl = color.ele[i];
        if (cl <= 0.0f)
            cs = 0.0f;
        else if (cl < 0.0031308f)
            cs = 12.92f * cl;
        else if (cl < 1.0f)
            cs = 1.055f * Math::Pow(cl, 0.41666f) - 0.055f;
        else
            cs = 1.0f;

        outColor.ele[i] = cs;
    }

    return outColor;
}

Vector4f PixelFormatHelpers::ConvertLinearToSRGB(const Vector4f &color)
{
    return Vector4f(ConvertLinearToSRGB(color.xyz()), color.w);
}

Vector3f PixelFormatHelpers::ConvertSRGBToLinear(const Vector3f &color)
{
    //static const float SRGB_TO_LINEAR_EXPONENT = 2.2f;
    //return Vector3f(Math::Pow(color.x, SRGB_TO_LINEAR_EXPONENT), Math::Pow(color.y, SRGB_TO_LINEAR_EXPONENT), Math::Pow(color.z, SRGB_TO_LINEAR_EXPONENT));

    // https://www.opengl.org/registry/specs/EXT/framebuffer_sRGB.txt    
    Vector3f outColor;
    for (uint32 i = 0; i < 3; i++)
    {
        float cl;
        float cs = color.ele[i];
        if (cs <= 0.04045f)
            cl = cs / 12.92f;
        else
            cl = Math::Pow((cs + 0.055f) / 1.055f, 2.4f);

        outColor.ele[i] = cl;
    }

    return outColor;
}

Vector4f PixelFormatHelpers::ConvertSRGBToLinear(const Vector4f &color)
{
    return Vector4f(ConvertSRGBToLinear(color.xyz()), color.w);
}

/* Generator code:

import math

def toLinear(cs):
  if (cs <= 0.04045):
    return cs / 12.92
  else:
    return math.pow((cs + 0.055) / 1.055, 2.4)

def toSRGB(cl):
  if (cl <= 0.0):
    return 0.0
  elif (cl < 0.0031308):
    return 12.92 * cl
  elif (cl < 1.0):
    return 1.055 * pow(cl, 0.41666) - 0.055
  else:
    return 1.0

def printLUT(func):
  line = ""
  for i in range(0, 256):
    if (i > 0 and (i % 32) == 0):
      print(line)
      line = ""
    line += str(int(math.floor(func(i / 255.0) * 255.0)))
    line += ", "
  print(line)

printLUT(toSRGB)
printLUT(toLinear)

*/

uint32 PixelFormatHelpers::ConvertLinearToSRGB(const uint32 rgba)
{
    //return ConvertFloat4ToRGBA(ConvertLinearToSRGB(ConvertRGBAToFloat4(rgba)));

//     static const uint8 linearToSRGBLUT[256] = {
//         0, 20, 28, 33, 38, 42, 46, 49, 52, 55, 58, 61, 63, 65, 68, 70, 72, 74, 76, 78, 80, 81, 83, 85, 87, 88, 90, 91, 93, 94, 96, 97,
//         99, 100, 102, 103, 104, 106, 107, 108, 109, 111, 112, 113, 114, 115, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 128, 129, 130, 131, 132, 133, 134, 135,
//         136, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 147, 148, 149, 150, 151, 152, 153, 153, 154, 155, 156, 157, 158, 158, 159, 160, 161, 162, 162,
//         163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 171, 171, 172, 173, 174, 174, 175, 176, 176, 177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185,
//         186, 187, 187, 188, 189, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 197, 198, 199, 199, 200, 200, 201, 202, 202, 203, 203, 204, 205, 205,
//         206, 206, 207, 208, 208, 209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214, 215, 216, 216, 217, 217, 218, 218, 219, 219, 220, 220, 221, 222, 222, 223, 223,
//         224, 224, 225, 225, 226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239,
//         240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255
//     };

    static const uint8 linearToSRGBLUT[256] = {
        0, 12, 21, 28, 33, 38, 42, 46, 49, 52, 55, 58, 61, 63, 66, 68, 70, 73, 75, 77, 79, 81, 82, 84, 86, 88, 89, 91, 93, 94, 96, 97,
        99, 100, 102, 103, 104, 106, 107, 109, 110, 111, 112, 114, 115, 116, 117, 118, 120, 121, 122, 123, 124, 125, 126, 127, 129, 130, 131, 132, 133, 134, 135, 136,
        137, 138, 139, 140, 141, 142, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160, 161, 161, 162, 163, 164,
        165, 165, 166, 167, 168, 168, 169, 170, 171, 171, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187,
        187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194, 194, 195, 196, 196, 197, 197, 198, 199, 199, 200, 201, 201, 202, 202, 203, 204, 204, 205, 205, 206, 206,
        207, 208, 208, 209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224,
        225, 225, 226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 237, 238, 238, 239, 239, 240,
        240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 253, 253, 254, 254, 255
    };

    union
    {
        uint32 alignedRGB;
        uint8 pRGBBytes[4];
    };

    alignedRGB = rgba;
    pRGBBytes[0] = linearToSRGBLUT[pRGBBytes[0]];
    pRGBBytes[1] = linearToSRGBLUT[pRGBBytes[1]];
    pRGBBytes[2] = linearToSRGBLUT[pRGBBytes[2]];
    return alignedRGB;
}

uint32 PixelFormatHelpers::ConvertSRGBToLinear(const uint32 rgba)
{
    //return ConvertFloat4ToRGBA(ConvertSRGBToLinear(rgba));

//     static const uint8 sRGBToLinearLUT[256] = { 
//         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
//         2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11,
//         12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28, 29,
//         29, 30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
//         55, 56, 57, 58, 59, 60, 61, 62, 63, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 90,
//         91, 92, 93, 95, 96, 97, 99, 100, 101, 103, 104, 105, 107, 108, 109, 111, 112, 114, 115, 117, 118, 119, 121, 122, 124, 125, 127, 128, 130, 131, 133, 135,
//         136, 138, 139, 141, 142, 144, 146, 147, 149, 151, 152, 154, 156, 157, 159, 161, 162, 164, 166, 168, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 187, 189,
//         191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 235, 237, 239, 241, 244, 246, 248, 250, 252, 255
//     };

    static const uint8 sRGBToLinearLUT[256] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
        3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12,
        13, 13, 13, 14, 14, 15, 15, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 29,
        29, 30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54,
        55, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 85, 87, 88,
        89, 90, 92, 93, 94, 95, 97, 98, 99, 101, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120, 122, 123, 125, 126, 128, 129, 131, 132,
        134, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 152, 153, 155, 157, 159, 160, 162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 188,
        190, 192, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 235, 237, 239, 241, 243, 245, 248, 250, 252, 255
    };

    union
    {
        uint32 alignedRGB;
        uint8 pRGBBytes[4];
    };

    alignedRGB = rgba;
    pRGBBytes[0] = sRGBToLinearLUT[pRGBBytes[0]];
    pRGBBytes[1] = sRGBToLinearLUT[pRGBBytes[1]];
    pRGBBytes[2] = sRGBToLinearLUT[pRGBBytes[2]];
    return alignedRGB;
}
