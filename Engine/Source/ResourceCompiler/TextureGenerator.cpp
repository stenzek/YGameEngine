#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/TextureGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/DataFormats.h"
#include "Core/ImageCodec.h"
#include "Core/Image.h"
#include "Core/DDSReader.h"
#include "YBaseLib/ZipArchive.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(TextureGenerator);

static const IMAGE_RESIZE_FILTER DEFAULT_MIPMAP_RESIZE_FILTER = IMAGE_RESIZE_FILTER_LANCZOS3;

const char *TextureGenerator::Properties::Usage = "Usage";
const char *TextureGenerator::Properties::SourceSRGB = "SourceSRGB";
const char *TextureGenerator::Properties::SourcePremultipliedAlpha = "SourcePremultipliedAlpha";
const char *TextureGenerator::Properties::MaxFilter = "MaxFilter";
const char *TextureGenerator::Properties::AddressU = "AddressU";
const char *TextureGenerator::Properties::AddressV = "AddressV";
const char *TextureGenerator::Properties::AddressW = "AddressW";
const char *TextureGenerator::Properties::BorderColor = "BorderColor";
const char *TextureGenerator::Properties::GenerateMipmaps = "GenerateMipmaps";
const char *TextureGenerator::Properties::MipmapResizeFilter = "MipmapResizeFilter";
const char *TextureGenerator::Properties::NPOTMipmaps = "NPOTMipmaps";
const char *TextureGenerator::Properties::EnableSRGB = "EnableSRGB";
const char *TextureGenerator::Properties::EnablePremultipliedAlpha = "EnablePremultipliedAlpha";
const char *TextureGenerator::Properties::EnableTextureCompression = "EnableTextureCompression";

TextureGenerator::TextureGenerator()
    : m_eTextureType(TEXTURE_TYPE_COUNT),
      m_eTexturePlatform(TEXTURE_PLATFORM_DXTC),
      m_ePixelFormat(PIXEL_FORMAT_UNKNOWN),
      m_nMipLevels(0),
      m_nArraySize(0),
      m_analyzed(false),
      m_bHasAlpha(false),
      m_bHasAlphaLevels(false),
      m_iWidth(0),
      m_iHeight(0),
      m_iDepth(0),
      m_pImages(NULL),
      m_nImages(0)
{

}

TextureGenerator::TextureGenerator(const TextureGenerator &copy)
    : m_eTextureType(copy.m_eTextureType),
      m_eTexturePlatform(copy.m_eTexturePlatform),
      m_propertyList(copy.m_propertyList),
      m_ePixelFormat(copy.m_ePixelFormat),
      m_nMipLevels(copy.m_nMipLevels),
      m_nArraySize(copy.m_nArraySize),
      m_analyzed(copy.m_analyzed),
      m_bHasAlpha(copy.m_bHasAlpha),
      m_bHasAlphaLevels(copy.m_bHasAlphaLevels),
      m_iWidth(copy.m_iWidth),
      m_iHeight(copy.m_iHeight),
      m_iDepth(copy.m_iDepth),
      m_nImages(copy.m_nImages)
{
    DebugAssert(m_nImages > 0);
    m_pImages = new Image[m_nImages];
    for (uint32 i = 0; i < m_nImages; i++)
        m_pImages[i].Copy(copy.m_pImages[i]);
}

TextureGenerator::~TextureGenerator()
{
    if (m_pImages != NULL)
        delete[] m_pImages;
}

void TextureGenerator::SetTexturePlatform(TEXTURE_PLATFORM platform)
{
    m_eTexturePlatform = platform;
}

void TextureGenerator::SetTextureUsage(TEXTURE_USAGE usage)
{
    m_propertyList.SetPropertyValue(Properties::Usage, NameTable_GetNameString(NameTables::TextureUsage, usage));
}

void TextureGenerator::SetTextureFilter(TEXTURE_FILTER filter)
{
    m_propertyList.SetPropertyValue(Properties::MaxFilter, NameTable_GetNameString(NameTables::TextureFilter, filter));
}

void TextureGenerator::SetTextureAddressModeU(TEXTURE_ADDRESS_MODE mode)
{
    m_propertyList.SetPropertyValue(Properties::AddressU, NameTable_GetNameString(NameTables::TextureAddressMode, mode));
}

void TextureGenerator::SetTextureAddressModeV(TEXTURE_ADDRESS_MODE mode)
{
    m_propertyList.SetPropertyValue(Properties::AddressV, NameTable_GetNameString(NameTables::TextureAddressMode, mode));
}

void TextureGenerator::SetTextureAddressModeW(TEXTURE_ADDRESS_MODE mode)
{
    m_propertyList.SetPropertyValue(Properties::AddressW, NameTable_GetNameString(NameTables::TextureAddressMode, mode));
}

void TextureGenerator::SetMipMapResizeFilter(IMAGE_RESIZE_FILTER filter)
{
    m_propertyList.SetPropertyValue(Properties::MipmapResizeFilter, NameTable_GetNameString(NameTables::ImageResizeFilter, filter));
}

void TextureGenerator::SetGenerateMipmaps(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::GenerateMipmaps, enabled);
}

void TextureGenerator::SetSourcePremultipliedAlpha(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::SourcePremultipliedAlpha, enabled);
}

void TextureGenerator::SetEnablePremultipliedAlpha(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::EnablePremultipliedAlpha, enabled);
}

void TextureGenerator::SetEnableTextureCompression(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::EnableTextureCompression, enabled);
}

void TextureGenerator::SetEnableSRGB(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::EnableSRGB, enabled);
}

void TextureGenerator::SetSourceSRGB(bool enabled)
{
    m_propertyList.SetPropertyValueBool(Properties::SourceSRGB, enabled);
}

const Image *TextureGenerator::GetImage(uint32 arrayIndex, uint32 mipIndex) const
{
    DebugAssert(arrayIndex < m_nArraySize && mipIndex < m_nMipLevels);
    return &m_pImages[arrayIndex * m_nMipLevels + mipIndex];
}

void TextureGenerator::SetDefaultProperties(TEXTURE_USAGE usage)
{
    bool useSRGB = (usage == TEXTURE_USAGE_COLOR_MAP);

    m_propertyList.SetPropertyValue(Properties::Usage, NameTable_GetNameString(NameTables::TextureUsage, usage));
    m_propertyList.SetPropertyValueBool(Properties::SourceSRGB, useSRGB);
    m_propertyList.SetPropertyValueBool(Properties::SourcePremultipliedAlpha, false);
    m_propertyList.SetPropertyValue(Properties::MaxFilter, NameTable_GetNameString(NameTables::TextureFilter, TEXTURE_FILTER_ANISOTROPIC));
    m_propertyList.SetPropertyValue(Properties::AddressU, NameTable_GetNameString(NameTables::TextureAddressMode, TEXTURE_ADDRESS_MODE_CLAMP));
    m_propertyList.SetPropertyValue(Properties::AddressV, NameTable_GetNameString(NameTables::TextureAddressMode, TEXTURE_ADDRESS_MODE_CLAMP));
    m_propertyList.SetPropertyValue(Properties::AddressW, NameTable_GetNameString(NameTables::TextureAddressMode, TEXTURE_ADDRESS_MODE_CLAMP));
    m_propertyList.SetPropertyValueFloat4(Properties::BorderColor, float4::One);
    m_propertyList.SetPropertyValueBool(Properties::GenerateMipmaps, true);
    m_propertyList.SetPropertyValue(Properties::MipmapResizeFilter, NameTable_GetNameString(NameTables::ImageResizeFilter, DEFAULT_MIPMAP_RESIZE_FILTER));
    m_propertyList.SetPropertyValueBool(Properties::NPOTMipmaps, false);
    m_propertyList.SetPropertyValueBool(Properties::EnableSRGB, useSRGB);
    m_propertyList.SetPropertyValueBool(Properties::EnablePremultipliedAlpha, true);
    m_propertyList.SetPropertyValueBool(Properties::EnableTextureCompression, true);
}

