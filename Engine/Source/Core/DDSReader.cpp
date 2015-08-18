#include "Core/PrecompiledHeader.h"
#include "Core/DDSReader.h"
#include "Core/DDSFormat.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Math.h"
Log_SetChannel(DDSReader);

static const PIXEL_FORMAT DDSDXGIFormatToPixelFormat[DDS_DXGI_FORMAT_COUNT] =
{
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_UNKNOWN
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32A32_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32A32_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32A32_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32A32_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32B32_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16B16A16_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G32_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32G8X24_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_X32_TYPELESS_G8X24_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R10G10B10A2_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R10G10B10A2_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R10G10B10A2_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R11G11B10_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8B8A8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8B8A8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8B8A8_UINT
    PIXEL_FORMAT_R8G8B8A8_SNORM, // DDS_DXGI_FORMAT_R8G8B8A8_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8B8A8_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16G16_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_D32_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R32_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R24G8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_D24_UNORM_S8_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_X24_TYPELESS_G8_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_FLOAT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_D16_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R16_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8_UINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8_SINT
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_A8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R1_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R8G8_B8G8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_G8R8_G8B8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC1_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC1_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC1_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC2_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC2_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC2_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC3_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC3_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC3_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC4_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC4_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC4_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC5_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC5_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC5_SNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B5G6R5_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B5G5R5A1_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8A8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8X8_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8A8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8X8_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC6H_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC6H_UF16
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC6H_SF16
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC7_TYPELESS
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC7_UNORM
    PIXEL_FORMAT_UNKNOWN,        // DDS_DXGI_FORMAT_BC7_UNORM_SRGB
};

DDSReader::DDSReader()
{
    m_pStream = NULL;
    m_eTextureType = DDS_TEXTURE_TYPE_UNKNOWN;
    m_iWidth = m_iHeight = m_iDepth = 0;
    m_nMipLevels = 0;
    m_iArraySize = 0;
    m_ePixelFormat = PIXEL_FORMAT_UNKNOWN;
    m_pMipLevels = NULL;
}

DDSReader::~DDSReader()
{
    Close();
}

DDS_TEXTURE_TYPE DDSReader::GetStreamDDSTextureType(const char *FileName, ByteStream *pStream)
{
    uint32 Magic;
    DDS_HEADER Header;
    DDS_TEXTURE_TYPE TextureType = DDS_TEXTURE_TYPE_UNKNOWN;
    
    // save current offset
    uint64 saveStreamOffset = pStream->GetPosition();

    // read header
    if (!pStream->Read2(&Magic, sizeof(Magic)) || Magic != DDS_MAGIC ||
        !pStream->Read2(&Header, sizeof(Header)) || Header.dwSize < sizeof(DDS_HEADER) ||
        (Header.dwSize > sizeof(DDS_HEADER) && !pStream->SeekRelative(Header.dwSize - sizeof(DDS_HEADER))))
    {
        Log_ErrorPrintf("DDSLoader::GetStreamDDSTextureType: \"%s\" Could not read header, or it is invalid.", FileName);
        goto CLEANUP;
    }

    if ((Header.dwFlags & (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH)) != (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH))
        Log_WarningPrintf("DDSLoader::GetStreamDDSTextureType: \"%s\" Missing some required flags, results may be unpredictible.", FileName);

    if (Header.dwFlags & (DDSD_PITCH | DDSD_LINEARSIZE))
        Log_WarningPrintf("DDSLoader::GetStreamDDSTextureType: \"%s\" Loading images with pitch/linearsize currently unsupported. Image may be corrupted.", FileName);

    // determine texture type
    if (Header.dwFlags & DDSD_HEIGHT)
    {
        if (Header.dwFlags & DDSD_DEPTH)
            TextureType = DDS_TEXTURE_TYPE_3D;
        else if (Header.dwCaps2 & DDSCAPS2_CUBEMAP)
            TextureType = DDS_TEXTURE_TYPE_CUBE;
        else
            TextureType = DDS_TEXTURE_TYPE_2D;
    }
    else
    {
        TextureType = DDS_TEXTURE_TYPE_1D;
    }

    // handle dx10 header
    if (Header.ddspf.dwFlags == DDS_FOURCC && Header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', '1', '0'))
    {
        DDS_HEADER_DXT10 DX10Header;
        if (pStream->Read(&DX10Header, sizeof(DX10Header)) != sizeof(DX10Header))
        {
            Log_ErrorPrintf("DDSLoader::GetStreamDDSTextureType: \"%s\" has fourcc DX10 but no DX10 header.", FileName);
            goto CLEANUP;
        }

        // copy from dx10 header
        if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE1D)
            TextureType = DDS_TEXTURE_TYPE_1D;
        else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE2D && DX10Header.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
            TextureType = DDS_TEXTURE_TYPE_CUBE;
        else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE2D)
            TextureType = DDS_TEXTURE_TYPE_2D;
        else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE3D)
            TextureType = DDS_TEXTURE_TYPE_3D;
        else
        {
            Log_ErrorPrintf("DDSLoader::GetStreamDDSTextureType: \"%s\" invalid resource dimension (%u) in DX10 header.", FileName, DX10Header.resourceDimension);
            goto CLEANUP;
        }
    }

