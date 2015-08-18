#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/TextureImporter.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "Engine/Common.h"
#include "ResourceCompiler/TextureGenerator.h"

TextureImporter::TextureImporter(ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(NULL),
      m_pGenerator(NULL)
{

}

TextureImporter::~TextureImporter()
{
    delete m_pGenerator;

    for (uint32 i = 0; i < m_sourceImages.GetSize(); i++)
        delete m_sourceImages[i];
}

void TextureImporter::SetDefaultOptions(TextureImporterOptions *pOptions)
{
    pOptions->AppendTexture = false;
    pOptions->RebuildTexture = false;
    pOptions->Type = TEXTURE_TYPE_2D;
    pOptions->Usage = TEXTURE_USAGE_NONE;
    pOptions->Filter = TEXTURE_FILTER_ANISOTROPIC;
    pOptions->AddressU = TEXTURE_ADDRESS_MODE_CLAMP;
    pOptions->AddressV = TEXTURE_ADDRESS_MODE_CLAMP;
    pOptions->AddressW = TEXTURE_ADDRESS_MODE_CLAMP;
    pOptions->ResizeWidth = 0;
    pOptions->ResizeHeight = 0;
    pOptions->ResizeFilter = IMAGE_RESIZE_FILTER_LANCZOS3;
    pOptions->GenerateMipMaps = true;
    pOptions->SourcePremultipliedAlpha = false;
    pOptions->EnablePremultipliedAlpha = true;
    pOptions->EnableTextureCompression = true;
    pOptions->OutputFormat = PIXEL_FORMAT_UNKNOWN;
    pOptions->Optimize = false;
    pOptions->Compile = false;
}

bool TextureImporter::HasTextureFileExtension(const char *fileName)
{
    const char *extension = Y_strrchr(fileName, '.');
    if (extension == NULL)
        return false;

    static const char *textureExtensions[] =
    {
        "bmp",
        "jpg",
        "jpeg",
        "png",
        "tga",
        "dds",
    };

    // remove '.'
    extension++;

    // compare against known
    uint32 i;
    for (i = 0; i < countof(textureExtensions); i++)
    {
        if (Y_stricmp(extension, textureExtensions[i]) == 0)
            return true;
    }

    return false;
}

bool TextureImporter::Execute(const TextureImporterOptions *pOptions)
{
    Timer timer;
    bool result = false;

    if ((!pOptions->RebuildTexture && pOptions->InputFileNames.GetSize() == 0) ||
        (pOptions->InputMaskFileNames.GetSize() > 0 && pOptions->InputMaskFileNames.GetSize() != pOptions->InputFileNames.GetSize()) ||
        pOptions->OutputName.IsEmpty())
    {
        m_pProgressCallbacks->ModalError("One or more required parameters not set.");
        return false;
    }

    if ((pOptions->ResizeWidth == 0 && pOptions->ResizeHeight != 0) ||
        (pOptions->ResizeWidth != 0 && pOptions->ResizeHeight == 0))
    {
        m_pProgressCallbacks->ModalError("Invalid resize dimensions");
        return false;
    }

    // store options
    m_pOptions = pOptions;

    if (!m_pOptions->RebuildTexture && !LoadSources())
    {
        m_pProgressCallbacks->ModalError("LoadSources() failed. The log may have more information.");
        goto EXIT;
    }

    m_pProgressCallbacks->DisplayFormattedInformation("LoadSources(): %.4f msec", timer.GetTimeMilliseconds());  

    if ((m_pOptions->AppendTexture | m_pOptions->RebuildTexture))
    {
        timer.Reset();
        if (!LoadGenerator())
        {
            m_pProgressCallbacks->ModalError("LoadGenerator() failed. The log may have more information.");
            goto EXIT;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("LoadGenerator(): %.4f msec", timer.GetTimeMilliseconds());

        timer.Reset();
        if (m_pOptions->AppendTexture && !AppendImages())
        {
            m_pProgressCallbacks->ModalError("AppendImages() failed. The log may have more information.");
            goto EXIT;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("AppendImages(): %.4f msec", timer.GetTimeMilliseconds());
    }
    else
    {
        timer.Reset();
        if (!CreateGenerator())
        {
            m_pProgressCallbacks->ModalError("CreateGenerator() failed. The log may have more information.");
            goto EXIT;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("CreateGenerator(): %.4f msec", timer.GetTimeMilliseconds());
    }

    timer.Reset();
    if (!ResizeAndSetProperties())
    {
        m_pProgressCallbacks->ModalError("ResizeAndSetProperties() failed. The log may have more information.");
        goto EXIT;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("ResizeAndSetProperties(): %.4f msec", timer.GetTimeMilliseconds());

    timer.Reset();
    if (m_pOptions->Optimize && !m_pGenerator->Optimize())
    {
        m_pProgressCallbacks->ModalError("Optimize() failed. The log may have more information.");
        goto EXIT;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("Optimize(): %.4f msec", timer.GetTimeMilliseconds());
   
    timer.Reset();
    if (!WriteOutput())
    {
        m_pProgressCallbacks->ModalError("WriteOutput() failed. The log may have more information.");
        goto EXIT;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("WriteOutput(): %.4f msec", timer.GetTimeMilliseconds());

    timer.Reset();
    if (m_pOptions->Compile && !WriteCompiledOutput())
    {
        m_pProgressCallbacks->ModalError("WriteCompiledOutput() failed. The log may have more information.");
        goto EXIT;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("WriteCompiledOutput(): %.4f msec", timer.GetTimeMilliseconds());

    // flag success
    result = true;

EXIT:
    m_pOptions = NULL;
    delete m_pGenerator;
    m_pGenerator = NULL;
    for (uint32 i = 0; i < m_sourceImages.GetSize();i++)
        delete m_sourceImages[i];
    m_sourceImages.Clear();

    return result;
}

bool TextureImporter::LoadSources()
{
    PathString osFileName;

    for (uint32 i = 0; i < m_pOptions->InputFileNames.GetSize(); i++)
    {
        FileSystem::CanonicalizePath(osFileName, m_pOptions->InputFileNames[i], true);

        ByteStream *pStream = FileSystem::OpenFile(osFileName, BYTESTREAM_OPEN_READ);
        if (pStream == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to open file '%s'", osFileName.GetCharArray());
            return false;
        }

        ImageCodec *pImageCodec = ImageCodec::GetImageCodecForStream(osFileName, pStream);
        if (pImageCodec == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to find codec for '%s'", osFileName.GetCharArray());
            return false;
        }

        Image *pSourceImage = new Image();
        if (!pImageCodec->DecodeImage(pSourceImage, osFileName, pStream))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to decode image '%s'", osFileName.GetCharArray());
            delete pSourceImage;
            return false;
        }

        m_pProgressCallbacks->DisplayFormattedInformation("Source loaded: %u x %u x %u [%s]", pSourceImage->GetWidth(), pSourceImage->GetHeight(), pSourceImage->GetDepth(), NameTable_GetNameString(NameTables::PixelFormat, pSourceImage->GetPixelFormat()));
        pStream->Release();

        // uncompress it
        const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pSourceImage->GetPixelFormat());
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            m_pProgressCallbacks->DisplayFormattedInformation("Decompressing image from %s to %s", pPixelFormatInfo->Name, PixelFormat_GetPixelFormatInfo(pPixelFormatInfo->UncompressedFormat)->Name);
            if (!pSourceImage->ConvertPixelFormat(pPixelFormatInfo->UncompressedFormat))
            {
                m_pProgressCallbacks->DisplayFormattedError("Failed to decompress image.", m_pOptions->InputFileNames[i].GetCharArray());
                delete pSourceImage;
                return false;
            }
        }

        if (i > 0 && pSourceImage->GetPixelFormat() != m_sourceImages[0]->GetPixelFormat() && !pSourceImage->ConvertPixelFormat(m_sourceImages[0]->GetPixelFormat()))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to convert '%s' to common pixel format.", m_pOptions->InputFileNames[i].GetCharArray());
            delete pSourceImage;
            return false;
        }

        if (m_pOptions->InputMaskFileNames.GetSize() > 0)
        {
            DebugAssert(i < m_pOptions->InputMaskFileNames.GetSize());

            FileSystem::CanonicalizePath(osFileName, m_pOptions->InputFileNames[i], true);
            m_pProgressCallbacks->DisplayFormattedInformation("Adding mask from file '%s'", osFileName.GetCharArray());

            pStream = FileSystem::OpenFile(osFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
            if (pStream == NULL)
            {
                m_pProgressCallbacks->DisplayFormattedError("Failed to open mask file '%s'", osFileName.GetCharArray());
                delete pSourceImage;
                return false;
            }

            pImageCodec = ImageCodec::GetImageCodecForStream(osFileName, pStream);
            if (pImageCodec == NULL)
            {
                m_pProgressCallbacks->DisplayFormattedError("Failed to find codec for mask file '%s'", osFileName.GetCharArray());
                pStream->Release();
                delete pSourceImage;
                return false;
            }

            Image maskImage;
            if (!pImageCodec->DecodeImage(&maskImage, osFileName, pStream))
            {
                m_pProgressCallbacks->DisplayFormattedError("Failed to decode mask file '%s'", osFileName.GetCharArray());
                pStream->Release();
                delete pSourceImage;
                return false;
            }

            const PIXEL_FORMAT_INFO *pMaskPixelFormatInfo = PixelFormat_GetPixelFormatInfo(maskImage.GetPixelFormat());
            m_pProgressCallbacks->DisplayFormattedInformation("Loaded mask file '%s' (%ux%u %s)", osFileName.GetCharArray(), maskImage.GetWidth(), maskImage.GetHeight(), pMaskPixelFormatInfo->Name);
            pStream->Release();

            if (maskImage.GetWidth() != pSourceImage->GetWidth() || maskImage.GetHeight() != pSourceImage->GetHeight())
            {
                m_pProgressCallbacks->DisplayFormattedError("Mask image and source image size mismatch.");
                delete pSourceImage;
                return false;
            }

            // convert the input pixels to something useful
            if (!pSourceImage->ConvertPixelFormat(PIXEL_FORMAT_R8G8B8A8_UNORM))
            {
                delete pSourceImage;
                return false;
            }

            // does it have alpha?
            if (pMaskPixelFormatInfo->HasAlpha)
            {
                // convert to something useful
                if (maskImage.GetPixelFormat() != PIXEL_FORMAT_R8G8B8A8_UNORM && !maskImage.ConvertPixelFormat(PIXEL_FORMAT_R8G8B8A8_UNORM))
                {
                    m_pProgressCallbacks->DisplayFormattedError("Failed to convert mask file to RGBA8 format");
                    delete pSourceImage;
                    return false;
                }

                // take the alpha channel from each pixel of the mask and apply it to the source
                for (uint32 y = 0; y < pSourceImage->GetHeight(); y++)
                {
                    const byte *pMaskPixels = maskImage.GetData() + (y * maskImage.GetDataRowPitch());
                    byte *pPixels = pSourceImage->GetData() + (y * pSourceImage->GetDataRowPitch());
                    for (uint32 x = 0; x < pSourceImage->GetWidth(); x++)
                    {
                        pPixels[3] = pMaskPixels[3];
                        pMaskPixels += 4;
                        pPixels += 4;
                    }
                }
            }
            else
            {
                // convert the image to R8
                if (maskImage.GetPixelFormat() != PIXEL_FORMAT_R8_UNORM && !maskImage.ConvertPixelFormat(PIXEL_FORMAT_R8_UNORM))
                {
                    m_pProgressCallbacks->DisplayFormattedError("Failed to convert mask file to R8 format");
                    delete pSourceImage;
                    return false;
                }

                // take the red channel from each pixel of the mask and apply it to the source's alpha channel
                for (uint32 y = 0; y < pSourceImage->GetHeight(); y++)
                {
                    const byte *pMaskPixels = maskImage.GetData() + (y * maskImage.GetDataRowPitch());
                    byte *pPixels = pSourceImage->GetData() + (y * pSourceImage->GetDataRowPitch());
                    for (uint32 x = 0; x < pSourceImage->GetWidth(); x++)
                    {
                        pPixels[3] = pMaskPixels[0];
                        pMaskPixels++;
                        pPixels += 4;
                    }
                }
            }
        }

        m_sourceImages.Add(pSourceImage);
    }

    // handle resizes
    {
        bool differingSizes = false;
        for (uint32 i = 1; i < m_sourceImages.GetSize(); i++)
        {
            if (m_sourceImages[i]->GetWidth() != m_sourceImages[0]->GetWidth() ||
                m_sourceImages[i]->GetHeight() != m_sourceImages[0]->GetHeight())
            {
                differingSizes = true;
                break;
            }
        }
        if (differingSizes && m_pOptions->ResizeWidth)
        {
            m_pProgressCallbacks->DisplayFormattedError("Sizes differ between images and no resize dimensions were specified");
            return false;
        }

        if (m_pOptions->ResizeWidth != 0)
        {
            for (uint32 i = 0; i < m_sourceImages.GetSize(); i++)
            {
                Image *pSourceImage = m_sourceImages[i];
                if (!pSourceImage->Resize(m_pOptions->ResizeFilter, m_pOptions->ResizeWidth, m_pOptions->ResizeHeight, 1))
                {
                    m_pProgressCallbacks->DisplayFormattedError("Resize failed");
                    return false;
                }
            }
        }
    }

    return true;
}

bool TextureImporter::LoadGenerator()
{
    PathString outputFileName;
    FileSystem::CanonicalizePath(outputFileName, m_pOptions->OutputName, true);
    outputFileName.AppendString(".tex.zip");

    m_pProgressCallbacks->DisplayFormattedInformation("Loading texture: '%s'", outputFileName.GetCharArray());

    ByteStream *pStream = FileSystem::OpenFile(outputFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to open '%s'", outputFileName.GetCharArray());
        return false;
    }

    m_pGenerator = new TextureGenerator();
    if (!m_pGenerator->Load(outputFileName, pStream))
    {
        pStream->Release();
        m_pProgressCallbacks->DisplayFormattedError("Failed to load '%s'", outputFileName.GetCharArray());
        return false;
    }

    pStream->Release();
    return true;
}

bool TextureImporter::CreateGenerator()
{
    TEXTURE_TYPE textureType = m_pOptions->Type;
    uint32 arraySize = 1;

    if (textureType == TEXTURE_TYPE_2D && m_sourceImages.GetSize() > 1)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("Changing 2D texture to 2D array texture as there is more than one image");
        textureType = TEXTURE_TYPE_2D_ARRAY;
        arraySize = m_sourceImages.GetSize();
    }
    else if (textureType == TEXTURE_TYPE_2D_ARRAY)
    {
        // okay, nothing to do
        arraySize = m_sourceImages.GetSize();
    }
    else if (textureType == TEXTURE_TYPE_CUBE)
    {
        if (m_sourceImages.GetSize() != 6)
        {
            m_pProgressCallbacks->DisplayFormattedError("Cube map textures must have 6 images");
            return false;
        }

        arraySize = m_sourceImages.GetSize();
    }
    else if (textureType == TEXTURE_TYPE_CUBE_ARRAY)
    {
        if ((m_sourceImages.GetSize() % 6) != 0)
        {
            m_pProgressCallbacks->DisplayFormattedError("Cube map textures must have a multiple of 6 images");
            return false;
        }

        arraySize = m_sourceImages.GetSize();
    }

    // create generator
    m_pGenerator = new TextureGenerator();
    if (!m_pGenerator->Create(textureType, m_sourceImages[0]->GetPixelFormat(), m_sourceImages[0]->GetWidth(), m_sourceImages[0]->GetHeight(), m_sourceImages[0]->GetDepth(), arraySize, m_pOptions->Usage))
    {
        m_pProgressCallbacks->DisplayFormattedError("Failed to create generator");
        return false;
    }

    for (uint32 i = 0; i < m_sourceImages.GetSize(); i++)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("setting image %u %ux%u %s", i, m_sourceImages[i]->GetWidth(), m_sourceImages[i]->GetHeight(), PixelFormat_GetPixelFormatInfo(m_sourceImages[i]->GetPixelFormat())->Name);
        if (!m_pGenerator->SetImage(i, m_sourceImages[i]))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to set image %u in generator", i);
            return false;
        }
    }

    return true;
}

bool TextureImporter::AppendImages()
{
    // resize to generator size
    for (uint32 i = 0; i < m_sourceImages.GetSize(); i++)
    {
        Image *pSourceImage = m_sourceImages[i];
        m_pProgressCallbacks->DisplayFormattedInformation("Appending source image %u...", i);

        if (pSourceImage->GetPixelFormat() != m_pGenerator->GetPixelFormat())
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to convert source image %u from %s to %s", i, PixelFormat_GetPixelFormatInfo(pSourceImage->GetPixelFormat())->Name, PixelFormat_GetPixelFormatInfo(m_pGenerator->GetPixelFormat())->Name);
            return false;
        }

        if ((pSourceImage->GetWidth() != m_pGenerator->GetWidth() || pSourceImage->GetHeight() != m_pGenerator->GetHeight()) &&
            !pSourceImage->Resize(m_pOptions->ResizeFilter, m_pGenerator->GetWidth(), m_pGenerator->GetHeight(), 1))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to resize source image %u", i);
            return false;
        }

        if (!m_pGenerator->AddImage(pSourceImage))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to add source image %u", i);
            return false;
        }
    }

    return true;
}