bool TextureGenerator::InternalCreate(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 mipLevels, uint32 arraySize)
{
    DebugAssert(textureType < TEXTURE_TYPE_COUNT);
    DebugAssert(pixelFormat != PIXEL_FORMAT_UNKNOWN && pixelFormat < PIXEL_FORMAT_COUNT);

    if (arraySize == 0 || mipLevels == 0)
        return false;

    // check dimensions
    switch (textureType)
    {
    case TEXTURE_TYPE_1D:
    case TEXTURE_TYPE_1D_ARRAY:
        {
            if (width < 1 || height != 1 || depth != 1)
                return false;

            if ((textureType == TEXTURE_TYPE_1D && arraySize != 1) ||
                (textureType == TEXTURE_TYPE_1D_ARRAY && arraySize == 0))
            {
                return false;
            }
        }
        break;

    case TEXTURE_TYPE_2D:
    case TEXTURE_TYPE_2D_ARRAY:
        {
            if (width < 1 || height < 1 || depth != 1)
                return false;

            if ((textureType == TEXTURE_TYPE_2D && arraySize != 1) ||
                (textureType == TEXTURE_TYPE_2D_ARRAY && arraySize == 0))
            {
                return false;
            }
        }
        break;

    case TEXTURE_TYPE_3D:
        {
            if (width < 1 || height < 1 || depth < 1)
                return false;
        }
        break;

    case TEXTURE_TYPE_CUBE:
    case TEXTURE_TYPE_CUBE_ARRAY:
        {
            if (width < 1 || height < 1 || depth != 1)
                return false;

            if ((textureType == TEXTURE_TYPE_CUBE && arraySize != 6) ||
                (textureType == TEXTURE_TYPE_CUBE_ARRAY && (arraySize < 6 || (arraySize % 6) != 0)))
            {
                return false;
            }
        }
        break;
    }

    // check mip count
    if (mipLevels == 0 || mipLevels > TEXTURE_MAX_MIPMAP_COUNT)
        return false;

    // setup everything
    m_eTextureType = textureType;
    m_ePixelFormat = pixelFormat;
    m_nMipLevels = mipLevels;
    m_nArraySize = arraySize;
    m_iWidth = width;
    m_iHeight = height;
    m_iDepth = depth;

    // set defaults here too, but they'll be replaced
    m_bHasAlpha = false;
    m_bHasAlphaLevels = false;

    // setup image array
    m_nImages = m_nMipLevels * m_nArraySize;
    m_pImages = new Image[m_nImages];
    uint32 imageIndex = 0;
    for (uint32 i = 0; i < m_nArraySize; i++)
    {
        for (uint32 j = 0; j < m_nMipLevels; j++)
        {
            uint32 mipWidth = Max(width >> j, (uint32)1);
            uint32 mipHeight = Max(height >> j, (uint32)1);
            uint32 mipDepth = Max(depth >> j, (uint32)1);
            m_pImages[imageIndex].Create(m_ePixelFormat, mipWidth, mipHeight, mipDepth);
            Y_memzero(m_pImages[imageIndex].GetData(), m_pImages[imageIndex].GetDataSize());
            imageIndex++;
        }
    }

    return true;
}

bool TextureGenerator::Create(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 arraySize, TEXTURE_USAGE usage /* = TEXTURE_USAGE_NONE */)
{
    if (!InternalCreate(textureType, pixelFormat, width, height, depth, 1, arraySize))
        return false;

    SetDefaultProperties(usage);
    return true;
}

bool TextureGenerator::Create(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 mipLevels, uint32 arraySize, TEXTURE_USAGE usage /* = TEXTURE_USAGE_NONE */)
{
    if (!InternalCreate(textureType, pixelFormat, width, height, depth, mipLevels, arraySize))
        return false;

    SetDefaultProperties(usage);
    return true;
}