CLEANUP:
    pStream->SeekAbsolute(saveStreamOffset);
    return TextureType;
}

bool DDSReader::Open(const char *FileName, ByteStream *pStream)
{
    uint32 Magic;
    DDS_HEADER Header;
    uint32 nImages;
    uint32 currentFilePosition;
    uint32 fileSize;

    if (!pStream->Read2(&Magic, sizeof(Magic)) || Magic != DDS_MAGIC ||
        !pStream->Read2(&Header, sizeof(Header)) || Header.dwSize < sizeof(DDS_HEADER) ||
        !pStream->SeekAbsolute(sizeof(Magic) + Header.dwSize))
    {
        Log_ErrorPrintf("DDSLoader::Open: \"%s\" Could not read header, or it is invalid.", FileName);
        goto FAILURE;
    }

    if ((Header.dwFlags & (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH)) != (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH))
        Log_WarningPrintf("DDSLoader::Open: \"%s\" Missing some required flags, results may be unpredictible.", FileName);

    if (Header.dwFlags & (DDSD_PITCH | DDSD_LINEARSIZE))
        Log_WarningPrintf("DDSLoader::Open: \"%s\" Loading images with pitch/linearsize currently unsupported. Image may be corrupted.", FileName);

    // determine texture type
    if (Header.dwFlags & DDSD_HEIGHT)
    {
        DebugAssert(!(Header.dwFlags & DDSD_DEPTH));
        if (Header.dwFlags & DDSD_DEPTH)
            m_eTextureType = DDS_TEXTURE_TYPE_3D;
        else if (Header.dwCaps2 & DDSCAPS2_CUBEMAP)
            m_eTextureType = DDS_TEXTURE_TYPE_CUBE;
        else
            m_eTextureType = DDS_TEXTURE_TYPE_2D;
    }
    else
    {
        m_eTextureType = DDS_TEXTURE_TYPE_1D;
    }

    // determine dimensions
    m_iWidth = Header.dwWidth;
    m_iHeight = (m_eTextureType == DDS_TEXTURE_TYPE_1D) ? 1 : Header.dwHeight;
    m_iDepth = (m_eTextureType != DDS_TEXTURE_TYPE_3D) ? 1 : Header.dwDepth;
    m_iArraySize = (m_eTextureType == DDS_TEXTURE_TYPE_CUBE) ? 6 : 1;
    m_nMipLevels = ((Header.dwFlags & DDSD_MIPMAPCOUNT) != 0) ? Header.dwMipMapCount : 1;

    // determine pixel format
    m_ePixelFormat = PIXEL_FORMAT_UNKNOWN;
    if (Header.ddspf.dwFlags == DDS_FOURCC)
    {
        if (Header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '1'))
            m_ePixelFormat = PIXEL_FORMAT_BC1_UNORM;
        else if (Header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '3'))
            m_ePixelFormat = PIXEL_FORMAT_BC2_UNORM;
        else if (Header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', 'T', '5'))
            m_ePixelFormat = PIXEL_FORMAT_BC3_UNORM;
        else if (Header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', '1', '0'))
        {
            DDS_HEADER_DXT10 DX10Header;
            if (pStream->Read(&DX10Header, sizeof(DX10Header)) != sizeof(DX10Header))
            {
                Log_ErrorPrintf("DDSLoader::Open: \"%s\" has fourcc DX10 but no DX10 header.", FileName);
                goto FAILURE;
            }

            // copy from dx10 header
            if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE1D)
                m_eTextureType = DDS_TEXTURE_TYPE_1D;
            else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE2D && DX10Header.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
                m_eTextureType = DDS_TEXTURE_TYPE_CUBE;
            else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE2D)
                m_eTextureType = DDS_TEXTURE_TYPE_2D;
            else if (DX10Header.resourceDimension == DDS_RESOURCE_DIMENSION_TEXTURE3D)
                m_eTextureType = DDS_TEXTURE_TYPE_3D;
            else
            {
                Log_ErrorPrintf("DDSLoader::Open: \"%s\" invalid resource dimension (%u) in DX10 header.", FileName, DX10Header.resourceDimension);
                goto FAILURE;
            }

            m_iArraySize = DX10Header.arraySize;

            if (DX10Header.dxgiFormat >= DDS_DXGI_FORMAT_COUNT || DDSDXGIFormatToPixelFormat[DX10Header.dxgiFormat] == PIXEL_FORMAT_UNKNOWN)
            {
                Log_ErrorPrintf("DDSLoader::Open: \"%s\" invalid dxgi format (%u) in DX10 header.", FileName, DX10Header.dxgiFormat);
                goto FAILURE;
            }

            m_ePixelFormat = DDSDXGIFormatToPixelFormat[DX10Header.dxgiFormat];
        }
    }
    else
    {
        for (uint32 i = 0; i < PIXEL_FORMAT_COUNT; i++)
        {
            const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo((PIXEL_FORMAT)i);
            if (pPixelFormatInfo->BitsPerPixel != Header.ddspf.dwRGBBitCount)
                continue;

            if (pPixelFormatInfo->ColorMaskAlpha != 0 && pPixelFormatInfo->ColorMaskRed != 0 && pPixelFormatInfo->ColorMaskGreen != 0 && pPixelFormatInfo->ColorMaskBlue != 0 &&
                Header.ddspf.dwFlags == DDS_RGBA)
            {
                if (Header.ddspf.dwRBitMask == pPixelFormatInfo->ColorMaskRed &&
                    Header.ddspf.dwGBitMask == pPixelFormatInfo->ColorMaskGreen &&
                    Header.ddspf.dwBBitMask == pPixelFormatInfo->ColorMaskBlue &&
                    Header.ddspf.dwABitMask == pPixelFormatInfo->ColorMaskAlpha)
                {
                    m_ePixelFormat = (PIXEL_FORMAT)i;
                    break;
                }
            }
            else if (pPixelFormatInfo->ColorMaskAlpha == 0 && pPixelFormatInfo->ColorMaskRed != 0 && pPixelFormatInfo->ColorMaskGreen != 0 && pPixelFormatInfo->ColorMaskBlue != 0 &&
                Header.ddspf.dwFlags == DDS_RGB)
            {
                if (Header.ddspf.dwRBitMask == pPixelFormatInfo->ColorMaskRed &&
                    Header.ddspf.dwGBitMask == pPixelFormatInfo->ColorMaskGreen &&
                    Header.ddspf.dwBBitMask == pPixelFormatInfo->ColorMaskBlue)
                {
                    m_ePixelFormat = (PIXEL_FORMAT)i;
                    break;
                }
            }
            else if (pPixelFormatInfo->ColorMaskAlpha == 0 && pPixelFormatInfo->ColorMaskRed != 0 && pPixelFormatInfo->ColorMaskGreen == 0 && pPixelFormatInfo->ColorMaskBlue == 0 &&
                Header.ddspf.dwFlags == DDS_LUMINANCE)
            {
                if (Header.ddspf.dwRBitMask == pPixelFormatInfo->ColorMaskRed)
                {
                    m_ePixelFormat = (PIXEL_FORMAT)i;
                    break;
                }
            }
            else if (pPixelFormatInfo->ColorMaskAlpha != 0 && pPixelFormatInfo->ColorMaskRed == 0 && pPixelFormatInfo->ColorMaskGreen == 0 && pPixelFormatInfo->ColorMaskBlue == 0 &&
                Header.ddspf.dwFlags == DDS_ALPHA)
            {
                if (Header.ddspf.dwABitMask == pPixelFormatInfo->ColorMaskAlpha)
                {
                    m_ePixelFormat = (PIXEL_FORMAT)i;
                    break;
                }
            }
        }
    }

    if (m_ePixelFormat == PIXEL_FORMAT_UNKNOWN)
    {
        Log_ErrorPrintf("DDSLoader::Open: \"%s\" Could not determine pixel format.", FileName);
        goto FAILURE;
    }

    if (!Y_ispow2(m_iWidth) ||
        ((m_eTextureType == DDS_TEXTURE_TYPE_2D || m_eTextureType == DDS_TEXTURE_TYPE_3D || m_eTextureType == DDS_TEXTURE_TYPE_CUBE) && !Y_ispow2(m_iHeight)) ||
        (m_eTextureType == DDS_TEXTURE_TYPE_3D && !Y_ispow2(m_iDepth)))
    {
        Log_ErrorPrintf("DDSLoader::Open: \"%s\" Texture dimensions (%u * %u * %u * %u) failed validation.", FileName, m_iWidth, m_iHeight, m_iDepth, m_iArraySize);
        goto FAILURE;
    }

    // check number of miplevels
    if (m_nMipLevels > 1)
    {
        uint32 mw = m_iWidth;
        uint32 mh = m_iHeight;
        uint32 md = m_iDepth;
        uint32 nCalculatedMips = 0;

        for ( ; ; )
        {
            nCalculatedMips++;

            if (mw == 1 && mh == 1 && md == 1)
                break;

            if (mw > 1)
                mw /= 2;
            if (mh > 1)
                mh /= 2;
            if (md > 1)
                md /= 2;
        }

        if (nCalculatedMips != m_nMipLevels)
        {
            Log_WarningPrintf("DDSLoader::Open: \"%s\" file has an incorrect number of mipmaps (us %u, file %u), disabling all but first.", FileName, nCalculatedMips, m_nMipLevels);
            m_nMipLevels = 1;
        }
    }

    // allocate miplevels
    nImages = m_nMipLevels * m_iArraySize;
    m_pMipLevels = new MipLevelData[nImages];
    Y_memzero(m_pMipLevels, sizeof(MipLevelData) * nImages);

    // fill miplevels
    currentFilePosition = (uint32)pStream->GetPosition();
    fileSize = (uint32)pStream->GetSize();
    for (uint32 i = 0; i < m_iArraySize; i++)
    {
        uint32 currentMipWidth = m_iWidth;
        uint32 currentMipHeight = m_iHeight;
        uint32 currentMipDepth = m_iDepth;

        for (uint32 j = 0; j < m_nMipLevels; j++)
        {
            MipLevelData *pMipLevel = &m_pMipLevels[i * m_nMipLevels + j];
            pMipLevel->Size = PixelFormat_CalculateImageSize(m_ePixelFormat, currentMipWidth, currentMipHeight, 1);
            pMipLevel->Pitch = PixelFormat_CalculateRowPitch(m_ePixelFormat, currentMipWidth);
            pMipLevel->OffsetInFile = currentFilePosition;
            pMipLevel->pData = NULL;

            currentFilePosition += pMipLevel->Size;
            if (currentFilePosition > fileSize)
            {
                Log_WarningPrintf("DDSLoader::Open: \"%s\" image data is larger than file (current offset: %u, file size: %u).", FileName, currentFilePosition, fileSize);
                goto FAILURE;
            }

            if (currentMipWidth > 1)
                currentMipWidth /= 2;
            if (currentMipHeight > 1)
                currentMipHeight /= 2;
            if (currentMipDepth > 1)
                currentMipDepth /= 2;
        }
    }

    // set remaining fields    
    m_strFileName = FileName;
    m_pStream = pStream;
    pStream->AddRef();

    Log_DevPrintf("\"%s\": %u mip levels, %u array textures, mip level 0 is %u x %u x %u", FileName, m_nMipLevels, m_iArraySize, m_iWidth, m_iHeight, m_iDepth);
    return true;