bool TextureImporter::ResizeAndSetProperties()
{
    // set properties
    m_pGenerator->SetTextureUsage(m_pOptions->Usage);
    m_pGenerator->SetTextureFilter(m_pOptions->Filter);
    m_pGenerator->SetTextureAddressModeU(m_pOptions->AddressU);
    m_pGenerator->SetTextureAddressModeV(m_pOptions->AddressV);
    m_pGenerator->SetTextureAddressModeW(m_pOptions->AddressW);
    m_pGenerator->SetMipMapResizeFilter(m_pOptions->ResizeFilter);
    m_pGenerator->SetGenerateMipmaps(m_pOptions->GenerateMipMaps);
    m_pGenerator->SetSourcePremultipliedAlpha(m_pOptions->SourcePremultipliedAlpha);
    m_pGenerator->SetEnablePremultipliedAlpha(m_pOptions->EnablePremultipliedAlpha);
    m_pGenerator->SetEnableTextureCompression(m_pOptions->EnableTextureCompression);

    // convert formats
    if (m_pOptions->OutputFormat != PIXEL_FORMAT_UNKNOWN)
    {
        if (!m_pGenerator->ConvertToPixelFormat(m_pOptions->OutputFormat))
        {
            m_pProgressCallbacks->DisplayFormattedError("failed to convert to pixel format %s", PixelFormat_GetPixelFormatInfo(m_pOptions->OutputFormat)->Name);
            return false;
        }
    }

    // do resize
    if (m_pOptions->ResizeWidth != 0)
    {
        if (m_pGenerator->GetWidth() != m_pOptions->ResizeWidth || m_pGenerator->GetHeight() != m_pOptions->ResizeHeight)
        {
            m_pProgressCallbacks->DisplayFormattedInformation("Resizing to %u x %u", m_pOptions->ResizeWidth, m_pOptions->ResizeHeight);
            if (!m_pGenerator->Resize(m_pOptions->ResizeFilter, m_pOptions->ResizeWidth, m_pOptions->ResizeHeight, 1))
                return false;
        }
    }

    return true;
}