bool TextureGenerator::Load(const char *fileName, ByteStream *pStream)
{
    ZipArchive *pArchive = ZipArchive::OpenArchiveReadOnly(pStream);
    if (pArchive == NULL)
    {
        Log_ErrorPrintf("TextureGenerator::Load: Could not open '%s' as archive.", fileName);
        return false;
    }

    bool result = false;

    // load xml
    {
        AutoReleasePtr<ByteStream> pDescriptorStream = pArchive->OpenFile("texture.xml", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pDescriptorStream == NULL)
        {
            Log_ErrorPrintf("TextureGenerator::Load: Could not find 'texture.xml' in archive '%s'.", fileName);
            goto CLEANUP;
        }

        XMLReader xmlReader;
        if (!xmlReader.Create(pDescriptorStream, "texture.xml") || !xmlReader.SkipToElement("texture"))
        {
            Log_ErrorPrintf("TextureGenerator::Load: Could not parse 'texture.xml' in archive '%s'.", fileName);
            goto CLEANUP;
        }

        // read attributes
        const char *textureTypeStr = xmlReader.FetchAttribute("type");
        const char *textureMipLevelsStr = xmlReader.FetchAttribute("miplevels");
        const char *textureArraySizeStr = xmlReader.FetchAttribute("arraysize");
        const char *texturePixelFormatStr = xmlReader.FetchAttribute("format");
        const char *textureWidthStr = xmlReader.FetchAttribute("width");
        const char *textureHeightStr = xmlReader.FetchAttribute("height");
        const char *textureDepthStr = xmlReader.FetchAttribute("depth");
        if (textureTypeStr == NULL || textureMipLevelsStr == NULL || textureArraySizeStr == NULL || texturePixelFormatStr == NULL ||
            textureWidthStr == NULL || textureHeightStr == NULL || textureDepthStr == NULL)
        {
            xmlReader.PrintError("missing attributes");
            goto CLEANUP;
        }

        TEXTURE_TYPE textureType;
        if (!NameTable_TranslateType(NameTables::TextureType, textureTypeStr, &textureType, true))
        {
            xmlReader.PrintError("invalid texture type: %s", textureTypeStr);
            goto CLEANUP;
        }

        PIXEL_FORMAT pixelFormat;
        if (!NameTable_TranslateType(NameTables::PixelFormat, texturePixelFormatStr, &pixelFormat, true))
        {
            xmlReader.PrintError("invalid pixel format: %s", texturePixelFormatStr);
            goto CLEANUP;
        }

        if (!InternalCreate(textureType, pixelFormat,
                            StringConverter::StringToUInt32(textureWidthStr),
                            StringConverter::StringToUInt32(textureHeightStr),
                            StringConverter::StringToUInt32(textureDepthStr),
                            StringConverter::StringToUInt32(textureMipLevelsStr),
                            StringConverter::StringToUInt32(textureArraySizeStr)))
        {
            xmlReader.PrintError("could not create images");
            goto CLEANUP;
        }

        if (!xmlReader.SkipToElement("properties"))
        {
            xmlReader.PrintError("could not skip to properties element");
            goto CLEANUP;
        }

        if (!m_propertyList.LoadFromXML(xmlReader))
        {
            xmlReader.PrintError("could not parse properties");
            goto CLEANUP;
        }
    }

    // load images
    {
        PathString imageFileName;
        for (uint32 i = 0; i < m_nImages; i++)
        {
            imageFileName.Format("%u.png", i);

            AutoReleasePtr<ByteStream> pImageStream = pArchive->OpenFile(imageFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
            if (pImageStream == NULL)
            {
                Log_ErrorPrintf("TextureGenerator::Load: Could not find '%s' in archive '%s'.", imageFileName.GetCharArray(), fileName);
                goto CLEANUP;
            }

            Image decodedImage;
            ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(imageFileName, pImageStream);
            if (pCodec == NULL || !pCodec->DecodeImage(&decodedImage, imageFileName, pImageStream))
            {
                Log_ErrorPrintf("TextureGenerator::Load: Could not decode image '%s' in archive '%s'.", imageFileName.GetCharArray(), fileName);
                goto CLEANUP;
            }

            Image &destinationImage = m_pImages[i];
            if (decodedImage.GetPixelFormat() != destinationImage.GetPixelFormat())
            {
                // freeimage likes to nuke the alpha channel on fully opaque images for us.. thanks for that.. so convert back to the original format
                if (!decodedImage.ConvertPixelFormat(m_ePixelFormat))
                {
                    Log_ErrorPrintf("TextureGenerator::Load: Image '%s' pixel format %s different to texture %s and conversion failed", imageFileName.GetCharArray(), NameTable_GetNameString(NameTables::PixelFormat, decodedImage.GetPixelFormat()), NameTable_GetNameString(NameTables::PixelFormat, destinationImage.GetPixelFormat()));
                    goto CLEANUP;
                }
            }

            if (decodedImage.GetWidth() != destinationImage.GetWidth() ||
                decodedImage.GetHeight() != destinationImage.GetHeight() ||
                decodedImage.GetDepth() != destinationImage.GetDepth())
            {
                Log_ErrorPrintf("TextureGenerator::Load: Image '%s' dimensions %ux%ux%u different to texture %ux%ux%u", imageFileName.GetCharArray(), decodedImage.GetWidth(), decodedImage.GetHeight(), decodedImage.GetDepth(), destinationImage.GetWidth(), destinationImage.GetHeight(), destinationImage.GetDepth());
                goto CLEANUP;
            }

            destinationImage.Copy(decodedImage);
        }
    }

    result = true;

CLEANUP:
    delete pArchive;
    return result;
}

bool TextureGenerator::Save(ByteStream *pOutputStream) const
{
    ZipArchive *pArchive = ZipArchive::CreateArchive(pOutputStream);
    if (pArchive == NULL)
    {
        Log_ErrorPrintf("TextureGenerator::Save: Could not create archive.");
        return false;
    }

    bool result = false;

    // save xml
    {
        AutoReleasePtr<ByteStream> pTextureOutputSTream = pArchive->OpenFile("texture.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
        if (pTextureOutputSTream == NULL)
        {
            Log_ErrorPrintf("TextureGenerator::Save: Could not open texture.xml for writing");
            goto CLEANUP;
        }

        XMLWriter xmlWriter;
        if (!xmlWriter.Create(pTextureOutputSTream))
        {
            Log_ErrorPrintf("TextureGenerator::Save: Could not create texture.xml XMLWriter");
            goto CLEANUP;
        }

        xmlWriter.StartElement("texture");
        {
            xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::TextureType, m_eTextureType));
            xmlWriter.WriteAttribute("miplevels", StringConverter::UInt32ToString(m_nMipLevels));
            xmlWriter.WriteAttribute("arraysize", StringConverter::UInt32ToString(m_nArraySize));
            xmlWriter.WriteAttribute("format", NameTable_GetNameString(NameTables::PixelFormat, m_ePixelFormat));
            xmlWriter.WriteAttribute("width", StringConverter::UInt32ToString(m_iWidth));
            xmlWriter.WriteAttribute("height", StringConverter::UInt32ToString(m_iHeight));
            xmlWriter.WriteAttribute("depth", StringConverter::UInt32ToString(m_iDepth));

            // write properties
            xmlWriter.StartElement("properties");
            m_propertyList.SaveToXML(xmlWriter);
            xmlWriter.EndElement();
        }
        xmlWriter.EndElement();

        if (xmlWriter.InErrorState() || pTextureOutputSTream->InErrorState())
            goto CLEANUP;

        xmlWriter.Close();
    }

    // save images
    {
        PathString imageFileName;
        PropertyTable encoderOptions;
        encoderOptions.SetPropertyValueUInt32(ImageCodecEncoderOptions::PNG_COMPRESSION_LEVEL, 6);

        for (uint32 i = 0; i < m_nImages; i++)
        {
            imageFileName.Format("%u.png", i);

            ImageCodec *pCodec = ImageCodec::GetImageCodecForFileName(imageFileName);
            if (pCodec == NULL)
            {
                Log_ErrorPrintf("TextureGenerator::Save: Could not get image codec for '%s'", imageFileName.GetCharArray());
                goto CLEANUP;
            }

            AutoReleasePtr<ByteStream> pImageStream = pArchive->OpenFile(imageFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
            if (pImageStream == NULL)
            {
                Log_ErrorPrintf("TextureGenerator::Save: Could not open '%s' for writing", imageFileName.GetCharArray());
                goto CLEANUP;
            }

            if (!pCodec->EncodeImage(imageFileName, pImageStream, &m_pImages[i], encoderOptions))
            {
                Log_ErrorPrintf("TextureGenerator::Save: Could not encode image '%s'", imageFileName.GetCharArray());
                goto CLEANUP;
            }
        }
    }

    // ok
    result = true;

CLEANUP:
    if (result)
    {
        pArchive->CommitChanges();
        pOutputStream->Commit();
        delete pArchive;
    }
    else
    {
        pArchive->DiscardChanges();
        pOutputStream->Discard();
        delete pArchive;
    }
    return result;
}

bool TextureGenerator::Resize(IMAGE_RESIZE_FILTER resizeFilter, uint32 width, uint32 height, uint32 depth)
{
    // throw away mip chain
    Image *pNewImages = new Image[m_nArraySize];
    for (uint32 i = 0; i < m_nImages; i++)
    {
        if (!pNewImages[i].CopyAndResize(m_pImages[i * m_nMipLevels], resizeFilter, width, height, depth))
        {
            delete[] pNewImages;
            return false;
        }
    }

    delete[] m_pImages;
    m_nMipLevels = 1;
    m_pImages = pNewImages;
    m_nImages = m_nArraySize;
    return true;
}

bool TextureGenerator::AddMask(uint32 arrayIndex, const Image *pMaskImage)
{
    DebugAssert(arrayIndex < m_nArraySize);

    if (m_ePixelFormat != PIXEL_FORMAT_R8G8B8A8_UNORM)
    {
        Log_ErrorPrintf("TextureGenerator::AddMask: Pixel format must be R8G8B8A8");
        return false;
    }

    Image &destinationImage = m_pImages[arrayIndex * m_nMipLevels];
    if (destinationImage.GetWidth() != pMaskImage->GetWidth() ||
        destinationImage.GetHeight() != pMaskImage->GetHeight())
    {
        Log_ErrorPrintf("TextureGenerator::AddMask: Image dimensions must match");
        return false;
    }

    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pMaskImage->GetPixelFormat());
    bool useAlphaChannel = pPixelFormatInfo->HasAlpha;

    Image tempImage;
    if (useAlphaChannel)
    {
        if (!tempImage.CopyAndConvertPixelFormat(*pMaskImage, PIXEL_FORMAT_R8G8B8A8_UNORM))
            return false;

        // take the alpha channel from each pixel of the mask and apply it to the source
        for (uint32 y = 0; y < destinationImage.GetHeight(); y++)
        {
            const byte *pMaskPixels = tempImage.GetData() + (y * tempImage.GetDataRowPitch());
            byte *pPixels = destinationImage.GetData() + (y * destinationImage.GetDataRowPitch());
            for (uint32 x = 0; x < destinationImage.GetWidth(); x++)
            {
                pPixels[3] = pMaskPixels[3];
                pMaskPixels += 4;
                pPixels += 4;
            }
        }
    }
    else
    {
        if (!tempImage.CopyAndConvertPixelFormat(*pMaskImage, PIXEL_FORMAT_R8_UNORM))
            return false;

        // take the alpha channel from each pixel of the mask and apply it to the source's alpha channel
        for (uint32 y = 0; y < destinationImage.GetHeight(); y++)
        {
            const byte *pMaskPixels = tempImage.GetData() + (y * tempImage.GetDataRowPitch());
            byte *pPixels = destinationImage.GetData() + (y * destinationImage.GetDataRowPitch());
            for (uint32 x = 0; x < destinationImage.GetWidth(); x++)
            {
                pPixels[3] = pMaskPixels[0];
                pMaskPixels++;
                pPixels += 4;
            }
        }
    }

    // change analyzed flag
    m_analyzed = false;
    return true;
}

