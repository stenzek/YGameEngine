#include "Core/PrecompiledHeader.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Log.h"
//Log_SetChannel(ImageCodecJPEG);

#if 0

#include <cstdio>
#include <jpeglib.h>
#include <jversion.h>
#pragma comment(lib, "libjpeg.lib")

class ImageCodecJPEG : public ImageCodec
{
public:
    // Returns the name of the image codec
    const char *GetCodecName() const
    {
        return "libjpeg " JVERSION " " JCOPYRIGHT;
    }
    
    // Decodes the image.
    bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyList &decoderOptions = PropertyList::EmptyPropertyList)
    {
        // allocate memory and read in the stream
        uint32 CompressedDataSize = (uint32)pInputStream->GetSize();
        uint32 BytesRead;
        byte *pCompressedData = (byte *)Y_malloc(CompressedDataSize);
        if ((BytesRead = pInputStream->Read(pCompressedData, CompressedDataSize)) != CompressedDataSize)
        {
            Log_ErrorPrintf("ImageCodecJPEG::DecodeImage: Only read %u of %u bytes.", BytesRead, CompressedDataSize);
            Y_free(pCompressedData);
            return false;
        }

        // todo custom error handler
        jpeg_decompress_struct DecompressStruct;
        jpeg_error_mgr ErrorMgr;
        DecompressStruct.err = jpeg_std_error(&ErrorMgr);
        jpeg_create_decompress(&DecompressStruct);
        jpeg_mem_src(&DecompressStruct, pCompressedData, CompressedDataSize);

        bool Result = false;
        if (jpeg_read_header(&DecompressStruct, TRUE) == JPEG_HEADER_OK)
        {
            if (jpeg_start_decompress(&DecompressStruct) == TRUE)
            {
                // set up the image
                uint32 Width = DecompressStruct.output_width;
                uint32 Height = DecompressStruct.output_height;
                if (DecompressStruct.out_color_space != JCS_RGB)
                {
                    Log_ErrorPrintf("ImageCodecJPEG::DecodeImage: Unknown colorspace %u.", DecompressStruct.out_color_space);
                }
                else
                {
                    pOutputImage->Create(PIXEL_FORMAT_R8G8B8_UNORM, Width, Height);
                    byte *pPixels = (byte *)pOutputImage->GetData();
                    uint32 i;
                    for (i = 0; i < Height; i++)
                    {
                        if (jpeg_read_scanlines(&DecompressStruct, (JSAMPARRAY)&pPixels, 1) != 1)
                            break;

                        pPixels += Width * 3;
                    }

                    if (i != Height)
                    {
                        Log_ErrorPrintf("ImageCodecJPEG::DecodeImage: Only decoded %u of %u scan lines.", i, Height);
                        pOutputImage->Delete();
                    }
                    else
                    {
                        Result = true;
                    }
                }
            }

            jpeg_finish_decompress(&DecompressStruct);
        }

        // free up the jpeg and image data
        jpeg_destroy_decompress(&DecompressStruct);
        Y_free(pCompressedData);

        // return result
        return Result;
    }

    // Encodes the image.
    bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyList &encoderOptions = PropertyList::EmptyPropertyList)
    {
        return false;
    }
};

static ImageCodecJPEG g_ImageCodecJPEG;

ImageCodec *__GetImageCodecJPEG()
{
    return &g_ImageCodecJPEG;
}

#endif