bool TextureImporter::WriteOutput()
{
    PathString outputFileName;
    FileSystem::CanonicalizePath(outputFileName, m_pOptions->OutputName, true);
    outputFileName.AppendString(".tex.zip");

    m_pProgressCallbacks->DisplayFormattedInformation("Output filename: '%s'", outputFileName.GetCharArray());

    ByteStream *pOutputStream;
    if ((pOutputStream = g_pVirtualFileSystem->OpenFile(outputFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE)) == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not open output file '%s'", outputFileName.GetCharArray());
        return false;
    }

    bool result = m_pGenerator->Save(pOutputStream);
    if (!result)
        pOutputStream->Discard();
    else
        pOutputStream->Commit();

    pOutputStream->Release();
    return result;
}

bool TextureImporter::WriteCompiledOutput()
{
    PathString outputFileName;
    FileSystem::CanonicalizePath(outputFileName, m_pOptions->OutputName, true);
    outputFileName.AppendString(".tex");

    m_pProgressCallbacks->DisplayFormattedInformation("Compiled output filename: '%s'", outputFileName.GetCharArray());

    ByteStream *pOutputStream;
    if ((pOutputStream = g_pVirtualFileSystem->OpenFile(outputFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE)) == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not open output file '%s'", outputFileName.GetCharArray());
        return false;
    }

    bool result = m_pGenerator->Compile(pOutputStream);
    if (!result)
        pOutputStream->Discard();
    else
        pOutputStream->Commit();

    pOutputStream->Release();
    return result;
}