bool TextureGenerator::AddImage(const Image *pImage)
{
    if (m_eTextureType != TEXTURE_TYPE_2D_ARRAY)
        return false;

    Image &referenceImage = m_pImages[0];
    if (pImage->GetWidth() != referenceImage.GetWidth() ||
        pImage->GetHeight() != referenceImage.GetHeight() ||
        pImage->GetDepth() != referenceImage.GetDepth() ||
        pImage->GetPixelFormat() != referenceImage.GetPixelFormat())
    {
        Log_ErrorPrintf("TextureGenerator::AddImage: Incompatible image.");
        return false;
    }

    uint32 arrayIndex = m_nArraySize;
    SetArraySize(m_nArraySize + 1);
    if (!SetImage(arrayIndex, pImage))
    {
        SetArraySize(arrayIndex);
        return false;
    }

    return true;
}

bool TextureGenerator::SetImage(uint32 arrayIndex, const Image *pImage)
{
    if (arrayIndex >= m_nArraySize)
    {
        Log_ErrorPrintf("TextureGenerator::SetImage: Array index (%u) out of range (%u).", arrayIndex, m_nArraySize);
        return false;
    }

    Image &destinationImage = m_pImages[arrayIndex * m_nMipLevels + 0];
    if (pImage->GetWidth() != destinationImage.GetWidth() || 
        pImage->GetHeight() != destinationImage.GetHeight() ||
        pImage->GetDepth() != destinationImage.GetDepth() ||
        pImage->GetPixelFormat() != destinationImage.GetPixelFormat())
    {
        Log_ErrorPrintf("TextureGenerator::SetImage: Incompatible image.");
        return false;
    }

    destinationImage.Copy(*pImage);
    return true;
}

bool TextureGenerator::SetImage(uint32 arrayIndex, uint32 mipIndex, const Image *pImage)
{
    if (arrayIndex >= m_nArraySize)
    {
        Log_ErrorPrintf("TextureGenerator::SetImage: Array index (%u) out of range (%u).", arrayIndex, m_nArraySize);
        return false;
    }

    if (mipIndex >= m_nMipLevels)
    {
        Log_ErrorPrintf("TextureGenerator::SetImage: Mip index (%u) out of range (%u).", mipIndex, m_nMipLevels);
        return false;
    }

    Image &destinationImage = m_pImages[arrayIndex * m_nMipLevels + mipIndex];
    if (pImage->GetWidth() != destinationImage.GetWidth() || 
        pImage->GetHeight() != destinationImage.GetHeight() ||
        pImage->GetDepth() != destinationImage.GetDepth() ||
        pImage->GetPixelFormat() != destinationImage.GetPixelFormat())
    {
        Log_ErrorPrintf("TextureGenerator::SetImage: Incompatible image.");
        return false;
    }

    destinationImage.Copy(*pImage);
    return true;
}

void TextureGenerator::SetArraySize(uint32 newArraySize)
{
    DebugAssert(newArraySize > 0);
    DebugAssert(m_eTextureType == TEXTURE_TYPE_1D_ARRAY || m_eTextureType == TEXTURE_TYPE_2D_ARRAY || m_eTextureType == TEXTURE_TYPE_CUBE_ARRAY);

    if (newArraySize == m_nArraySize)
        return;

    Image *pNewImages = new Image[newArraySize];
    uint32 i;
    for (i = 0; i < m_nArraySize && i < newArraySize; i++)
    {
        // copy existing image
        pNewImages[i].Copy(m_pImages[i * m_nMipLevels]);
    }
    for (; i < newArraySize; i++)
    {
        // empty new image
        pNewImages[i].Create(m_ePixelFormat, m_iWidth, m_iHeight, m_iDepth);
        Y_memzero(pNewImages[i].GetData(), pNewImages[i].GetDataSize());
    }

    // update size
    delete[] m_pImages;
    m_pImages = pNewImages;
    m_nImages = newArraySize;
    m_nMipLevels = 1;
    m_nArraySize = newArraySize;
}