FAILURE:
    Close();
    return false;
}

bool DDSReader::LoadMipLevel(uint32 MipLevel)
{
    uint32 i;
    DebugAssert(MipLevel < m_nMipLevels);

    for (i = 0; i < m_iArraySize; i++)
    {
        MipLevelData *pMipLevel = &m_pMipLevels[i * m_nMipLevels + MipLevel];
        if (pMipLevel->pData == NULL)
        {
            pMipLevel->pData = new byte[pMipLevel->Size];

            if (!m_pStream->SeekAbsolute(pMipLevel->OffsetInFile) || !m_pStream->Read2(pMipLevel->pData, pMipLevel->Size))
            {
                Log_ErrorPrintf("DDSLoader::LoadMipLevel: \"%s\" failed to read arrayindex %u miplevel %u.", m_strFileName.GetCharArray(), i, MipLevel);
                delete[] pMipLevel->pData;
                pMipLevel->pData = NULL;
                return false;
            }
        }
    }

    return true;
}

bool DDSReader::LoadAllMipLevels()
{
    uint32 i, j;

    for (i = 0; i < m_iArraySize; i++)
    {
        for (j = 0; j < m_nMipLevels; j++)
        {
            MipLevelData *pMipLevel = &m_pMipLevels[i * m_nMipLevels + j];
            if (pMipLevel->pData == NULL)
            {
                pMipLevel->pData = new byte[pMipLevel->Size];

                if (!m_pStream->SeekAbsolute(pMipLevel->OffsetInFile) || !m_pStream->Read2(pMipLevel->pData, pMipLevel->Size))
                {
                    Log_ErrorPrintf("DDSLoader::LoadAllMipLevels: \"%s\" failed to read arrayindex %u miplevel %u.", m_strFileName.GetCharArray(), i, j);
                    delete[] pMipLevel->pData;
                    pMipLevel->pData = NULL;
                    return false;
                }
            }
        }
    }

    return true;
}

void DDSReader::Close()
{
    FreeMipLevels();

    m_strFileName.Clear();
    SAFE_RELEASE(m_pStream);

    m_eTextureType = DDS_TEXTURE_TYPE_UNKNOWN;
    m_iWidth = m_iHeight = m_iDepth = 0;
    m_nMipLevels = 0;
    m_iArraySize = 0;
    m_ePixelFormat = PIXEL_FORMAT_UNKNOWN;
}

const DDSReader::MipLevelData *DDSReader::_GetMipLevel(uint32 ArrayIndex, uint32 MipLevel) const
{
    DebugAssert(ArrayIndex < m_iArraySize);
    return &m_pMipLevels[ArrayIndex * m_nMipLevels + MipLevel];
}

void DDSReader::FreeMipLevels()
{
    uint32 i;

    if (m_pMipLevels != NULL)
    {
        for (i = 0; i < m_nMipLevels; i++)
        {
            if (m_pMipLevels[i].pData != NULL)
                delete[] m_pMipLevels[i].pData;
        }

        delete[] m_pMipLevels;
        m_pMipLevels = NULL;
    }
}
