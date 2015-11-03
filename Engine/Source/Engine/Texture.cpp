#include "Engine/PrecompiledHeader.h"
#include "Engine/Texture.h"
#include "Engine/DataFormats.h"
#include "Renderer/Renderer.h"
#include "Core/Image.h"
Log_SetChannel(Texture);

Y_Define_NameTable(NameTables::TextureType)
    Y_NameTable_Entry("1D",          TEXTURE_TYPE_1D)
    Y_NameTable_Entry("2D",          TEXTURE_TYPE_2D)
    Y_NameTable_Entry("3D",          TEXTURE_TYPE_3D)
    Y_NameTable_Entry("Cube",        TEXTURE_TYPE_CUBE)
    Y_NameTable_Entry("1DArray",     TEXTURE_TYPE_1D_ARRAY)
    Y_NameTable_Entry("2DArray",     TEXTURE_TYPE_2D_ARRAY)
    Y_NameTable_Entry("CubeArray",   TEXTURE_TYPE_CUBE_ARRAY)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TextureClassNames)
    Y_NameTable_Entry("Texture1D", TEXTURE_TYPE_1D)
    Y_NameTable_Entry("Texture2D", TEXTURE_TYPE_2D)
    Y_NameTable_Entry("Texture3D", TEXTURE_TYPE_3D)
    Y_NameTable_Entry("TextureCube", TEXTURE_TYPE_CUBE)
    Y_NameTable_Entry("Texture1DArray", TEXTURE_TYPE_1D_ARRAY)
    Y_NameTable_Entry("Texture2DArray", TEXTURE_TYPE_2D_ARRAY)
    Y_NameTable_Entry("TextureCubeArray", TEXTURE_TYPE_CUBE_ARRAY)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TextureUsage)
    Y_NameTable_Entry("None", TEXTURE_USAGE_NONE)
    Y_NameTable_Entry("ColorMap", TEXTURE_USAGE_COLOR_MAP)
    Y_NameTable_Entry("GlossMap", TEXTURE_USAGE_GLOSS_MAP)
    Y_NameTable_Entry("AlphaMap", TEXTURE_USAGE_ALPHA_MAP)
    Y_NameTable_Entry("NormalMap", TEXTURE_USAGE_NORMAL_MAP)
    Y_NameTable_Entry("HeightMap", TEXTURE_USAGE_HEIGHT_MAP)
    Y_NameTable_Entry("UIAsset", TEXTURE_USAGE_UI_ASSET)
    Y_NameTable_Entry("UILuminanceAsset", TEXTURE_USAGE_UI_LUMINANCE_ASSET)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TextureFilter)
    Y_NameTable_Entry("MinMagMipPoint", TEXTURE_FILTER_MIN_MAG_MIP_POINT)
    Y_NameTable_Entry("MinMagPointMipLinear", TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR)
    Y_NameTable_Entry("MinPointMagLinearMipPoint", TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT)
    Y_NameTable_Entry("MinPointMagMipLinear", TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR)
    Y_NameTable_Entry("MinLinearMagMipPoint", TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT)
    Y_NameTable_Entry("MinLinearMagPointMipLinear", TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
    Y_NameTable_Entry("MinMagLinearMipPoint", TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT)
    Y_NameTable_Entry("MinMagMipLinear", TEXTURE_FILTER_MIN_MAG_MIP_LINEAR)
    Y_NameTable_Entry("Anisotropic", TEXTURE_FILTER_ANISOTROPIC)
    Y_NameTable_Entry("ComparisonMinMagMipPoint", TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT)
    Y_NameTable_Entry("ComparisonMinMagPointMipLinear", TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR)
    Y_NameTable_Entry("ComparisonMinPointMagLinearMipPoint", TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT)
    Y_NameTable_Entry("ComparisonMinPointMagMipLinear", TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR)
    Y_NameTable_Entry("ComparisonMinLinearMagMipPoint", TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT)
    Y_NameTable_Entry("ComparisonMinLinearMagPointMipLinear", TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
    Y_NameTable_Entry("ComparisonMinMagLinearMipPoint", TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
    Y_NameTable_Entry("ComparisonMinMagMipLinear", TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR)
    Y_NameTable_Entry("ComparisonAnisotropic", TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TextureAddressMode)
    Y_NameTable_Entry("Wrap", TEXTURE_ADDRESS_MODE_WRAP)
    Y_NameTable_Entry("Mirror", TEXTURE_ADDRESS_MODE_MIRROR)
    Y_NameTable_Entry("Clamp", TEXTURE_ADDRESS_MODE_CLAMP)
    Y_NameTable_Entry("Border", TEXTURE_ADDRESS_MODE_BORDER)
    Y_NameTable_Entry("MirrorOnce", TEXTURE_ADDRESS_MODE_MIRROR_ONCE)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TexturePlatform)
    Y_NameTable_Entry("DXTC", TEXTURE_PLATFORM_DXTC)
    Y_NameTable_Entry("PVRTC", TEXTURE_PLATFORM_PVRTC)
    Y_NameTable_Entry("ATC", TEXTURE_PLATFORM_ATC)
    Y_NameTable_Entry("ETC", TEXTURE_PLATFORM_ETC)
    Y_NameTable_Entry("ES2_NOTC", TEXTURE_PLATFORM_ES2_NOTC)
    Y_NameTable_Entry("ES2_DXTC", TEXTURE_PLATFORM_ES2_DXTC)
Y_NameTable_End()

Y_Define_NameTable(NameTables::TexturePlatformFileExtension)
    Y_NameTable_Entry("dxtc", TEXTURE_PLATFORM_DXTC)
    Y_NameTable_Entry("pvrtc", TEXTURE_PLATFORM_PVRTC)
    Y_NameTable_Entry("atc", TEXTURE_PLATFORM_ATC)
    Y_NameTable_Entry("etc", TEXTURE_PLATFORM_ETC)
    Y_NameTable_Entry("es2notc", TEXTURE_PLATFORM_ES2_NOTC)
    Y_NameTable_Entry("es2dxtc", TEXTURE_PLATFORM_ES2_DXTC)
Y_NameTable_End()

DEFINE_RESOURCE_TYPE_INFO(Texture);

Texture::Texture(TEXTURE_TYPE TextureType, const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pResourceTypeInfo),
      m_eTextureType(TextureType),
      m_eTexturePlatform(NUM_TEXTURE_PLATFORMS),
      m_ePixelFormat(PIXEL_FORMAT_UNKNOWN),
      m_eTextureUsage(TEXTURE_USAGE_NONE),
      m_eTextureFilter(TEXTURE_FILTER_COUNT),
      m_eBlendingMode(MATERIAL_BLENDING_MODE_COUNT),
      m_iMinLOD(Y_INT32_MIN),
      m_iMaxLOD(Y_INT32_MAX),
      m_nMipLevels(0),
      m_nImages(0),
      m_pImages(NULL),
      m_pDeviceTexture(NULL),
      m_bDeviceResourcesCreated(false)
{
    
}

Texture::~Texture()
{
    uint32 i;

    if (m_pDeviceTexture != NULL)
        m_pDeviceTexture->Release();

    if (m_pImages != NULL)
    {
        for (i = 0; i < m_nImages; i++)
            delete[] m_pImages[i].pPixels;

        delete[] m_pImages;
    }
}

GPUTexture *Texture::GetGPUTexture() const
{
    if (!m_bDeviceResourcesCreated)
        CreateDeviceResources();

    return m_pDeviceTexture;
}

bool Texture::CreateDeviceResources() const
{
    m_bDeviceResourcesCreated = true;
    return true;
}

void Texture::ReleaseDeviceResources() const
{
    m_bDeviceResourcesCreated = false;
}

TEXTURE_TYPE Texture::GetTextureTypeForStream(const char *FileName, ByteStream *pStream)
{
    // in lack of a peek, we have to save the offset and re-seek it
    uint64 currentOffset = pStream->GetPosition();

    // read header
    DF_TEXTURE_HEADER textureHeader;
    if (!pStream->Read2(&textureHeader, sizeof(textureHeader)) || 
        textureHeader.Magic != DF_TEXTURE_HEADER_MAGIC ||
        textureHeader.HeaderSize < sizeof(textureHeader))
    {
        pStream->SeekAbsolute(currentOffset);
        return TEXTURE_TYPE_COUNT;
    }

    pStream->SeekAbsolute(currentOffset);
    if (textureHeader.TextureType < TEXTURE_TYPE_COUNT)
        return (TEXTURE_TYPE)textureHeader.TextureType;
    else
        return TEXTURE_TYPE_COUNT;
}

Texture *Texture::CreateTextureObjectForType(TEXTURE_TYPE textureType)
{
    switch (textureType)
    {
    case TEXTURE_TYPE_2D:
        return new Texture2D();

    case TEXTURE_TYPE_2D_ARRAY:
        return new Texture2DArray();

    case TEXTURE_TYPE_CUBE:
        return new TextureCube();
    }

    return NULL;
}

const ResourceTypeInfo *Texture::GetResourceTypeInfoForTextureType(TEXTURE_TYPE textureType)
{
    switch (textureType)
    {
    case TEXTURE_TYPE_2D:
        return Texture2D::StaticTypeInfo();

    case TEXTURE_TYPE_2D_ARRAY:
        return Texture2DArray::StaticTypeInfo();

    case TEXTURE_TYPE_CUBE:
        return TextureCube::StaticTypeInfo();
    }

    return NULL;
}

uint3 Texture::GetTextureDimensions(const Texture *pTexture)
{
    DebugAssert(pTexture != NULL);
    switch (pTexture->GetTextureType())
    {
    case TEXTURE_TYPE_2D:
        {
            const Texture2D *pTexture2D = pTexture->Cast<Texture2D>();
            return uint3(pTexture2D->GetWidth(), pTexture2D->GetHeight(), 1);
        }
        break;

    case TEXTURE_TYPE_CUBE:
        {
            const TextureCube *pTextureCube = pTexture->Cast<TextureCube>();
            return uint3(pTextureCube->GetWidth(), pTextureCube->GetHeight(), 1);
        }
        break;
    }

    UnreachableCode();
    return uint3::Zero;
}

DEFINE_RESOURCE_TYPE_INFO(Texture2D);
DEFINE_RESOURCE_GENERIC_FACTORY(Texture2D);

Texture2D::Texture2D(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(TEXTURE_TYPE_2D, pResourceTypeInfo),
      m_eAddressModeU(TEXTURE_ADDRESS_MODE_WRAP),
      m_eAddressModeV(TEXTURE_ADDRESS_MODE_WRAP),
      m_iWidth(0),
      m_iHeight(0)
{

}

Texture2D::~Texture2D()
{

}

bool Texture2D::Create(const char *Name, TEXTURE_PLATFORM texturePlatform, TEXTURE_USAGE textureUsage, TEXTURE_FILTER textureFilter, MATERIAL_BLENDING_MODE blendingMode,
                       TEXTURE_ADDRESS_MODE addressModeU, TEXTURE_ADDRESS_MODE addressModeV, int32 minLOD, int32 maxLOD, 
                       PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 mipLevels)
{
    DebugAssert(mipLevels > 0 && mipLevels < TEXTURE_MAX_MIPMAP_COUNT);
    m_strName = Name;

    m_eTextureUsage = textureUsage;
    m_eTexturePlatform = texturePlatform;
    m_eTextureFilter = textureFilter;
    m_eBlendingMode = blendingMode;
    m_eAddressModeU = addressModeU;
    m_eAddressModeV = addressModeV;
    m_iMinLOD = minLOD;
    m_iMaxLOD = maxLOD;
    m_ePixelFormat = pixelFormat;
    m_nMipLevels = mipLevels;
    m_iWidth = width;
    m_iHeight = height;

    m_nImages = mipLevels;
    m_pImages = new ImageData[m_nImages];

    uint32 imageWidth = width;
    uint32 imageHeight = height;
    for (uint32 i = 0; i < m_nMipLevels; i++)
    {
        ImageData &image = m_pImages[i];
        image.Size = PixelFormat_CalculateImageSize(pixelFormat, width, height, 1);
        image.RowPitch = PixelFormat_CalculateRowPitch(pixelFormat, width);
        image.SlicePitch = image.Size;
        image.FileOffset = 0;
        image.pPixels = new byte[image.Size];

        if (imageWidth > 1)
            imageWidth /= 2;
        if (imageHeight > 1)
            imageHeight /= 2;
    }

    return true;
}

bool Texture2D::Load(const char *name, ByteStream *pStream)
{
    uint32 i;
    uint64 startOffset = pStream->GetPosition();

    // assign name, remove extension
    m_strName = name;

    // read in texture header
    DF_TEXTURE_HEADER textureHeader;
    if (!pStream->Read2(&textureHeader, sizeof(textureHeader)) || 
        textureHeader.Magic != DF_TEXTURE_HEADER_MAGIC ||
        textureHeader.HeaderSize < sizeof(textureHeader))
    {
        return false;
    }

    // check it's the correct type
    if (textureHeader.TextureType != TEXTURE_TYPE_2D || textureHeader.ArraySize > 1)
        return false;

    // check fields
    if (textureHeader.TextureUsage >= TEXTURE_USAGE_COUNT ||
        textureHeader.TexturePlatform >= NUM_TEXTURE_PLATFORMS ||
        textureHeader.TextureFilter >= TEXTURE_FILTER_COUNT ||
        textureHeader.PixelFormat >= PIXEL_FORMAT_COUNT ||
        textureHeader.PixelFormat == PIXEL_FORMAT_UNKNOWN ||
        textureHeader.BlendingMode >= MATERIAL_BLENDING_MODE_COUNT ||
        textureHeader.AddressModeU >= TEXTURE_ADDRESS_MODE_COUNT ||
        textureHeader.AddressModeV >= TEXTURE_ADDRESS_MODE_COUNT)
    {
        return false;
    }

    // check dimensions
    if (textureHeader.Width < 1 || textureHeader.Height < 1 || textureHeader.Depth != 1)
        return false;

    // check image count
    if (textureHeader.ImageCount != (textureHeader.ArraySize * textureHeader.MipLevels))
        return false;

    // ok, bring in the data
    m_eTextureUsage = (TEXTURE_USAGE)textureHeader.TextureUsage;
    m_eTexturePlatform = (TEXTURE_PLATFORM)textureHeader.TexturePlatform;
    m_eTextureFilter = (TEXTURE_FILTER)textureHeader.TextureFilter;
    m_ePixelFormat = (PIXEL_FORMAT)textureHeader.PixelFormat;
    m_eBlendingMode = (MATERIAL_BLENDING_MODE)textureHeader.BlendingMode;
    m_eAddressModeU = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeU;
    m_eAddressModeV = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeV;
    m_iMinLOD = (int32)textureHeader.MinLOD;
    m_iMaxLOD = (int32)textureHeader.MaxLOD;
    m_iWidth = textureHeader.Width;
    m_iHeight = textureHeader.Height;
    m_nMipLevels = textureHeader.MipLevels;
    m_nImages = textureHeader.ImageCount;

    // allocate image data
    m_pImages = new ImageData[m_nImages];
    Y_memzero(m_pImages, sizeof(ImageData) * m_nImages);

    // read in image offsets
    uint32 *imageOffsets = (uint32 *)alloca(sizeof(uint32) * textureHeader.ImageCount);
    if (!pStream->SeekAbsolute(startOffset + (uint64)textureHeader.HeaderSize) || !pStream->Read2(imageOffsets, sizeof(uint32) * textureHeader.ImageCount))
        return false;

    // bring in each mip level (todo streaming)
    for (i = 0; i < m_nImages; i++)
    {
        ImageData &dstImage = m_pImages[i];

        if (!pStream->SeekAbsolute(startOffset + (uint64)imageOffsets[i]))
            return false;

        DF_TEXTURE_IMAGE_HEADER textureImageHeader;
        if (!pStream->Read2(&textureImageHeader, sizeof(textureImageHeader)))
            return false;

        dstImage.FileOffset = imageOffsets[i];
        dstImage.Size = textureImageHeader.Size;
        dstImage.RowPitch = textureImageHeader.RowPitch;
        dstImage.SlicePitch = textureImageHeader.SlicePitch;
        dstImage.pPixels = new byte[dstImage.Size];
        if (!pStream->Read2(dstImage.pPixels, dstImage.Size))
            return false;
    }

    // create on gpu
    if (g_pRenderer != nullptr && !CreateDeviceResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool Texture2D::CreateDeviceResources() const
{
    if (m_bDeviceResourcesCreated)
        return true;

    // create texture objects
    if (m_pDeviceTexture == NULL)
    {
        // allocate temp arrays
        const void **ppImageData = (const void **)alloca(sizeof(const void *) * m_nImages);
        uint32 *pRowPitches = (uint32 *)alloca(sizeof(uint32) * m_nImages);
        uint32 i;

        // fill temp arrays
        for (i = 0; i < m_nImages; i++)
        {
            ppImageData[i] = m_pImages[i].pPixels;
            pRowPitches[i] = m_pImages[i].RowPitch;
        }

        // fill texture desc
        GPU_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = m_iWidth;
        textureDesc.Height = m_iHeight;
        textureDesc.Format = m_ePixelFormat;
        textureDesc.Flags = GPU_TEXTURE_FLAG_SHADER_BINDABLE;
        textureDesc.MipLevels = m_nMipLevels;

        // fill sampler desc
        GPU_SAMPLER_STATE_DESC samplerStateDesc;
        samplerStateDesc.Filter = m_eTextureFilter;
        Renderer::CorrectTextureFilter(&samplerStateDesc);
        samplerStateDesc.AddressU = m_eAddressModeU;
        samplerStateDesc.AddressV = m_eAddressModeV;
        samplerStateDesc.AddressW = TEXTURE_ADDRESS_MODE_CLAMP;
        samplerStateDesc.BorderColor.SetZero();
        samplerStateDesc.LODBias = 0.0f;
        samplerStateDesc.MinLOD = m_iMinLOD;
        samplerStateDesc.MaxLOD = m_iMaxLOD;
        samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_NEVER;

        // create texture
        m_pDeviceTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, ppImageData, pRowPitches);

        // set debug name
#ifdef Y_BUILD_CONFIG_DEBUG
        if (m_pDeviceTexture != NULL)
            m_pDeviceTexture->SetDebugName(m_strName);
#endif
    }

    return BaseClass::CreateDeviceResources();
}

void Texture2D::ReleaseDeviceResources() const
{
    SAFE_RELEASE(m_pDeviceTexture);
    return BaseClass::ReleaseDeviceResources();
}

bool Texture2D::ExportToImage(uint32 mipIndex, Image *pOutputImage) const
{
    DebugAssert(mipIndex < m_nMipLevels);

    uint32 imageWidth = m_iWidth >> mipIndex;
    uint32 imageHeight = m_iHeight >> mipIndex;
    if (imageWidth == 0)
        imageWidth = 1;
    if (imageHeight == 0)
        imageHeight = 1;

    // create image
    pOutputImage->Create(m_ePixelFormat, imageWidth, imageHeight, 1);

    // work out rows
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    uint32 nRows = Max((uint32)1, (pPixelFormatInfo->IsBlockCompressed) ? (m_iHeight / pPixelFormatInfo->BlockSize) : (m_iHeight));

    // copy the data
    const ImageData *pImageData = GetMipLevelData(mipIndex);
    Y_memcpy_stride(pOutputImage->GetData(), pOutputImage->GetDataRowPitch(), pImageData->pPixels, pImageData->RowPitch, pOutputImage->GetDataRowPitch(), nRows);

    // done
    return true;
}

DEFINE_RESOURCE_TYPE_INFO(Texture2DArray);
DEFINE_RESOURCE_GENERIC_FACTORY(Texture2DArray);

Texture2DArray::Texture2DArray(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(TEXTURE_TYPE_2D_ARRAY, pResourceTypeInfo),
      m_eAddressModeU(TEXTURE_ADDRESS_MODE_WRAP),
      m_eAddressModeV(TEXTURE_ADDRESS_MODE_WRAP),
      m_iWidth(0),
      m_iHeight(0)
{

}

Texture2DArray::~Texture2DArray()
{

}

bool Texture2DArray::Create(const char *Name, TEXTURE_PLATFORM texturePlatform, TEXTURE_USAGE textureUsage, TEXTURE_FILTER textureFilter, MATERIAL_BLENDING_MODE blendingMode,
                            TEXTURE_ADDRESS_MODE addressModeU, TEXTURE_ADDRESS_MODE addressModeV, int32 minLOD, int32 maxLOD, 
                            PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 arraySize, uint32 mipLevels)
{
    DebugAssert(mipLevels > 0 && mipLevels < TEXTURE_MAX_MIPMAP_COUNT);
    m_strName = Name;

    m_eTextureUsage = textureUsage;
    m_eTexturePlatform = texturePlatform;
    m_eTextureFilter = textureFilter;
    m_eBlendingMode = blendingMode;
    m_eAddressModeU = addressModeU;
    m_eAddressModeV = addressModeV;
    m_iMinLOD = minLOD;
    m_iMaxLOD = maxLOD;
    m_ePixelFormat = pixelFormat;
    m_nMipLevels = mipLevels;
    m_iWidth = width;
    m_iHeight = height;

    m_nImages = arraySize * mipLevels;
    m_pImages = new ImageData[m_nImages];

    uint32 imageWidth = width;
    uint32 imageHeight = height;
    for (uint32 i = 0; i < m_nMipLevels; i++)
    {
        for (uint32 j = 0; j < arraySize; j++)
        {
            ImageData &image = m_pImages[i * arraySize + j];
            image.Size = PixelFormat_CalculateImageSize(pixelFormat, width, height, 1);
            image.RowPitch = PixelFormat_CalculateRowPitch(pixelFormat, width);
            image.SlicePitch = image.Size;
            image.FileOffset = 0;
            image.pPixels = new byte[image.Size];
        }

        if (imageWidth > 1)
            imageWidth /= 2;
        if (imageHeight > 1)
            imageHeight /= 2;
    }

    return true;
}

bool Texture2DArray::Load(const char *name, ByteStream *pStream)
{
    uint32 i;
    uint64 startOffset = pStream->GetPosition();

    // assign name, remove extension
    m_strName = name;

    // read in texture header
    DF_TEXTURE_HEADER textureHeader;
    if (!pStream->Read2(&textureHeader, sizeof(textureHeader)) || 
        textureHeader.Magic != DF_TEXTURE_HEADER_MAGIC ||
        textureHeader.HeaderSize < sizeof(textureHeader))
    {
        return false;
    }

    // check it's the correct type
    if (textureHeader.TextureType != TEXTURE_TYPE_2D_ARRAY || textureHeader.ArraySize == 0)
        return false;

    // check fields
    if (textureHeader.TextureUsage >= TEXTURE_USAGE_COUNT ||
        textureHeader.TexturePlatform >= NUM_TEXTURE_PLATFORMS ||
        textureHeader.TextureFilter > TEXTURE_FILTER_COUNT ||
        textureHeader.AddressModeU >= TEXTURE_ADDRESS_MODE_COUNT ||
        textureHeader.AddressModeV >= TEXTURE_ADDRESS_MODE_COUNT ||
        textureHeader.PixelFormat >= PIXEL_FORMAT_COUNT ||
        textureHeader.PixelFormat == PIXEL_FORMAT_UNKNOWN)
    {
        return false;
    }

    // check dimensions
    if (textureHeader.Width < 1 || textureHeader.Height < 1 || textureHeader.Depth != 1)
        return false;

    // check image count
    if (textureHeader.ImageCount != (textureHeader.ArraySize * textureHeader.MipLevels))
        return false;

    // ok, bring in the data
    m_eTextureUsage = (TEXTURE_USAGE)textureHeader.TextureUsage;
    m_eTexturePlatform = (TEXTURE_PLATFORM)textureHeader.TexturePlatform;
    m_eTextureFilter = (TEXTURE_FILTER)textureHeader.TextureFilter;
    m_ePixelFormat = (PIXEL_FORMAT)textureHeader.PixelFormat;
    m_eAddressModeU = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeU;
    m_eAddressModeV = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeV;
    m_iMinLOD = (int32)textureHeader.MinLOD;
    m_iMaxLOD = (int32)textureHeader.MaxLOD;
    m_iWidth = textureHeader.Width;
    m_iHeight = textureHeader.Height;
    m_iArraySize = textureHeader.ArraySize;
    m_nMipLevels = textureHeader.MipLevels;
    m_nImages = textureHeader.ImageCount;

    // allocate image data
    m_pImages = new ImageData[m_nImages];
    Y_memzero(m_pImages, sizeof(ImageData) * m_nImages);

    // read in image offsets
    uint32 *imageOffsets = (uint32 *)alloca(sizeof(uint32) * textureHeader.ImageCount);
    if (!pStream->SeekAbsolute(startOffset + (uint64)textureHeader.HeaderSize) || !pStream->Read2(imageOffsets, sizeof(uint32) * textureHeader.ImageCount))
        return false;

    // bring in each mip level (todo streaming)
    for (i = 0; i < m_nImages; i++)
    {
        ImageData &dstImage = m_pImages[i];

        if (!pStream->SeekAbsolute(startOffset + (uint64)imageOffsets[i]))
            return false;

        DF_TEXTURE_IMAGE_HEADER textureImageHeader;
        if (!pStream->Read2(&textureImageHeader, sizeof(textureImageHeader)))
            return false;

        dstImage.FileOffset = imageOffsets[i];
        dstImage.Size = textureImageHeader.Size;
        dstImage.RowPitch = textureImageHeader.RowPitch;
        dstImage.SlicePitch = textureImageHeader.SlicePitch;
        dstImage.pPixels = new byte[dstImage.Size];
        if (!pStream->Read2(dstImage.pPixels, dstImage.Size))
            return false;
    }

    // create on gpu
    if (g_pRenderer != nullptr && !CreateDeviceResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool Texture2DArray::CreateDeviceResources() const
{
    if (m_bDeviceResourcesCreated)
        return true;

    // create texture objects
    if (m_pDeviceTexture == NULL)
    {
        // allocate temp arrays
        const void **ppImageData = (const void **)alloca(sizeof(const void *) * m_nImages);
        uint32 *pRowPitches = (uint32 *)alloca(sizeof(uint32) * m_nImages);
        uint32 i;

        // fill temp arrays
        for (i = 0; i < m_nImages; i++)
        {
            ppImageData[i] = m_pImages[i].pPixels;
            pRowPitches[i] = m_pImages[i].RowPitch;
        }

        // fill texture desc
        GPU_TEXTURE2DARRAY_DESC textureDesc;
        textureDesc.Width = m_iWidth;
        textureDesc.Height = m_iHeight;
        textureDesc.Format = m_ePixelFormat;
        textureDesc.Flags = GPU_TEXTURE_FLAG_SHADER_BINDABLE;
        textureDesc.MipLevels = m_nMipLevels;
        textureDesc.ArraySize = m_iArraySize;

        // fill sampler desc
        GPU_SAMPLER_STATE_DESC samplerStateDesc;
        samplerStateDesc.Filter = m_eTextureFilter;
        Renderer::CorrectTextureFilter(&samplerStateDesc);
        samplerStateDesc.AddressU = m_eAddressModeU;
        samplerStateDesc.AddressV = m_eAddressModeV;
        samplerStateDesc.AddressW = TEXTURE_ADDRESS_MODE_CLAMP;
        samplerStateDesc.BorderColor.SetZero();
        samplerStateDesc.LODBias = 0.0f;
        samplerStateDesc.MinLOD = m_iMinLOD;
        samplerStateDesc.MaxLOD = m_iMaxLOD;
        samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_NEVER;

        // create texture
        m_pDeviceTexture = g_pRenderer->CreateTexture2DArray(&textureDesc, &samplerStateDesc, ppImageData, pRowPitches);

#ifdef Y_BUILD_CONFIG_DEBUG
        // set debug name
        if (m_pDeviceTexture != NULL)
            m_pDeviceTexture->SetDebugName(m_strName);
#endif
    }

    return BaseClass::CreateDeviceResources();
}

void Texture2DArray::ReleaseDeviceResources() const
{
    SAFE_RELEASE(m_pDeviceTexture);
    return BaseClass::ReleaseDeviceResources();
}

bool Texture2DArray::ExportToImage(uint32 mipIndex, uint32 arrayIndex, Image *pOutputImage) const
{
    DebugAssert(arrayIndex < m_iArraySize && mipIndex < m_nMipLevels);

    uint32 imageWidth = m_iWidth >> mipIndex;
    uint32 imageHeight = m_iHeight >> mipIndex;
    if (imageWidth == 0)
        imageWidth = 1;
    if (imageHeight == 0)
        imageHeight = 1;

    // create image
    pOutputImage->Create(m_ePixelFormat, imageWidth, imageHeight, 1);

    // work out rows
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    uint32 nRows = Max((uint32)1, (pPixelFormatInfo->IsBlockCompressed) ? (m_iHeight / pPixelFormatInfo->BlockSize) : (m_iHeight));

    // copy the data
    const ImageData *pImageData = GetMipLevelData(mipIndex, arrayIndex);
    Y_memcpy_stride(pOutputImage->GetData(), pOutputImage->GetDataRowPitch(), pImageData->pPixels, pImageData->RowPitch, pOutputImage->GetDataRowPitch(), nRows);

    // done
    return true;
}

DEFINE_RESOURCE_TYPE_INFO(TextureCube);
DEFINE_RESOURCE_GENERIC_FACTORY(TextureCube);

TextureCube::TextureCube(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(TEXTURE_TYPE_CUBE, pResourceTypeInfo),
      m_eAddressModeU(TEXTURE_ADDRESS_MODE_WRAP),
      m_eAddressModeV(TEXTURE_ADDRESS_MODE_WRAP),
      m_iWidth(0),
      m_iHeight(0)
{

}

TextureCube::~TextureCube()
{

}

bool TextureCube::Load(const char *name, ByteStream *pStream)
{
    uint32 i;
    uint64 startOffset = pStream->GetPosition();

    // assign name, remove extension
    m_strName = name;

    // read in texture header
    DF_TEXTURE_HEADER textureHeader;
    if (!pStream->Read2(&textureHeader, sizeof(textureHeader)) || 
        textureHeader.Magic != DF_TEXTURE_HEADER_MAGIC ||
        textureHeader.HeaderSize < sizeof(textureHeader))
    {
        return false;
    }

    // check it's the correct type
    if (textureHeader.TextureType != TEXTURE_TYPE_CUBE || textureHeader.ArraySize != CUBEMAP_FACE_COUNT)
        return false;

    // check fields
    if (textureHeader.TextureUsage >= TEXTURE_USAGE_COUNT ||
        textureHeader.TexturePlatform >= NUM_TEXTURE_PLATFORMS ||
        textureHeader.TextureFilter > TEXTURE_FILTER_COUNT ||
        textureHeader.AddressModeU >= TEXTURE_ADDRESS_MODE_COUNT ||
        textureHeader.AddressModeV >= TEXTURE_ADDRESS_MODE_COUNT)
    {
        return false;
    }

    // check dimensions
    if (textureHeader.Width < 1 || textureHeader.Height < 1 || textureHeader.Depth != 1)
        return false;

    // check image count
    if (textureHeader.ImageCount != (textureHeader.ArraySize * textureHeader.MipLevels))
        return false;

    // ok, bring in the data
    m_ePixelFormat = (PIXEL_FORMAT)textureHeader.PixelFormat;
    m_eTexturePlatform = (TEXTURE_PLATFORM)textureHeader.TexturePlatform;
    m_eTextureUsage = (TEXTURE_USAGE)textureHeader.TextureUsage;
    m_eTextureFilter = (TEXTURE_FILTER)textureHeader.TextureFilter;
    m_eAddressModeU = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeU;
    m_eAddressModeV = (TEXTURE_ADDRESS_MODE)textureHeader.AddressModeV;
    m_iMinLOD = textureHeader.MinLOD;
    m_iMaxLOD = textureHeader.MaxLOD;
    m_iWidth = textureHeader.Width;
    m_iHeight = textureHeader.Height;
    m_nMipLevels = textureHeader.MipLevels;
    m_nImages = textureHeader.ImageCount;

    // allocate image data
    m_pImages = new ImageData[m_nImages];
    Y_memzero(m_pImages, sizeof(ImageData) * m_nImages);

    // read in image offsets
    uint32 *imageOffsets = (uint32 *)alloca(sizeof(uint32) * textureHeader.ImageCount);
    if (!pStream->SeekAbsolute(startOffset + (uint64)textureHeader.HeaderSize) || !pStream->Read2(imageOffsets, sizeof(uint32) * textureHeader.ImageCount))
        return false;

    // bring in each mip level (todo streaming)
    for (i = 0; i < m_nImages; i++)
    {
        ImageData &dstImage = m_pImages[i];

        if (!pStream->SeekAbsolute(startOffset + (uint64)imageOffsets[i]))
            return false;

        DF_TEXTURE_IMAGE_HEADER textureImageHeader;
        if (!pStream->Read2(&textureImageHeader, sizeof(textureImageHeader)))
            return false;

        dstImage.FileOffset = imageOffsets[i];
        dstImage.Size = textureImageHeader.Size;
        dstImage.RowPitch = textureImageHeader.RowPitch;
        dstImage.SlicePitch = textureImageHeader.SlicePitch;
        dstImage.pPixels = new byte[dstImage.Size];
        if (!pStream->Read2(dstImage.pPixels, dstImage.Size))
            return false;
    }

    // create on gpu
    if (!CreateDeviceResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool TextureCube::CreateDeviceResources() const
{
    if (m_bDeviceResourcesCreated)
        return true;

    // create texture objects
    if (m_pDeviceTexture == NULL)
    {
        // allocate temp arrays
        DebugAssert(m_nImages > 0 && m_nImages == m_nMipLevels * CUBEMAP_FACE_COUNT);
        const void **ppImageData = (const void **)alloca(sizeof(const void *) * m_nImages);
        uint32 *pRowPitches = (uint32 *)alloca(sizeof(uint32) * m_nImages);
        uint32 i;

        // fill temp arrays
        for (i = 0; i < m_nImages; i++)
        {
            ppImageData[i] = m_pImages[i].pPixels;
            pRowPitches[i] = m_pImages[i].RowPitch;
        }

        // fill desc
        GPU_TEXTURECUBE_DESC textureDesc;
        textureDesc.Width = m_iWidth;
        textureDesc.Height = m_iHeight;
        textureDesc.Format = m_ePixelFormat;
        textureDesc.Flags = GPU_TEXTURE_FLAG_SHADER_BINDABLE;
        textureDesc.MipLevels = m_nMipLevels;

        // fill sampler desc
        GPU_SAMPLER_STATE_DESC samplerStateDesc;
        samplerStateDesc.Filter = m_eTextureFilter;
        Renderer::CorrectTextureFilter(&samplerStateDesc);
        samplerStateDesc.AddressU = m_eAddressModeU;
        samplerStateDesc.AddressV = m_eAddressModeV;
        samplerStateDesc.AddressW = TEXTURE_ADDRESS_MODE_CLAMP;
        samplerStateDesc.BorderColor.SetZero();
        samplerStateDesc.LODBias = 0.0f;
        samplerStateDesc.MinLOD = m_iMinLOD;
        samplerStateDesc.MaxLOD = m_iMaxLOD;
        samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_NEVER;

        // create texture
        m_pDeviceTexture = g_pRenderer->CreateTextureCube(&textureDesc, &samplerStateDesc, ppImageData, pRowPitches);

#ifdef Y_BUILD_CONFIG_DEBUG
        // set debug name
        if (m_pDeviceTexture != NULL)
            m_pDeviceTexture->SetDebugName(m_strName);
#endif
    }

    return BaseClass::CreateDeviceResources();
}

void TextureCube::ReleaseDeviceResources() const
{
    SAFE_RELEASE(m_pDeviceTexture);
    return BaseClass::ReleaseDeviceResources();
}

bool TextureCube::ExportToImage(uint32 cubeFace, uint32 mipIndex, Image *pOutputImage) const
{
    DebugAssert(cubeFace < CUBE_FACE_COUNT && mipIndex < m_nMipLevels);

    uint32 imageWidth = m_iWidth >> mipIndex;
    uint32 imageHeight = m_iHeight >> mipIndex;
    if (imageWidth == 0)
        imageWidth = 1;
    if (imageHeight == 0)
        imageHeight = 1;

    // create image
    pOutputImage->Create(m_ePixelFormat, imageWidth, imageHeight, 1);

    // work out rows
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    uint32 nRows = Max((uint32)1, (pPixelFormatInfo->IsBlockCompressed) ? (m_iHeight / pPixelFormatInfo->BlockSize) : (m_iHeight));

    // copy the data
    const ImageData *pImageData = GetMipLevelData(cubeFace, mipIndex);
    Y_memcpy_stride(pOutputImage->GetData(), pOutputImage->GetDataRowPitch(), pImageData->pPixels, pImageData->RowPitch, pOutputImage->GetDataRowPitch(), nRows);

    // done
    return true;
}