bool TextureGenerator::GenerateMipmaps()
{
    DebugAssert(m_nImages > 0 && m_nArraySize > 0);

    // get uncompressed pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    DebugAssert(pPixelFormatInfo != NULL);
    if (m_ePixelFormat != pPixelFormatInfo->UncompressedFormat)
    {
        // cannot allocate mips while image is still compressed
        return false;
    }

    // npot mipmaps?
    uint32 baseWidth = m_iWidth;
    uint32 baseHeight = m_iHeight;
    uint32 baseDepth = m_iDepth;
    if (!m_propertyList.GetPropertyValueDefaultBool(Properties::NPOTMipmaps, false))
    {
        if (baseWidth > 0 && !Y_ispow2(baseWidth))
            baseWidth = Y_nextpow2(baseWidth);
        if (baseHeight > 0 && !Y_ispow2(baseHeight))
            baseHeight = Y_nextpow2(baseHeight);
        if (baseDepth > 0 && !Y_ispow2(baseDepth))
            baseDepth = Y_nextpow2(baseDepth);
    }

    // determine number of mipmaps
    uint32 currentWidth = baseWidth;
    uint32 currentHeight = baseHeight;
    uint32 currentDepth = baseDepth;
    uint32 nMips = 1;
    for (;;)
    {
        if ((currentWidth == 1 && currentHeight == 1 && currentDepth == 1) || nMips == TEXTURE_MAX_MIPMAP_COUNT)
            break;

        currentWidth = Max(currentWidth / 2, (uint32)1);
        currentHeight = Max(currentHeight / 2, (uint32)1);
        currentDepth = Max(currentDepth / 2, (uint32)1);
        nMips++;
    }
    
    // calculate new image count
    uint32 nNewImages = nMips * m_nArraySize;
    Image *pNewImageArray = new Image[nNewImages];
    Log_InfoPrintf("TextureGenerator::GenerateMipmaps: Generating %u mipmaps per image...", nMips);

    // get mip resize filter
    IMAGE_RESIZE_FILTER resizeFilter;
    if (!NameTable_TranslateType(NameTables::ImageResizeFilter, m_propertyList.GetPropertyValueDefault(Properties::MipmapResizeFilter), &resizeFilter, true))
        resizeFilter = DEFAULT_MIPMAP_RESIZE_FILTER;

    // for each array image
    uint32 i, j;
    for (i = 0; i < m_nArraySize; i++)
    {
        // convert to the uncompressed format
        Image baseImage;
        baseImage.Copy(m_pImages[i * m_nMipLevels]);

        // resize full scale image if necessary
        if (baseWidth != baseImage.GetWidth() ||
            baseHeight != baseImage.GetHeight() ||
            baseDepth != baseImage.GetDepth())
        {
            if (!baseImage.Resize(resizeFilter, baseWidth, baseHeight, baseDepth))
            {
                Log_ErrorPrintf("TextureGenerator::GenerateMipmaps: Could not resize base image.");
                delete[] pNewImageArray;
                return false;
            }
        }

        // store mip levels
        currentWidth = baseWidth;
        currentHeight = baseHeight;
        currentDepth = baseDepth;
        for (j = 0; j < nMips; j++)
        {
            Image &destImage = pNewImageArray[i * nMips + j];
            if (j == 0)
            {
                // miplevel 0 can just be copied
                destImage.Copy(baseImage);
            }
            else
            {
                if (!destImage.CopyAndResize(baseImage, resizeFilter, currentWidth, currentHeight, currentDepth))
                {
                    Log_ErrorPrintf("TextureGenerator::GenerateMipmaps: Could not copy/resize miplevel %u of array index %u", j, i);
                    delete[] pNewImageArray;
                    return false;
                }
            }

            currentWidth = Max(currentWidth / 2, (uint32)1);
            currentHeight = Max(currentHeight / 2, (uint32)1);
            currentDepth = Max(currentDepth / 2, (uint32)1);
        }
    }

    // store everything
    m_iWidth = baseWidth;
    m_iHeight = baseHeight;
    m_iDepth = baseDepth;
    m_nMipLevels = nMips;
    m_nImages = nNewImages;

    // update images
    delete[] m_pImages;
    m_pImages = pNewImageArray;
    return true;
}

bool TextureGenerator::ConvertToPixelFormat(PIXEL_FORMAT newPixelFormat)
{
    if (m_ePixelFormat == newPixelFormat)
        return true;

    uint32 i;
    Image *pNewImageArray = new Image[m_nImages];
    for (i = 0; i < m_nImages; i++)
    {
        if (!pNewImageArray[i].CopyAndConvertPixelFormat(m_pImages[i], newPixelFormat))
        {
            Log_ErrorPrintf("TextureGenerator::ConvertToPixelFormat: Failed to convert pixel format of image %u", i);
            delete[] pNewImageArray;
            return false;
        }
    }

    m_ePixelFormat = newPixelFormat;
    delete[] m_pImages;
    m_pImages = pNewImageArray;
    return true;
}

bool TextureGenerator::AnalyzeImage()
{
    uint32 i;
    Image tempImage;
    m_bHasAlpha = false;
    m_bHasAlphaLevels = false;

    // if it doesn't have an alpha channel, skip it outright
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    if (pPixelFormatInfo->HasAlpha)
    {
        // only look at miplevel 0
        for (i = 0; i < m_nImages; i += m_nMipLevels)
        {
            // convert to r8g8b8a8
            Image *pImageToAnalyse;
            if (m_pImages[i].GetPixelFormat() == PIXEL_FORMAT_R8G8B8A8_UNORM)
            {
                // use in place
                pImageToAnalyse = &m_pImages[i];
            }
            else
            {
                if (!tempImage.CopyAndConvertPixelFormat(m_pImages[i], PIXEL_FORMAT_R8G8B8A8_UNORM))
                {
                    Log_ErrorPrintf("TextureGenerator::AnalyseImage: CopyAndConvertPixelFormat failed.");
                    return false;
                }

                pImageToAnalyse = &tempImage;
            }

            const byte *pDataPtr = pImageToAnalyse->GetData();
            for (uint32 j = 0; j < pImageToAnalyse->GetDepth(); j++)
            {
                const byte *pSlicePtr = pDataPtr;
                for (uint32 k = 0; k < pImageToAnalyse->GetHeight(); k++)
                {
                    const uint32 *pRowPtr = reinterpret_cast<const uint32 *>(pSlicePtr);
                    for (uint32 l = 0; l < pImageToAnalyse->GetWidth(); l++)
                    {
                        uint32 pixel = *(pRowPtr++);
                        uint8 alpha = uint8(pixel >> 24);
                        if (alpha != 0xFF)
                        {
                            m_bHasAlpha = true;
                            if (alpha != 0x00)
                                m_bHasAlphaLevels = true;
                        }
                    }

                    pSlicePtr += pImageToAnalyse->GetDataRowPitch();
                }

                pDataPtr += pImageToAnalyse->GetDataSlicePitch();
            }
        }
    }

    m_analyzed = true;
    return true;
}

bool TextureGenerator::UncompressImage()
{
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    DebugAssert(pPixelFormatInfo != NULL);

    if (m_ePixelFormat == pPixelFormatInfo->UncompressedFormat)
        return true;

    return ConvertToPixelFormat(pPixelFormatInfo->UncompressedFormat);
}

bool TextureGenerator::RemoveAlphaChannel()
{
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    DebugAssert(pPixelFormatInfo != NULL);

    if (!pPixelFormatInfo->HasAlpha)
        return true;

    if (!ConvertToPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM))
        return false;

    // update analysis
    m_bHasAlpha = false;
    m_bHasAlphaLevels = false;
    return true;
}

bool TextureGenerator::RemoveAlphaChannelIfOpaque()
{
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    DebugAssert(pPixelFormatInfo != NULL);

    if (!pPixelFormatInfo->HasAlpha)
        return true;

    if (!m_analyzed && !AnalyzeImage())
        return false;

    if (m_bHasAlpha)
        return true;

    Log_InfoPrintf("TextureGenerator::RemoveAlphaChannelIfOpaque: Image is completely opaque, removing alpha channel (%s -> R8G8B8_UNORM)...", NameTable_GetNameString(NameTables::PixelFormat, m_ePixelFormat));
    return ConvertToPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM);
}

