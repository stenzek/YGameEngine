#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/Texture.h"
#include "Core/Image.h"
#include "Core/PropertyTable.h"

class TextureGenerator
{
    friend class Texture;
    friend class Texture2D;
    friend class Texture2DArray;
    friend class TextureCube;

public:
    class Properties
    {
    public:
        static const char *Usage;
        static const char *SourceSRGB;
        static const char *SourcePremultipliedAlpha;
        static const char *MaxFilter;
        static const char *AddressU;
        static const char *AddressV;
        static const char *AddressW;
        static const char *BorderColor;
        static const char *GenerateMipmaps;
        static const char *MipmapResizeFilter;
        static const char *NPOTMipmaps;
        static const char *EnableSRGB;
        static const char *EnablePremultipliedAlpha;
        static const char *EnableTextureCompression;
    };

public:
    TextureGenerator();
    TextureGenerator(const TextureGenerator &copy);
    ~TextureGenerator();

    const TEXTURE_TYPE GetTextureType() const { return m_eTextureType; }
    const TEXTURE_PLATFORM GetTexturePlatform() const { return m_eTexturePlatform; }
    const PIXEL_FORMAT GetPixelFormat() const { return m_ePixelFormat; }
    const uint32 GetMipLevels() const { return m_nMipLevels; }
    const uint32 GetArraySize() const { return m_nArraySize; }
    const uint32 GetWidth() const { return m_iWidth; }
    const uint32 GetHeight() const { return m_iHeight; }
    const uint32 GetDepth() const { return m_iDepth; }

    const Image *GetImage(uint32 arrayIndex, uint32 mipIndex) const;

    bool Create(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 arraySize, TEXTURE_USAGE usage = TEXTURE_USAGE_NONE);
    bool Create(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 mipLevels, uint32 arraySize, TEXTURE_USAGE usage = TEXTURE_USAGE_NONE);

    bool Load(const char *fileName, ByteStream *pStream);
    bool Save(ByteStream *pOutputStream) const;

    bool Resize(IMAGE_RESIZE_FILTER resizeFilter, uint32 width, uint32 height, uint32 depth);
    bool Optimize();

    bool Compile(ByteStream *pOutputStream, TEXTURE_PLATFORM platform = NUM_TEXTURE_PLATFORMS) const;

    void SetTexturePlatform(TEXTURE_PLATFORM platform);
    void SetTextureUsage(TEXTURE_USAGE usage);
    void SetTextureFilter(TEXTURE_FILTER filter);
    void SetTextureAddressModeU(TEXTURE_ADDRESS_MODE mode);
    void SetTextureAddressModeV(TEXTURE_ADDRESS_MODE mode);
    void SetTextureAddressModeW(TEXTURE_ADDRESS_MODE mode);

    void SetMipMapResizeFilter(IMAGE_RESIZE_FILTER filter);
    void SetGenerateMipmaps(bool enabled);
    void SetSourcePremultipliedAlpha(bool enabled);
    void SetEnablePremultipliedAlpha(bool enabled);
    void SetEnableTextureCompression(bool enabled);
    void SetEnableSRGB(bool enabled);
    void SetSourceSRGB(bool enabled);

    bool AddMask(uint32 arrayIndex, const Image *pMaskImage);

    bool AddImage(const Image *pImage);
    bool SetImage(uint32 arrayIndex, const Image *pImage);
    bool SetImage(uint32 arrayIndex, uint32 mipIndex, const Image *pImage);
    void SetArraySize(uint32 newArraySize);

    bool AnalyzeImage();
    PIXEL_FORMAT GetBestPixelFormat(bool allowCompression) const;
    bool GenerateMipmaps();
    bool ConvertToPremultipliedAlpha();
    bool ConvertToPixelFormat(PIXEL_FORMAT newPixelFormat);
    bool UncompressImage();
    bool RemoveAlphaChannel();
    bool RemoveAlphaChannelIfOpaque();
    bool ConvertBGRToRGB();
    bool ConvertToTextureArray();

    bool ImportDDS(const char *fileName, ByteStream *pStream);
    bool ImportFromImage(const char *fileName, ByteStream *pStream);
    bool ImportFromImage(const Image &image);

private:
    bool InternalCreate(TEXTURE_TYPE textureType, PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 mipLevels, uint32 arraySize);
    void SetDefaultProperties(TEXTURE_USAGE usage);
    bool InternalCompile(ByteStream *pOutputStream);

    TEXTURE_TYPE m_eTextureType;
    TEXTURE_PLATFORM m_eTexturePlatform;
    PropertyTable m_propertyList;

    // image details
    PIXEL_FORMAT m_ePixelFormat;
    uint32 m_nMipLevels;
    uint32 m_nArraySize;

    // discovered via analysis
    bool m_analyzed;
    bool m_bHasAlpha;
    bool m_bHasAlphaLevels;

    // mirrors "level 0" mip
    uint32 m_iWidth;
    uint32 m_iHeight;
    uint32 m_iDepth;

    // image array
    Image *m_pImages;
    uint32 m_nImages;
};

