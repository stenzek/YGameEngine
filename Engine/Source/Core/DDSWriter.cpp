#include "Core/PrecompiledHeader.h"
#include "Core/DDSWriter.h"
#include "Core/DDSFormat.h"
#include "Core/PixelFormat.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
Log_SetChannel(DDSWriter);

DDSWriter::DDSWriter()
{
    m_pOutputStream = NULL;
    m_pfFormat = PIXEL_FORMAT_UNKNOWN;
    m_uWidth = m_uHeight = m_uDepth = 0;
    m_uArraySize = m_uMipCount = 0;
    m_uCurrentMipLevel = 0;
    m_uCurrentWidth = m_uCurrentHeight = m_uCurrentDepth = 0;
}

DDSWriter::~DDSWriter()
{
    
}

void DDSWriter::Initialize(ByteStream *pOutputStream)
{
    m_pOutputStream = pOutputStream;
    m_eType = DDS_TEXTURE_TYPE_COUNT;
    m_pfFormat = PIXEL_FORMAT_UNKNOWN;
    m_uWidth = m_uHeight = m_uDepth = 0;
    m_uArraySize = m_uMipCount = 0;
    m_uCurrentMipLevel = 0;
    m_uCurrentWidth = m_uCurrentHeight = m_uCurrentDepth = 0;
}

bool DDSWriter::WriteHeader(DDS_TEXTURE_TYPE Type, PIXEL_FORMAT Format, uint32 Width, uint32 Height, uint32 DepthOrArraySize, uint32 MipCount)
{
    DebugAssert(Type < DDS_TEXTURE_TYPE_COUNT && Format < PIXEL_FORMAT_COUNT && DepthOrArraySize > 0 && MipCount > 0);
    DebugAssert(m_pfFormat == PIXEL_FORMAT_UNKNOWN);

    // fix dimensions
    DebugAssert(Width > 0);
    if (Type == DDS_TEXTURE_TYPE_1D)
    {
        Height = 0;
    }
    else if (Type == DDS_TEXTURE_TYPE_2D)
    {
        DebugAssert(Height > 0);
    }
    else if (Type == DDS_TEXTURE_TYPE_3D)
    {
        DebugAssert(Height > 0 && DepthOrArraySize > 0);
    }
    else if (Type == DDS_TEXTURE_TYPE_CUBE)
    {
        DebugAssert(Height > 0);
        DebugAssert(DepthOrArraySize == 1);
    }

    DDS_HEADER Header;
    Y_memzero(&Header, sizeof(Header));

    Header.dwSize = sizeof(DDS_HEADER);
    Header.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH;
    if (MipCount > 1)
        Header.dwFlags |= DDSD_MIPMAPCOUNT;
    if (Type != DDS_TEXTURE_TYPE_1D)
        Header.dwFlags |= DDSD_HEIGHT;
    if (Type == DDS_TEXTURE_TYPE_3D)
        Header.dwFlags |= DDSD_DEPTH;

    Header.dwHeight = Width;
    Header.dwWidth = (Type != DDS_TEXTURE_TYPE_1D) ? Height : 0;
    Header.dwDepth = (Type == DDS_TEXTURE_TYPE_3D) ? DepthOrArraySize : 0;
    Header.dwPitchOrLinearSize = 0;

    Header.dwMipMapCount = MipCount;
    Header.dwCaps = DDSCAPS_TEXTURE;
    if (MipCount > 1)
        Header.dwCaps |= DDSCAPS_MIPMAP;
    if (MipCount > 1 || (DepthOrArraySize > 1 || Type == DDS_TEXTURE_TYPE_3D))
        Header.dwCaps |= DDSCAPS_COMPLEX;
    
    // get output pf info
    const PIXEL_FORMAT_INFO *ppfInfo = PixelFormat_GetPixelFormatInfo(Format);
    DebugAssert(ppfInfo != NULL);

    // fill in ddspf
    Header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
    if (ppfInfo->IsBlockCompressed)
    {
        switch (Format)
        {
        case PIXEL_FORMAT_BC1_UNORM:
            Header.ddspf.dwFourCC = DDS_MAKEFOURCC('D', 'X', 'T', '1');
            Header.ddspf.dwFlags = DDS_FOURCC;
            break;
        case PIXEL_FORMAT_BC2_UNORM:
            Header.ddspf.dwFourCC = DDS_MAKEFOURCC('D', 'X', 'T', '3');
            Header.ddspf.dwFlags = DDS_FOURCC;
            break;
        case PIXEL_FORMAT_BC3_UNORM:
            Header.ddspf.dwFourCC = DDS_MAKEFOURCC('D', 'X', 'T', '5');
            Header.ddspf.dwFlags = DDS_FOURCC;
            break;
        }
    }
    else
    {
        Header.ddspf.dwRGBBitCount = ppfInfo->BitsPerPixel;
        Header.ddspf.dwRBitMask = ppfInfo->ColorMaskRed;
        Header.ddspf.dwGBitMask = ppfInfo->ColorMaskGreen;
        Header.ddspf.dwBBitMask = ppfInfo->ColorMaskBlue;
        Header.ddspf.dwABitMask = ppfInfo->ColorMaskAlpha;

        if (ppfInfo->ColorMaskAlpha != 0 && ppfInfo->ColorMaskRed != 0 && ppfInfo->ColorMaskGreen != 0 && ppfInfo->ColorMaskBlue != 0)
            Header.ddspf.dwFlags = DDS_RGBA;
        else if (ppfInfo->ColorMaskAlpha == 0 && ppfInfo->ColorMaskRed != 0 && ppfInfo->ColorMaskGreen != 0 && ppfInfo->ColorMaskBlue != 0)
            Header.ddspf.dwFlags = DDS_RGB;
        else if (ppfInfo->ColorMaskAlpha == 0 && ppfInfo->ColorMaskRed != 0 && ppfInfo->ColorMaskGreen == 0 && ppfInfo->ColorMaskBlue == 0)
            Header.ddspf.dwFlags = DDS_LUMINANCE;
        else if (ppfInfo->ColorMaskAlpha != 0 && ppfInfo->ColorMaskRed == 0 && ppfInfo->ColorMaskGreen == 0 && ppfInfo->ColorMaskBlue == 0)
            Header.ddspf.dwFlags = DDS_ALPHA;
    }

    if (Header.ddspf.dwFlags == 0)
    {
        Log_ErrorPrintf("DDSWriter::WriteHeader: Could not determine ddspf for pixel format %s", ppfInfo->Name);
        return false;
    }

    // write the header
    static const uint32 DDSMagic = DDS_MAGIC;
    if (m_pOutputStream->Write(&DDSMagic, sizeof(DDSMagic)) != sizeof(DDSMagic) ||
        m_pOutputStream->Write(&Header, sizeof(Header)) != sizeof(Header))
    {
        goto WRITEERROR;
    }

    m_eType = Type;
    m_pfFormat = Format;
    m_uWidth = m_uCurrentWidth = Width;
    m_uHeight = m_uCurrentHeight = (Type != DDS_TEXTURE_TYPE_1D) ? Height : 1;
    m_uDepth = m_uCurrentDepth = (Type == DDS_TEXTURE_TYPE_3D) ? DepthOrArraySize : 1;
    m_uArraySize = (Type != DDS_TEXTURE_TYPE_3D) ? DepthOrArraySize : 1;
    m_uMipCount = MipCount;
    m_uCurrentMipLevel = 0;
    return true;

WRITEERROR:
    Log_ErrorPrintf("DDSWriter::WriteHeader: Write error at stream position %u size %u", (uint32)m_pOutputStream->GetPosition(), (uint32)m_pOutputStream->GetSize());
    return false;
}

