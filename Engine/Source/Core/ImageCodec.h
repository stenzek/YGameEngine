#pragma once
#include "Core/Common.h"
#include "Core/PixelFormat.h"
#include "Core/Image.h"
#include "Core/PropertyTable.h"
#include "YBaseLib/ByteStream.h"

namespace ImageCodecEncoderOptions
{
    extern const char *JPEG_QUALITY_LEVEL;
    extern const char *PNG_COMPRESSION_LEVEL;
}

namespace ImageCodecDecoderOptions
{

}

class ImageCodec
{
public:
    // Returns the name of the image codec
    virtual const char *GetCodecName() const = 0;

    // Decodes the image.
    virtual bool DecodeImage(Image *pOutputImage, const char *FileName, ByteStream *pInputStream, const PropertyTable &decoderOptions = PropertyTable::EmptyPropertyList) = 0;

    // Encodes the image.
    virtual bool EncodeImage(const char *FileName, ByteStream *pOutputStream, const Image *pInputImage, const PropertyTable &encoderOptions = PropertyTable::EmptyPropertyList) = 0;

    // Codec getters
    static ImageCodec *GetImageCodecForFileName(const char *fileName);
    static ImageCodec *GetImageCodecForStream(const char *fileName, ByteStream *pStream);
};