bool TextureGenerator::ConvertBGRToRGB()
{
    // convert BGR[A] formats to RGB[A]. this is what freeimage will read them back currently as.
    if (m_ePixelFormat == PIXEL_FORMAT_B8G8R8_UNORM || m_ePixelFormat == PIXEL_FORMAT_B8G8R8X8_UNORM)
    {
        if (!ConvertToPixelFormat(PIXEL_FORMAT_R8G8B8_UNORM))
            return false;
    }
    else if (m_ePixelFormat == PIXEL_FORMAT_B8G8R8A8_UNORM)
    {
        if (!ConvertToPixelFormat(PIXEL_FORMAT_R8G8B8A8_UNORM))
            return false;
    }

    return true;
}

bool TextureGenerator::ConvertToTextureArray()
{
    if (m_eTextureType != TEXTURE_TYPE_2D)
        return false;

    // this can just be switched, nothing else really has to change as it will only contain one image
    DebugAssert(m_nArraySize == 1);
    m_eTextureType = TEXTURE_TYPE_2D_ARRAY;
    return true;
}

bool TextureGenerator::ConvertToPremultipliedAlpha()
{
    if (m_propertyList.GetPropertyValueDefaultBool(Properties::SourcePremultipliedAlpha, true))
        return true;

    if (!ConvertToPixelFormat(PIXEL_FORMAT_R8G8B8A8_UNORM))
        return false;

    Log_InfoPrintf("TextureGenerator::ConvertToPremultipliedAlpha: Converting to premultiplied alpha...");

    uint32 i, j, k;
    for (i = 0; i < m_nImages; i++)
    {
        Image *pCurrentImage = &m_pImages[i];
        byte *pRow = reinterpret_cast<byte *>(pCurrentImage->GetData());
        for (j = 0; j < pCurrentImage->GetHeight(); j++)
        {
            byte *pPixels = pRow;
            for (k = 0; k < pCurrentImage->GetWidth(); k++)
            {
                float r = ((float)pPixels[0]) / 255.0f;
                float g = ((float)pPixels[1]) / 255.0f;
                float b = ((float)pPixels[2]) / 255.0f;
                float a = ((float)pPixels[3]) / 255.0f;
                pPixels[0] = (byte)Math::Clamp(((uint32)(r * a * 255.0f)), (uint32)0, (uint32)255);
                pPixels[1] = (byte)Math::Clamp(((uint32)(g * a * 255.0f)), (uint32)0, (uint32)255);
                pPixels[2] = (byte)Math::Clamp(((uint32)(b * a * 255.0f)), (uint32)0, (uint32)255);
                pPixels += 4;
            }

            pRow += pCurrentImage->GetDataRowPitch();
        }
    }

    m_propertyList.SetPropertyValueBool(Properties::SourcePremultipliedAlpha, true);
    return true;
}

PIXEL_FORMAT TextureGenerator::GetBestPixelFormat(bool allowCompression) const
{
    if (!m_analyzed && !const_cast<TextureGenerator *>(this)->AnalyzeImage())
        return m_ePixelFormat;

    // disable compression on small images
    if (m_iWidth < 4 || m_iHeight < 4)
        allowCompression = false;

    // get usage
    TEXTURE_USAGE textureUsage;
    if (!NameTable_TranslateType(NameTables::TextureUsage, m_propertyList.GetPropertyValueDefault(Properties::Usage, ""), &textureUsage, true))
        textureUsage = TEXTURE_USAGE_NONE;

    // automatically determine best output format
    switch (m_eTexturePlatform)
    {
    case TEXTURE_PLATFORM_DXTC:
        {
            switch (textureUsage)
            {
            case TEXTURE_USAGE_NONE:
            case TEXTURE_USAGE_COLOR_MAP:
            case TEXTURE_USAGE_UI_ASSET:
                {
                    // TODO: Optional BC7
                    if (allowCompression)
                        return (m_bHasAlpha && m_bHasAlphaLevels) ? PIXEL_FORMAT_BC3_UNORM : PIXEL_FORMAT_BC1_UNORM;
                    else
                        return PIXEL_FORMAT_R8G8B8A8_UNORM;
                }
                break;

            case TEXTURE_USAGE_GLOSS_MAP:
            case TEXTURE_USAGE_ALPHA_MAP:
            case TEXTURE_USAGE_UI_LUMINANCE_ASSET:
            case TEXTURE_USAGE_HEIGHT_MAP:
                {
                    //if (allowCompression)
                        //return PIXEL_FORMAT_BC4_UNORM;
                    //else
                        return PIXEL_FORMAT_R8_UNORM;
                }
                break;

            case TEXTURE_USAGE_NORMAL_MAP:
                {
                    //if (allowCompression)
                        //return PIXEL_FORMAT_BC5_UNORM;
                    //else
                        return PIXEL_FORMAT_R8G8B8A8_UNORM;
                }
                break;
            }

            return PIXEL_FORMAT_R8G8B8A8_UNORM;
        }
        break;

    case TEXTURE_PLATFORM_PVRTC:
    case TEXTURE_PLATFORM_ATC:
    case TEXTURE_PLATFORM_ETC:
    case TEXTURE_PLATFORM_ES2_NOTC:
    case TEXTURE_PLATFORM_ES2_DXTC:
        {
            // RGBA textures for everything
            return PIXEL_FORMAT_R8G8B8A8_UNORM;
        }
        break;
    }
    
    return m_ePixelFormat;
}

bool TextureGenerator::Optimize()
{
    if (!UncompressImage())
        return false;

    if (!ConvertBGRToRGB())
        return false;

    if (!m_analyzed && !AnalyzeImage())
        return false;

    if (!RemoveAlphaChannelIfOpaque())
        return false;

    // convert to premultiplied alpha
    if (m_bHasAlphaLevels && !m_propertyList.GetPropertyValueDefaultBool(Properties::SourcePremultipliedAlpha, false))
    {
        if (m_propertyList.GetPropertyValueDefaultBool(Properties::EnablePremultipliedAlpha, true))
        {
            if (!ConvertToPremultipliedAlpha())
                return false;

            m_propertyList.SetPropertyValueBool(Properties::SourcePremultipliedAlpha, true);
        }
    }

    // generate mipmaps
    if (m_nMipLevels == 1 && m_propertyList.GetPropertyValueDefaultBool(Properties::GenerateMipmaps, true) && !GenerateMipmaps())
        return false;

    return true;
}

bool TextureGenerator::Compile(ByteStream *pOutputStream, TEXTURE_PLATFORM platform /* = NUM_TEXTURE_PLATFORMS */) const
{
    TextureGenerator copy(*this);
    if (platform != NUM_TEXTURE_PLATFORMS)
        copy.SetTexturePlatform(platform);

    return copy.InternalCompile(pOutputStream);
}

