#pragma once

// adapted from DDS.h
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)

#ifndef DDS_MAKEFOURCC
    #define DDS_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32)(byte)(ch0) | ((uint32)(byte)(ch1) << 8) |   \
                ((uint32)(byte)(ch2) << 16) | ((uint32)(byte)(ch3) << 24 ))
#endif //defined(DDS_MAKEFOURCC)

#define DDS_MAGIC 0x20534444 // "DDS "

struct DDS_PIXELFORMAT
{
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwFourCC;
    uint32 dwRGBBitCount;
    uint32 dwRBitMask;
    uint32 dwGBitMask;
    uint32 dwBBitMask;
    uint32 dwABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA

/*
const DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT2 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT4 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };

const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };

const DDS_PIXELFORMAT DDSPF_R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

const DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

// This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
const DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, DDS_MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };


// #define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
// #define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
// #define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
// #define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
// #define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE
// 
// #define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
// #define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
// #define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX
// 
// #define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
// #define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
// #define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
// #define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
// #define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
// #define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ
// 
// #define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
//                                DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
//                                DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )
// 
// #define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME
*/

#define DDSD_CAPS                   0x1         // Required in every .dds file.
#define DDSD_HEIGHT                 0x2         // Required in every .dds file.
#define DDSD_WIDTH                  0x4         // Required in every .dds file.
#define DDSD_PITCH                  0x8         // Required when pitch is provided for an uncompressed texture.
#define DDSD_PIXELFORMAT            0x1000      // Required in every .dds file.
#define DDSD_MIPMAPCOUNT            0x20000     // Required in a mipmapped texture.
#define DDSD_LINEARSIZE             0x80000     // Required when pitch is provided for a compressed texture.
#define DDSD_DEPTH                  0x800000    // Required in a depth texture.

#define DDSCAPS_COMPLEX             0x8         // Optional; must be used on any file that contains more than one surface (a mipmap, a cubic environment map, or mipmapped volume texture).
#define DDSCAPS_MIPMAP              0x400000    // Optional; should be used for a mipmap.
#define DDSCAPS_TEXTURE             0x1000      // Required

#define DDSCAPS2_CUBEMAP            0x200       // Required for a cube map.
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x400       // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x800       // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x1000      // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x2000      // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x4000      // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x8000      // Required when these surfaces are stored in a cube map.
#define DDSCAPS2_VOLUME             0x200000    // Required for a volume texture.

typedef struct
{
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwHeight;
    uint32 dwWidth;
    uint32 dwPitchOrLinearSize;
    uint32 dwDepth; // only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
    uint32 dwMipMapCount;
    uint32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32 dwCaps;
    uint32 dwCaps2;
    uint32 dwCaps3;
    uint32 dwCaps4;
    uint32 dwReserved2;
} DDS_HEADER;

enum DDS_RESOURCE_DIMENSION
{
    DDS_RESOURCE_DIMENSION_TEXTURE1D            = 2,
    DDS_RESOURCE_DIMENSION_TEXTURE2D            = 3,
    DDS_RESOURCE_DIMENSION_TEXTURE3D            = 4,
};

#define DDS_RESOURCE_MISC_TEXTURECUBE 0x4

enum DDS_DXGI_FORMAT
{
    DDS_DXGI_FORMAT_UNKNOWN                      = 0,
    DDS_DXGI_FORMAT_R32G32B32A32_TYPELESS        = 1,
    DDS_DXGI_FORMAT_R32G32B32A32_FLOAT           = 2,
    DDS_DXGI_FORMAT_R32G32B32A32_UINT            = 3,
    DDS_DXGI_FORMAT_R32G32B32A32_SINT            = 4,
    DDS_DXGI_FORMAT_R32G32B32_TYPELESS           = 5,
    DDS_DXGI_FORMAT_R32G32B32_FLOAT              = 6,
    DDS_DXGI_FORMAT_R32G32B32_UINT               = 7,
    DDS_DXGI_FORMAT_R32G32B32_SINT               = 8,
    DDS_DXGI_FORMAT_R16G16B16A16_TYPELESS        = 9,
    DDS_DXGI_FORMAT_R16G16B16A16_FLOAT           = 10,
    DDS_DXGI_FORMAT_R16G16B16A16_UNORM           = 11,
    DDS_DXGI_FORMAT_R16G16B16A16_UINT            = 12,
    DDS_DXGI_FORMAT_R16G16B16A16_SNORM           = 13,
    DDS_DXGI_FORMAT_R16G16B16A16_SINT            = 14,
    DDS_DXGI_FORMAT_R32G32_TYPELESS              = 15,
    DDS_DXGI_FORMAT_R32G32_FLOAT                 = 16,
    DDS_DXGI_FORMAT_R32G32_UINT                  = 17,
    DDS_DXGI_FORMAT_R32G32_SINT                  = 18,
    DDS_DXGI_FORMAT_R32G8X24_TYPELESS            = 19,
    DDS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT         = 20,
    DDS_DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS     = 21,
    DDS_DXGI_FORMAT_X32_TYPELESS_G8X24_UINT      = 22,
    DDS_DXGI_FORMAT_R10G10B10A2_TYPELESS         = 23,
    DDS_DXGI_FORMAT_R10G10B10A2_UNORM            = 24,
    DDS_DXGI_FORMAT_R10G10B10A2_UINT             = 25,
    DDS_DXGI_FORMAT_R11G11B10_FLOAT              = 26,
    DDS_DXGI_FORMAT_R8G8B8A8_TYPELESS            = 27,
    DDS_DXGI_FORMAT_R8G8B8A8_UNORM               = 28,
    DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB          = 29,
    DDS_DXGI_FORMAT_R8G8B8A8_UINT                = 30,
    DDS_DXGI_FORMAT_R8G8B8A8_SNORM               = 31,
    DDS_DXGI_FORMAT_R8G8B8A8_SINT                = 32,
    DDS_DXGI_FORMAT_R16G16_TYPELESS              = 33,
    DDS_DXGI_FORMAT_R16G16_FLOAT                 = 34,
    DDS_DXGI_FORMAT_R16G16_UNORM                 = 35,
    DDS_DXGI_FORMAT_R16G16_UINT                  = 36,
    DDS_DXGI_FORMAT_R16G16_SNORM                 = 37,
    DDS_DXGI_FORMAT_R16G16_SINT                  = 38,
    DDS_DXGI_FORMAT_R32_TYPELESS                 = 39,
    DDS_DXGI_FORMAT_D32_FLOAT                    = 40,
    DDS_DXGI_FORMAT_R32_FLOAT                    = 41,
    DDS_DXGI_FORMAT_R32_UINT                     = 42,
    DDS_DXGI_FORMAT_R32_SINT                     = 43,
    DDS_DXGI_FORMAT_R24G8_TYPELESS               = 44,
    DDS_DXGI_FORMAT_D24_UNORM_S8_UINT            = 45,
    DDS_DXGI_FORMAT_R24_UNORM_X8_TYPELESS        = 46,
    DDS_DXGI_FORMAT_X24_TYPELESS_G8_UINT         = 47,
    DDS_DXGI_FORMAT_R8G8_TYPELESS                = 48,
    DDS_DXGI_FORMAT_R8G8_UNORM                   = 49,
    DDS_DXGI_FORMAT_R8G8_UINT                    = 50,
    DDS_DXGI_FORMAT_R8G8_SNORM                   = 51,
    DDS_DXGI_FORMAT_R8G8_SINT                    = 52,
    DDS_DXGI_FORMAT_R16_TYPELESS                 = 53,
    DDS_DXGI_FORMAT_R16_FLOAT                    = 54,
    DDS_DXGI_FORMAT_D16_UNORM                    = 55,
    DDS_DXGI_FORMAT_R16_UNORM                    = 56,
    DDS_DXGI_FORMAT_R16_UINT                     = 57,
    DDS_DXGI_FORMAT_R16_SNORM                    = 58,
    DDS_DXGI_FORMAT_R16_SINT                     = 59,
    DDS_DXGI_FORMAT_R8_TYPELESS                  = 60,
    DDS_DXGI_FORMAT_R8_UNORM                     = 61,
    DDS_DXGI_FORMAT_R8_UINT                      = 62,
    DDS_DXGI_FORMAT_R8_SNORM                     = 63,
    DDS_DXGI_FORMAT_R8_SINT                      = 64,
    DDS_DXGI_FORMAT_A8_UNORM                     = 65,
    DDS_DXGI_FORMAT_R1_UNORM                     = 66,
    DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP           = 67,
    DDS_DXGI_FORMAT_R8G8_B8G8_UNORM              = 68,
    DDS_DXGI_FORMAT_G8R8_G8B8_UNORM              = 69,
    DDS_DXGI_FORMAT_BC1_TYPELESS                 = 70,
    DDS_DXGI_FORMAT_BC1_UNORM                    = 71,
    DDS_DXGI_FORMAT_BC1_UNORM_SRGB               = 72,
    DDS_DXGI_FORMAT_BC2_TYPELESS                 = 73,
    DDS_DXGI_FORMAT_BC2_UNORM                    = 74,
    DDS_DXGI_FORMAT_BC2_UNORM_SRGB               = 75,
    DDS_DXGI_FORMAT_BC3_TYPELESS                 = 76,
    DDS_DXGI_FORMAT_BC3_UNORM                    = 77,
    DDS_DXGI_FORMAT_BC3_UNORM_SRGB               = 78,
    DDS_DXGI_FORMAT_BC4_TYPELESS                 = 79,
    DDS_DXGI_FORMAT_BC4_UNORM                    = 80,
    DDS_DXGI_FORMAT_BC4_SNORM                    = 81,
    DDS_DXGI_FORMAT_BC5_TYPELESS                 = 82,
    DDS_DXGI_FORMAT_BC5_UNORM                    = 83,
    DDS_DXGI_FORMAT_BC5_SNORM                    = 84,
    DDS_DXGI_FORMAT_B5G6R5_UNORM                 = 85,
    DDS_DXGI_FORMAT_B5G5R5A1_UNORM               = 86,
    DDS_DXGI_FORMAT_B8G8R8A8_UNORM               = 87,
    DDS_DXGI_FORMAT_B8G8R8X8_UNORM               = 88,
    DDS_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM   = 89,
    DDS_DXGI_FORMAT_B8G8R8A8_TYPELESS            = 90,
    DDS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB          = 91,
    DDS_DXGI_FORMAT_B8G8R8X8_TYPELESS            = 92,
    DDS_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB          = 93,
    DDS_DXGI_FORMAT_BC6H_TYPELESS                = 94,
    DDS_DXGI_FORMAT_BC6H_UF16                    = 95,
    DDS_DXGI_FORMAT_BC6H_SF16                    = 96,
    DDS_DXGI_FORMAT_BC7_TYPELESS                 = 97,
    DDS_DXGI_FORMAT_BC7_UNORM                    = 98,
    DDS_DXGI_FORMAT_BC7_UNORM_SRGB               = 99,
    DDS_DXGI_FORMAT_COUNT,
    DDS_DXGI_FORMAT_FORCE_UINT                   = 0xffffffffUL 
};

typedef struct
{
    DDS_DXGI_FORMAT dxgiFormat;
    DDS_RESOURCE_DIMENSION resourceDimension;
    uint32 miscFlag;
    uint32 arraySize;
    uint32 reserved;
} DDS_HEADER_DXT10;

#pragma pack(pop)
// end DDS.h
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
