#include "Core/PrecompiledHeader.h"
#include "Core/PixelFormat.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Log.h"
Log_SetChannel(PixelFormatConverters);

// kinda a crappy converter, but basically we just convert each pixel to a R32G32B32A32 pixel, then back to the destination format
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void DecodeR8G8B8A8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j * 4 + 0]) / 255.0f;
            pOutRow[j * 4 + 1] = float(pInBytes[j * 4 + 1]) / 255.0f;
            pOutRow[j * 4 + 2] = float(pInBytes[j * 4 + 2]) / 255.0f;
            pOutRow[j * 4 + 3] = float(pInBytes[j * 4 + 3]) / 255.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR8G8B8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j * 3 + 0]) / 255.0f;
            pOutRow[j * 4 + 1] = float(pInBytes[j * 3 + 1]) / 255.0f;
            pOutRow[j * 4 + 2] = float(pInBytes[j * 3 + 2]) / 255.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeB8G8R8A8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j * 4 + 3]) / 255.0f;
            pOutRow[j * 4 + 1] = float(pInBytes[j * 4 + 2]) / 255.0f;
            pOutRow[j * 4 + 2] = float(pInBytes[j * 4 + 1]) / 255.0f;
            pOutRow[j * 4 + 3] = float(pInBytes[j * 4 + 0]) / 255.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeB8G8R8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j * 3 + 3]) / 255.0f;
            pOutRow[j * 4 + 1] = float(pInBytes[j * 3 + 2]) / 255.0f;
            pOutRow[j * 4 + 2] = float(pInBytes[j * 3 + 1]) / 255.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeB8G8R8X8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j * 4 + 3]) / 255.0f;
            pOutRow[j * 4 + 1] = float(pInBytes[j * 4 + 2]) / 255.0f;
            pOutRow[j * 4 + 2] = float(pInBytes[j * 4 + 1]) / 255.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR8(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = float(pInBytes[j]) / 255.0f;
            pOutRow[j * 4 + 1] = 1.0f;
            pOutRow[j * 4 + 2] = 1.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR32G32B32A32F(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = *(const float *)(&pInBytes[j * 16 + 0]);
            pOutRow[j * 4 + 1] = *(const float *)(&pInBytes[j * 16 + 4]);
            pOutRow[j * 4 + 2] = *(const float *)(&pInBytes[j * 16 + 8]);
            pOutRow[j * 4 + 3] = *(const float *)(&pInBytes[j * 16 + 12]);
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR16G16B16A16F(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = Math::HalfToFloat(*(const uint16 *)(&pInBytes[j * 8 + 0]));
            pOutRow[j * 4 + 1] = Math::HalfToFloat(*(const uint16 *)(&pInBytes[j * 8 + 2]));
            pOutRow[j * 4 + 2] = Math::HalfToFloat(*(const uint16 *)(&pInBytes[j * 8 + 4]));
            pOutRow[j * 4 + 3] = Math::HalfToFloat(*(const uint16 *)(&pInBytes[j * 8 + 6]));
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR32F(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = *(const float *)(&pInBytes[j * 4]);
            pOutRow[j * 4 + 1] = 1.0f;
            pOutRow[j * 4 + 2] = 1.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

static void DecodeR16F(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
{
    const byte *pInBytes = (const byte *)pInPixels;
    float *pOutRow = pOutPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutRow[j * 4 + 0] = Math::HalfToFloat(*(const uint16 *)(&pInBytes[j * 2]));
            pOutRow[j * 4 + 1] = 1.0f;
            pOutRow[j * 4 + 2] = 1.0f;
            pOutRow[j * 4 + 3] = 1.0f;
        }
        pInBytes += SourcePitch;
        pOutRow += Width * 4;
    }
}

// 
// static void Decode8BitChannel(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat)
// {
//     PIXEL_FORMAT_INFO *pFormatInfo = PixelFormat_GetPixelFormatInfo(SourceFormat);
//     DebugAssert(pFormatInfo->BitsPerPixel <= 32);
// 
//     const byte *pInBytes = (const byte *)pInPixels;
//     float *pOutRow = pOutPixels;
//     uint32 i, j;
// 
//     uint32 ShiftRed 
// 
//     for (i = 0; i < Height; i++)
//     {
//         const byte *pInThisRow = pInBytes;
//         float *pOutThisRow = pOutRow;
//         for (j = 0; j < Width; j++)
//         {
//             uint32 InInt = 0;
//             if (pFormatInfo->BitsPerPixel >= 8)
//             {
//                 InInt |= (((uint32)*pInThisRow++) << 24);
//                 if (pFormatInfo->BitsPerPixel >= 16)
//                 {
//                     InInt |= (((uint32)*pInThisRow++) << 16);
//                     if (pFormatInfo->BitsPerPixel >= 24)
//                     {
//                         InInt |= (((uint32)*pInThisRow++) << 8);
//                         if (pFormatInfo->BitsPerPixel >= 32)
//                             InInt |= (((uint32)*pInThisRow++));
//                     }
//                 }
//             }
// 
//             // splat the value across all 4 components, then mask it out
//             if (pFormatInfo->ColorMaskRed != 0)
//                 *pOutThisRow++ = (InInt & pFormatInfo->ColorMaskRed) 
// 
// 
//                 
//             pOutRow[j * 4 + 0] = float(pInBytes[j * 3 + 0]) * 255.0f;
//             pOutRow[j * 4 + 1] = float(pInBytes[j * 3 + 1]) * 255.0f;
//             pOutRow[j * 4 + 2] = float(pInBytes[j * 3 + 2]) * 255.0f;
//             pOutRow[j * 4 + 3] = 1.0f;
//         }
//         pInBytes += SourcePitch;
//         pOutRow += Width * 4;
//     }
// }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void EncodeR8G8B8A8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j * 4 + 0] = (byte)(pInRow[j * 4 + 0] * 255.0f);
            pOutBytes[j * 4 + 1] = (byte)(pInRow[j * 4 + 1] * 255.0f);
            pOutBytes[j * 4 + 2] = (byte)(pInRow[j * 4 + 2] * 255.0f);
            pOutBytes[j * 4 + 3] = (byte)(pInRow[j * 4 + 3] * 255.0f);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR8G8B8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j * 3 + 0] = (byte)(pInRow[j * 4 + 0] * 255.0f);
            pOutBytes[j * 3 + 1] = (byte)(pInRow[j * 4 + 1] * 255.0f);
            pOutBytes[j * 3 + 2] = (byte)(pInRow[j * 4 + 2] * 255.0f);
            // drop alpha
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeB8G8R8A8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j * 4 + 2] = (byte)(pInRow[j * 4 + 0] * 255.0f);
            pOutBytes[j * 4 + 1] = (byte)(pInRow[j * 4 + 1] * 255.0f);
            pOutBytes[j * 4 + 0] = (byte)(pInRow[j * 4 + 2] * 255.0f);
            pOutBytes[j * 4 + 3] = (byte)(pInRow[j * 4 + 3] * 255.0f);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeB8G8R8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j * 3 + 2] = (byte)(pInRow[j * 4 + 0] * 255.0f);
            pOutBytes[j * 3 + 1] = (byte)(pInRow[j * 4 + 1] * 255.0f);
            pOutBytes[j * 3 + 0] = (byte)(pInRow[j * 4 + 2] * 255.0f);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeB8G8R8X8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j * 4 + 2] = (byte)(pInRow[j * 4 + 0] * 255.0f);
            pOutBytes[j * 4 + 1] = (byte)(pInRow[j * 4 + 1] * 255.0f);
            pOutBytes[j * 4 + 0] = (byte)(pInRow[j * 4 + 2] * 255.0f);
            pOutBytes[j * 4 + 3] = 0;
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR8(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            pOutBytes[j] = (byte)(pInRow[j * 4 + 0] * 255.0f);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR32G32B32A32F(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            *(float *)(&pOutBytes[j * 16 + 0]) = pInRow[j * 4 + 0];
            *(float *)(&pOutBytes[j * 16 + 4]) = pInRow[j * 4 + 1];
            *(float *)(&pOutBytes[j * 16 + 8]) = pInRow[j * 4 + 2];
            *(float *)(&pOutBytes[j * 16 + 12]) = pInRow[j * 4 + 3];
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR16G16B16A16F(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            *(uint16 *)(&pOutBytes[j * 8 + 0]) = Math::FloatToHalf(pInRow[j * 4 + 0]);
            *(uint16 *)(&pOutBytes[j * 8 + 2]) = Math::FloatToHalf(pInRow[j * 4 + 1]);
            *(uint16 *)(&pOutBytes[j * 8 + 4]) = Math::FloatToHalf(pInRow[j * 4 + 2]);
            *(uint16 *)(&pOutBytes[j * 8 + 6]) = Math::FloatToHalf(pInRow[j * 4 + 3]);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR32F(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            *(float *)(&pOutBytes[j * 4]) = pInRow[j * 4 + 0];
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

static void EncodeR16F(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
    byte *pOutBytes = (byte *)pOutPixels;
    const float *pInRow = pInPixels;
    uint32 i, j;

    for (i = 0; i < Height; i++)
    {
        for (j = 0; j < Width; j++)
        {
            *(uint16 *)(&pOutBytes[j * 2]) = Math::FloatToHalf(pInRow[j * 4 + 0]);
        }
        pOutBytes += DestinationPitch;
        pInRow += Width * 4;
    }
}

// static void EncodeGetPixelRGBA8(const float *pInPixels, uint32 Width, uint32 Height, uint32 x, uint32 y, uint32 *Out)
// {
//     byte *pByteOut = (byte *)Out;
//     if (x >= Width || y >= Height)
//     {
//         pByteOut[0] = 0;
//         pByteOut[1] = 0;
//         pByteOut[2] = 0;
//         pByteOut[3] = 255;
//     }
//     else
//     {
//         const float *pPixel = &pInPixels[y * Width + x];
//         pByteOut[0] = (byte)(pPixel[0] * 255.0f);
//         pByteOut[1] = (byte)(pPixel[1] * 255.0f);
//         pByteOut[2] = (byte)(pPixel[2] * 255.0f);
//         pByteOut[3] = (byte)(pPixel[3] * 255.0f);
//     }
// }

#ifdef HAVE_SQUISH

#include <squish.h>

static void EncodeBC123(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat)
{
//    static const float *ZeroPixel = { 0, 0, 0, 0 };
//#define GETPIXEL(row, col) ((row < Width && col < Height) ? ((float *)(((byte *)pInPixels) + ((row * Width) + col) * DestinationPitch)) : ZeroPixel;
    //static const uint32 *ZeroPixel = { 0, 0, 0, 0xFF };

    const PIXEL_FORMAT_INFO *pFormatInfo = PixelFormat_GetPixelFormatInfo(DestinationFormat);
    uint32 BlocksWide = Max((uint32)1, Width / pFormatInfo->BlockSize);
    uint32 BlocksHigh = Max((uint32)1, Height / pFormatInfo->BlockSize);
    byte *pOutBytes = (byte *)pOutPixels;

    uint32 BlockIn[16];

    //uint32 Flags = squish::kColourRangeFit;
    uint32 Flags = squish::kColourIterativeClusterFit;
    if (DestinationFormat == PIXEL_FORMAT_BC1_UNORM)
        Flags |= squish::kDxt1;
    else if (DestinationFormat == PIXEL_FORMAT_BC2_UNORM)
        Flags |= squish::kDxt3;
    else if (DestinationFormat == PIXEL_FORMAT_BC3_UNORM)
        Flags |= squish::kDxt5;

    for (uint32 i = 0; i < BlocksHigh; i++)
    {
        for (uint32 j = 0; j < BlocksWide; j++)
        {
            uint32 mask = 0;
            uint32 maskPos = 0;

            for (uint32 y = 0; y < 4; y++)
            {
                for (uint32 x = 0; x < 4; x++)
                {
                    uint32 rx = j * pFormatInfo->BlockSize + x;
                    uint32 ry = i * pFormatInfo->BlockSize + y;

                    if (rx < Width && ry < Height)
                    {
                        const float *pSrcPixel = &pInPixels[(ry * Width + rx) * 4];
                        byte *pDstPixel = (byte *)&BlockIn[y * 4 + x];
                        pDstPixel[0] = Min((byte)255, (byte)(pSrcPixel[0] * 255.0f));
                        pDstPixel[1] = Min((byte)255, (byte)(pSrcPixel[1] * 255.0f));
                        pDstPixel[2] = Min((byte)255, (byte)(pSrcPixel[2] * 255.0f));
                        pDstPixel[3] = Min((byte)255, (byte)(pSrcPixel[3] * 255.0f));

                        mask |= 1 << maskPos;
                    }

                    maskPos++;
                }
            }

            byte *BlockOut = pOutBytes + (j * pFormatInfo->BytesPerBlock);
            //squish::Compress((const squish::u8 *)BlockIn, (squish::u8 *)BlockOut, Flags);
            squish::CompressMasked((const squish::u8 *)BlockIn, mask, (squish::u8 *)BlockOut, Flags);
        }
        pOutBytes += DestinationPitch;
    }
}

static void DecodeBC123(const void *pInPixels, float *pOutPixels, uint32 width, uint32 height, uint32 sourcePitch, PIXEL_FORMAT sourceFormat)
{
    const PIXEL_FORMAT_INFO *pFormatInfo = PixelFormat_GetPixelFormatInfo(sourceFormat);
    uint32 blockSize = pFormatInfo->BlockSize;
    uint32 blocksWide = Max((uint32)1, width / blockSize);
    uint32 blocksHigh = Max((uint32)1, height / blockSize);

    uint32 flags = 0;
    if (sourceFormat == PIXEL_FORMAT_BC1_UNORM)
        flags |= squish::kDxt1;
    else if (sourceFormat == PIXEL_FORMAT_BC2_UNORM)
        flags |= squish::kDxt3;
    else if (sourceFormat == PIXEL_FORMAT_BC3_UNORM)
        flags |= squish::kDxt5;

    // decode each block
    for (uint32 by = 0; by < blocksHigh; by++)
    {
        const byte *pSourcePointer = reinterpret_cast<const byte *>(pInPixels) + (sourcePitch * by);

        for (uint32 bx = 0; bx < blocksWide; bx++)
        {
            byte blockRGBA[16 * 4];
            squish::Decompress(blockRGBA, pSourcePointer, flags);

            uint32 startX = bx * blockSize;
            uint32 startY = by * blockSize;
            uint32 endX = startX + blockSize;
            uint32 endY = startY + blockSize;
            const byte *pBlockPtr = blockRGBA;
            for (uint32 y = startY; y < endY; y++)
            {
                if (y >= height)
                    break;

                const byte *pNextBlockPtr = pBlockPtr + blockSize * 4;
                float *pDstPixel = &pOutPixels[(y * width + startX) * 4];

                for (uint32 x = startX; x < endX; x++)
                {
                    if (x >= width)
                        break;

                    *(pDstPixel++) = (float)*(pBlockPtr++) * 255.0f;
                    *(pDstPixel++) = (float)*(pBlockPtr++) * 255.0f;
                    *(pDstPixel++) = (float)*(pBlockPtr++) * 255.0f;
                    *(pDstPixel++) = (float)*(pBlockPtr++) * 255.0f;
                }

                DebugAssert(pNextBlockPtr >= pBlockPtr);
                pBlockPtr = pNextBlockPtr;
            }

            pSourcePointer += pFormatInfo->BytesPerBlock;
        }
    }
}

#endif      // HAVE_SQUISH

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PixelFormatEncodeDecode
{
    typedef void(*EncodeFunctionType)(const float *pInPixels, void *pOutPixels, uint32 Width, uint32 Height, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat);
    typedef void(*DecodeFunctionType)(const void *pInPixels, float *pOutPixels, uint32 Width, uint32 Height, uint32 SourcePitch, PIXEL_FORMAT SourceFormat);
    
    PIXEL_FORMAT Format;
    EncodeFunctionType EncodeFunction;
    DecodeFunctionType DecodeFunction;
};

static const PixelFormatEncodeDecode g_PixelFormatEncodeDecode[] =
{
    { PIXEL_FORMAT_R8G8B8A8_UNORM,          EncodeR8G8B8A8,         DecodeR8G8B8A8      },
    { PIXEL_FORMAT_R8G8B8_UNORM,            EncodeR8G8B8,           DecodeR8G8B8        },
    { PIXEL_FORMAT_B8G8R8A8_UNORM,          EncodeB8G8R8A8,         DecodeB8G8R8A8      },
    { PIXEL_FORMAT_B8G8R8X8_UNORM,          EncodeB8G8R8X8,         DecodeB8G8R8X8      },
    { PIXEL_FORMAT_B8G8R8_UNORM,            EncodeB8G8R8,           DecodeB8G8R8        },
    { PIXEL_FORMAT_R8_UNORM,                EncodeR8,               DecodeR8            },
    { PIXEL_FORMAT_R32G32B32A32_FLOAT,      EncodeR32G32B32A32F,    DecodeR32G32B32A32F },
    { PIXEL_FORMAT_R16G16B16A16_FLOAT,      EncodeR16G16B16A16F,    DecodeR16G16B16A16F },
    { PIXEL_FORMAT_R32_FLOAT,               EncodeR32F,             DecodeR32F          },
    { PIXEL_FORMAT_R16_FLOAT,               EncodeR16F,             DecodeR16F          },
#ifdef HAVE_SQUISH
    { PIXEL_FORMAT_BC1_UNORM,               EncodeBC123,            DecodeBC123         },
    { PIXEL_FORMAT_BC2_UNORM,               EncodeBC123,            DecodeBC123         },
    { PIXEL_FORMAT_BC3_UNORM,               EncodeBC123,            DecodeBC123         },
#endif
};

bool PixelFormat_ConvertPixels(uint32 Width, uint32 Height, const void *SourcePixels, uint32 SourcePitch, PIXEL_FORMAT SourceFormat, void *DestinationPixels, uint32 DestinationPitch, PIXEL_FORMAT DestinationFormat, uint32 *DestinationPixelSize)
{
    uint32 i;

    DebugAssert(SourceFormat < PIXEL_FORMAT_COUNT && DestinationFormat < PIXEL_FORMAT_COUNT);
    DebugAssert(SourceFormat != DestinationFormat);

    //Log_DevPrintf("PixelFormat_ConvertPixels: Converting %ux%u image from %s to %s...", Width, Height, PixelFormat_GetPixelFormatInfo(SourceFormat)->Name, PixelFormat_GetPixelFormatInfo(DestinationFormat)->Name);

    PixelFormatEncodeDecode::DecodeFunctionType DecodeFunction = NULL;
    PixelFormatEncodeDecode::EncodeFunctionType EncodeFunction = NULL;

    for (i = 0; i < countof(g_PixelFormatEncodeDecode); i++)
    {
        if (g_PixelFormatEncodeDecode[i].Format == SourceFormat)
            DecodeFunction = g_PixelFormatEncodeDecode[i].DecodeFunction;
        if (g_PixelFormatEncodeDecode[i].Format == DestinationFormat)
            EncodeFunction = g_PixelFormatEncodeDecode[i].EncodeFunction;
    }

    if (DecodeFunction == NULL)
    {
        Log_ErrorPrintf("PixelFormat_ConvertPixels: No Decode function for %s.", PixelFormat_GetPixelFormatInfo(SourceFormat)->Name);
        return false;
    }

    if (EncodeFunction == NULL)
    {
        Log_ErrorPrintf("PixelFormat_ConvertPixels: No Encode function for %s.", PixelFormat_GetPixelFormatInfo(DestinationFormat)->Name);
        return false;
    }

    uint32 CalculatedDestinationPixelSize = PixelFormat_CalculateImageSize(DestinationFormat, Width, Height, 1);
    if (*DestinationPixelSize < CalculatedDestinationPixelSize)
    {
        Log_ErrorPrintf("PixelFormat_ConvertPixels: DestinationPixelSize too small (%u), %u required.", *DestinationPixelSize, CalculatedDestinationPixelSize);
        return false;
    }

    // allocate memory for temp pixels (ouch)
    //Log_DevPrintf("PixelFormat_ConvertPixels: Temporary array consumes %u bytes of memory...", sizeof(float) * Width * Height * 4);
    float *pTempPixels = Y_mallocT<float>(Width * Height * 4);

    // decode pixels to 32f
    DecodeFunction(SourcePixels, pTempPixels, Width, Height, SourcePitch, SourceFormat);

    // now encode them to the dest format
    EncodeFunction(pTempPixels, DestinationPixels, Width, Height, DestinationPitch, DestinationFormat);

    // log/free/return
    Y_free(pTempPixels);
    *DestinationPixelSize = CalculatedDestinationPixelSize;
    return true;
}