bool TextureGenerator::InternalCompile(ByteStream *pOutputStream)
{
    if (!m_analyzed && !AnalyzeImage())
        return false;

    // convert to premultiplied alpha
    bool isPremultipliedAlpha = false;
    if (m_bHasAlpha || m_bHasAlphaLevels)
    {
        isPremultipliedAlpha = m_propertyList.GetPropertyValueDefaultBool(Properties::SourcePremultipliedAlpha, false);
        if (!isPremultipliedAlpha && m_propertyList.GetPropertyValueDefaultBool(Properties::EnablePremultipliedAlpha, true))
        {
            if (!UncompressImage() || !ConvertToPremultipliedAlpha())
                return false;

            isPremultipliedAlpha = true;
        }
    }

    // generate mipmaps
    if (m_nMipLevels == 1 && m_propertyList.GetPropertyValueDefaultBool(Properties::GenerateMipmaps, true) && !GenerateMipmaps())
        return false;

    // todo: srgb conversion

    // get format
    PIXEL_FORMAT preferredPixelFormat = GetBestPixelFormat(m_propertyList.GetPropertyValueDefaultBool(Properties::EnableTextureCompression, true));
    if (m_ePixelFormat != preferredPixelFormat)
    {
        Log_DevPrintf("TextureGenerator::InternalCompile: Converting from %s to optimal format %s", NameTable_GetNameString(NameTables::PixelFormat, m_ePixelFormat), NameTable_GetNameString(NameTables::PixelFormat, preferredPixelFormat));
        if (!ConvertToPixelFormat(preferredPixelFormat))
            return false;
    }

    // switch to srgb format
    bool platformSupportsSRGB = (m_eTexturePlatform < TEXTURE_PLATFORM_ES2_NOTC);
    if (m_propertyList.GetPropertyValueDefaultBool(Properties::SourceSRGB, false) && platformSupportsSRGB)
    {
        PIXEL_FORMAT srgbFormat = PixelFormatHelpers::GetSRGBFormat(m_ePixelFormat);
        if (srgbFormat == PIXEL_FORMAT_UNKNOWN)
        {
            Log_ErrorPrintf("TextureGenerator::InternalCompile: No matching SRGB format for %s", NameTable_GetNameString(NameTables::PixelFormat, m_ePixelFormat));
            return false;
        }

        // just change the image formats
        for (uint32 i = 0; i < m_nImages; i++)
        {
            if (!m_pImages[i].SetPixelFormatWithoutConversion(srgbFormat))
            {
                Log_ErrorPrintf("TextureGenerator::InternalCompile: Failed to change pixel format from %s to %s", NameTable_GetNameString(NameTables::PixelFormat, m_ePixelFormat), NameTable_GetNameString(NameTables::PixelFormat, srgbFormat));
                return false;
            }
        }

        // and update the internal format
        m_ePixelFormat = srgbFormat;
    }

    // save to stream
    {
        uint32 currentOffset = 0;
        DF_TEXTURE_HEADER textureHeader;
        textureHeader.Magic = DF_TEXTURE_HEADER_MAGIC;
        textureHeader.HeaderSize = sizeof(textureHeader);
        textureHeader.TextureType = (uint32)m_eTextureType;
        textureHeader.TexturePlatform = (uint32)m_eTexturePlatform;

        // properties
        {
            const String *pPropertyValue;

            if ((pPropertyValue = m_propertyList.GetPropertyValuePointer(Properties::Usage)) == NULL || !NameTable_TranslateType(NameTables::TextureUsage, *pPropertyValue, &textureHeader.TextureUsage, true))
            {
                Log_WarningPrintf("Texture Compiler Warning: Unknown value for %s (%s), using default.", Properties::Usage, (pPropertyValue != NULL) ? pPropertyValue->GetCharArray() : "null");
                textureHeader.TextureUsage = TEXTURE_USAGE_NONE;
            }

            if ((pPropertyValue = m_propertyList.GetPropertyValuePointer(Properties::MaxFilter)) == NULL || !NameTable_TranslateType(NameTables::TextureFilter, *pPropertyValue, &textureHeader.TextureFilter, true))
            {
                Log_WarningPrintf("Texture Compiler Warning: Unknown value for %s (%s), using default.", Properties::MaxFilter, (pPropertyValue != NULL) ? pPropertyValue->GetCharArray() : "null");
                textureHeader.TextureFilter = TEXTURE_FILTER_ANISOTROPIC;
            }

            if ((pPropertyValue = m_propertyList.GetPropertyValuePointer(Properties::AddressU)) == NULL || !NameTable_TranslateType(NameTables::TextureAddressMode, *pPropertyValue, &textureHeader.AddressModeU, true))
            {
                Log_WarningPrintf("Texture Compiler Warning: Unknown value for %s (%s), using default.", Properties::AddressU, (pPropertyValue != NULL) ? pPropertyValue->GetCharArray() : "null");
                textureHeader.AddressModeU = TEXTURE_ADDRESS_MODE_CLAMP;
            }

            if ((pPropertyValue = m_propertyList.GetPropertyValuePointer(Properties::AddressV)) == NULL || !NameTable_TranslateType(NameTables::TextureAddressMode, *pPropertyValue, &textureHeader.AddressModeV, true))
            {
                Log_WarningPrintf("Texture Compiler Warning: Unknown value for %s (%s), using default.", Properties::AddressV, (pPropertyValue != NULL) ? pPropertyValue->GetCharArray() : "null");
                textureHeader.AddressModeV = TEXTURE_ADDRESS_MODE_CLAMP;
            }

            if ((pPropertyValue = m_propertyList.GetPropertyValuePointer(Properties::AddressW)) == NULL || !NameTable_TranslateType(NameTables::TextureAddressMode, *pPropertyValue, &textureHeader.AddressModeW, true))
            {
                Log_WarningPrintf("Texture Compiler Warning: Unknown value for %s (%s), using default.", Properties::AddressW, (pPropertyValue != NULL) ? pPropertyValue->GetCharArray() : "null");
                textureHeader.AddressModeW = TEXTURE_ADDRESS_MODE_CLAMP;
            }
        }

        if (m_bHasAlpha)
            textureHeader.BlendingMode = (isPremultipliedAlpha) ? MATERIAL_BLENDING_MODE_PREMULTIPLIED : MATERIAL_BLENDING_MODE_STRAIGHT;
        else
            textureHeader.BlendingMode = MATERIAL_BLENDING_MODE_NONE;

        textureHeader.MinLOD = 0;
        textureHeader.MaxLOD = (int32)m_nMipLevels;

        textureHeader.PixelFormat = (uint32)m_ePixelFormat;
        textureHeader.ArraySize = m_nArraySize;
        textureHeader.Width = m_iWidth;
        textureHeader.Height = m_iHeight;
        textureHeader.Depth = m_iDepth;
        textureHeader.MipLevels = m_nMipLevels;
        textureHeader.ImageCount = m_nImages;
        pOutputStream->Write2(&textureHeader, sizeof(textureHeader));
        currentOffset += sizeof(textureHeader);
        currentOffset += sizeof(uint32) * m_nImages;
    
        // determine and write offsets
        for (uint32 i = 0; i < m_nImages; i++)
        {
            const Image &img = m_pImages[i];
            pOutputStream->Write2(&currentOffset, sizeof(currentOffset));
            currentOffset += sizeof(DF_TEXTURE_IMAGE_HEADER);
            currentOffset += img.GetDataSize();
        }

        // write images
        for (uint32 i = 0; i < m_nImages; i++)
        {
            const Image &img = m_pImages[i];
            DF_TEXTURE_IMAGE_HEADER imageHeader;
            imageHeader.Size = img.GetDataSize();
            imageHeader.RowPitch = img.GetDataRowPitch();
            imageHeader.SlicePitch = img.GetDataSlicePitch();
            pOutputStream->Write2(&imageHeader, sizeof(imageHeader));
            pOutputStream->Write2(img.GetData(), img.GetDataSize());
        }
    }

    return !pOutputStream->InErrorState();
}