bool DDSWriter::WriteMipLevel(uint32 MipLevel, const void *pData, uint32 cbData)
{
    DebugAssert(m_pfFormat != PIXEL_FORMAT_UNKNOWN);
    DebugAssert(MipLevel == m_uCurrentMipLevel);
    DebugAssert(m_uCurrentMipLevel < m_uMipCount);

    // calc miplevel size
    uint32 WriteSize = 0;
    if (m_eType == DDS_TEXTURE_TYPE_1D)
        WriteSize = PixelFormat_CalculateRowPitch(m_pfFormat, m_uCurrentWidth) * m_uArraySize;
    else if (m_eType == DDS_TEXTURE_TYPE_2D)
        WriteSize = PixelFormat_CalculateImageSize(m_pfFormat, m_uCurrentWidth, m_uCurrentHeight, 1) * m_uArraySize;
    else if (m_eType == DDS_TEXTURE_TYPE_3D)
        WriteSize = PixelFormat_CalculateImageSize(m_pfFormat, m_uCurrentWidth, m_uCurrentHeight, m_uCurrentDepth);
    else if (m_eType == DDS_TEXTURE_TYPE_CUBE)
        WriteSize = PixelFormat_CalculateImageSize(m_pfFormat, m_uCurrentWidth, m_uCurrentHeight, 1) * 6;

    // should be >=
    if (WriteSize > cbData)
    {
        Log_ErrorPrintf("DDSWriter::WriteMipLevel(%u): Not enough data provided (%u, %u required).", MipLevel, cbData, WriteSize);
        return false;
    }

    // write it
    if (!m_pOutputStream->Write2(pData, WriteSize))
    {
        Log_ErrorPrintf("DDSWriter::WriteHeader: Write error at stream position %u size %u", (uint32)m_pOutputStream->GetPosition(), (uint32)m_pOutputStream->GetSize());
        return false;
    }

    m_uCurrentMipLevel++;
    if (m_uCurrentWidth > 1)
        m_uCurrentWidth /= 2;
    if (m_uCurrentHeight > 1)
        m_uCurrentHeight /= 2;
    if (m_uCurrentDepth > 1)
        m_uCurrentDepth /= 2;

    return true;

}

bool DDSWriter::Finalize()
{
    DebugAssert(m_pfFormat != PIXEL_FORMAT_UNKNOWN);

    if (m_uCurrentMipLevel != m_uMipCount)
        return false;

    return true;
}