bool TextureGenerator::ImportDDS(const char *fileName, ByteStream *pStream)
{
    DDSReader ddsReader;
    if (!ddsReader.Open(fileName, pStream) || !ddsReader.LoadAllMipLevels())
    {
        Log_ErrorPrintf("TextureGenerator::ImportDDS: Failed to load DDS file.");
        return false;
    }

    TEXTURE_TYPE textureType;
    switch (ddsReader.GetType())
    {
    case DDS_TEXTURE_TYPE_1D:
        textureType = (ddsReader.GetArraySize() > 1) ? TEXTURE_TYPE_1D_ARRAY : TEXTURE_TYPE_1D;
        break;

    case DDS_TEXTURE_TYPE_2D:
        textureType = (ddsReader.GetArraySize() > 1) ? TEXTURE_TYPE_2D_ARRAY : TEXTURE_TYPE_2D;
        break;

    case DDS_TEXTURE_TYPE_3D:
        textureType = TEXTURE_TYPE_3D;
        break;

    case DDS_TEXTURE_TYPE_CUBE:
        textureType = (ddsReader.GetArraySize() > 6) ? TEXTURE_TYPE_CUBE_ARRAY : TEXTURE_TYPE_CUBE;
        break;

    default:
        Log_ErrorPrintf("TextureGenerator::ImportDDS: Invalid type.");
        return false;
    }

    DebugAssert(ddsReader.GetWidth() == 1 || Y_ispow2(ddsReader.GetWidth()));
    DebugAssert(ddsReader.GetHeight() == 1 || Y_ispow2(ddsReader.GetHeight()));
    DebugAssert(ddsReader.GetDepth() == 1 || Y_ispow2(ddsReader.GetDepth()));
    Log_InfoPrintf("TextureGenerator::LoadDDS: Source is a %u x %u x %u image with %u mip levels and %u array textures (%s)", ddsReader.GetWidth(), ddsReader.GetHeight(), ddsReader.GetDepth(), ddsReader.GetNumMipLevels(), ddsReader.GetArraySize(), NameTable_GetNameString(NameTables::PixelFormat, ddsReader.GetPixelFormat()));

    // create texture
    if (!Create(textureType, ddsReader.GetPixelFormat(), ddsReader.GetWidth(), ddsReader.GetHeight(), ddsReader.GetDepth(), ddsReader.GetNumMipLevels(), ddsReader.GetArraySize()))
    {
        Log_ErrorPrintf("TextureGenerator::ImportDDS: Could not create texture.");
        return false;
    }

    // get pfi
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(m_ePixelFormat);
    DebugAssert(pPixelFormatInfo != NULL);

    // add images
    for (uint32 i = 0; i < m_nArraySize; i++)
    {
        for (uint32 j = 0; j < m_nMipLevels; j++)
        {
            Image &destinationImage = m_pImages[i * m_nMipLevels + j];
            const byte *pMipLevelData = ddsReader.GetMipLevelData(i, j);
            uint32 mipPitch = ddsReader.GetMipPitch(i, j);

            // get number of rows
            uint32 rows = destinationImage.GetHeight();
            if (pPixelFormatInfo->IsBlockCompressed)
                rows = Max(rows / pPixelFormatInfo->BlockSize, (uint32)1);

            if (ddsReader.GetMipSize(i, j) < (mipPitch * rows))
            {
                Log_ErrorPrintf("TextureGenerator::ImportDDS: Invalid mip data size (%u, %u).", i, j);
                return false;
            }

            Y_memcpy_stride(destinationImage.GetData(), destinationImage.GetDataRowPitch(), pMipLevelData, mipPitch, Min(mipPitch, destinationImage.GetDataRowPitch()), rows);
        }
    }

    if (!UncompressImage())
        return false;

    if (!ConvertBGRToRGB())
        return false;

    if (!RemoveAlphaChannelIfOpaque())
        return false;

    return true;
}

bool TextureGenerator::ImportFromImage(const char *fileName, ByteStream *pStream)
{
    // check for .dds
    const char *extension = Y_strrchr(fileName, '.');
    if (extension != NULL && Y_stricmp(extension, ".dds") == 0)
        return ImportDDS(fileName, pStream);

    // find codec
    ImageCodec *pImageCodec = ImageCodec::GetImageCodecForStream(fileName, pStream);
    if (pImageCodec == NULL)
    {
        Log_ErrorPrintf("TextureGenerator::ImportFromImage: No image codec found.");
        return false;
    }

    Image decodedImage;
    if (!pImageCodec->DecodeImage(&decodedImage, fileName, pStream))
    {
        Log_ErrorPrintf("TextureGenerator::ImportFromImage: Failed to decode image.");
        return false;
    }

    return ImportFromImage(decodedImage);
}

bool TextureGenerator::ImportFromImage(const Image &image)
{
    if (!Create(TEXTURE_TYPE_2D, image.GetPixelFormat(), image.GetWidth(), image.GetHeight(), image.GetDepth(), 1))
    {
        Log_ErrorPrintf("TextureGenerator::ImportFromImage: Failed to create texture.");
        return false;
    }

    DebugAssert(m_nImages == 1);
    m_pImages[0].Copy(image);

    if (!UncompressImage())
        return false;

    if (!ConvertBGRToRGB())
        return false;

    if (!RemoveAlphaChannelIfOpaque())
        return false;

    return true;
}


/*
bool TextureGenerator::Load2DImage(const char *FileName, ByteStream *pStream)
{
    DebugAssert(m_pImages == NULL);



    // setup everything
    m_eTextureType = TEXTURE_TYPE_2D;
    m_eTextureUsage = TEXTURE_USAGE_NONE;
    m_eTextureFilter = TEXTURE_FILTER_COUNT;
    m_ePixelFormat = decodedImage.GetPixelFormat();
    m_nMipLevels = 1;
    m_nArraySize = 1;
    m_iWidth = decodedImage.GetWidth();
    m_iHeight = decodedImage.GetHeight();
    m_iDepth = decodedImage.GetDepth();

    // set defaults here too, but they'll be replaced
    m_bHasAlpha = false;
    m_bHasAlphaLevels = false;

    // setup image array
    m_nImages = 1;
    m_pImages = new Image[m_nImages];
    m_pImages[0].Copy(decodedImage);

    if (!AnalyseImage())
    {
        Log_WarningPrintf("TextureGenerator::Load: Failed to analyse image.");
        return false;
    }

    return true;
}
*/

// Interface
BinaryBlob *ResourceCompiler::CompileTexture(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name)
{
    SmallString textureSourceFileName;
    textureSourceFileName.Format("%s.tex.zip", name);

    BinaryBlob *pTextureSource = pCallbacks->GetFileContents(textureSourceFileName);
    if (pTextureSource == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileTexture: Failed to read '%s'", textureSourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pTextureSource->GetDataPointer(), pTextureSource->GetDataSize());
    TextureGenerator *pGenerator = new TextureGenerator();
    if (!pGenerator->Load(textureSourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pTextureSource->Release();
        return nullptr;
    }

    pStream->Release();
    pTextureSource->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pOutputStream, platform))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
