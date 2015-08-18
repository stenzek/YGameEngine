#pragma once
#include "Core/Resource.h"
#include "Renderer/RendererTypes.h"

class Image;
class GPUTexture;
class GPUSamplerState;

class Texture : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(Texture, Resource);
    DECLARE_RESOURCE_NO_FACTORY(Texture);

    struct ImageData
    {
        uint32 Size;
        uint32 RowPitch;
        uint32 SlicePitch;
        uint32 FileOffset;
        byte *pPixels;
    };

public:
    Texture(TEXTURE_TYPE TextureType, const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    virtual ~Texture();

    static TEXTURE_TYPE GetTextureTypeForStream(const char *FileName, ByteStream *pStream);
    static Texture *CreateTextureObjectForType(TEXTURE_TYPE textureType);
    static const ResourceTypeInfo *GetResourceTypeInfoForTextureType(TEXTURE_TYPE textureType);
    static uint3 GetTextureDimensions(const Texture *pTexture);

    const TEXTURE_TYPE GetTextureType() const { return m_eTextureType; }
    const TEXTURE_PLATFORM GetTexturePlatform() const { return m_eTexturePlatform; }
    const TEXTURE_FILTER GetTextureFilter() const { return m_eTextureFilter; }
    const PIXEL_FORMAT GetPixelFormat() const { return m_ePixelFormat; }
    const MATERIAL_BLENDING_MODE GetBlendingMode() const { return m_eBlendingMode; }
    uint32 GetNumMipLevels() const { return m_nMipLevels; }

    virtual bool Load(const char *FileName, ByteStream *pInputStream) = 0;

    GPUTexture *GetGPUTexture() const;
    virtual bool CreateDeviceResources() const;
    virtual void ReleaseDeviceResources() const;

protected:
    TEXTURE_TYPE m_eTextureType;
    TEXTURE_PLATFORM m_eTexturePlatform;
    PIXEL_FORMAT m_ePixelFormat;
    TEXTURE_USAGE m_eTextureUsage;
    TEXTURE_FILTER m_eTextureFilter;
    MATERIAL_BLENDING_MODE m_eBlendingMode;
    int32 m_iMinLOD;
    int32 m_iMaxLOD;
    uint32 m_nMipLevels;
    uint32 m_nImages;
    ImageData *m_pImages;

    mutable GPUTexture *m_pDeviceTexture;
    mutable bool m_bDeviceResourcesCreated;
};

class Texture2D : public Texture
{
    DECLARE_RESOURCE_TYPE_INFO(Texture2D, Texture);
    DECLARE_RESOURCE_GENERIC_FACTORY(Texture2D);

public:
    Texture2D(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~Texture2D();

    const TEXTURE_ADDRESS_MODE GetAddressModeU() const { return m_eAddressModeU; }
    const TEXTURE_ADDRESS_MODE GetAddressModeV() const { return m_eAddressModeV; }
    const uint32 GetWidth() const { return m_iWidth; }
    const uint32 GetHeight() const { return m_iHeight; }

    // manually create a texture resource. the image data will only contain junk until it is properly filled.
    bool Create(const char *Name, TEXTURE_PLATFORM texturePlatform, TEXTURE_USAGE textureUsage, TEXTURE_FILTER textureFilter, MATERIAL_BLENDING_MODE blendingMode,
                TEXTURE_ADDRESS_MODE addressModeU, TEXTURE_ADDRESS_MODE addressModeV, int32 minLOD, int32 maxLOD,
                PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 mipLevels);

    bool Load(const char *name, ByteStream *pStream);

    // mip level accessor
    const ImageData *GetMipLevelData(uint32 mipIndex) const { DebugAssert(mipIndex < m_nMipLevels); return &m_pImages[mipIndex]; }
    ImageData *GetMipLevelData(uint32 mipIndex) { DebugAssert(mipIndex < m_nMipLevels); return &m_pImages[mipIndex]; }

    // device resources
    virtual bool CreateDeviceResources() const;
    virtual void ReleaseDeviceResources() const;

    // texture sampling/loading
    bool GetTexelLevel(uint32 x, uint32 y, uint32 mipLevel, SHADER_PARAMETER_TYPE outType, void *pOutValue);
    bool SampleTexelLevel(float u, float v, uint32 mipLevel, SHADER_PARAMETER_TYPE outType, void *pOutValue, TEXTURE_FILTER overrideFilter = TEXTURE_FILTER_COUNT);

    // calculate the U coordinate from this texture
    float GetUCoordinate(int32 pos) const { return (float)pos / (float)m_iWidth; }
    float GetVCoordinate(int32 pos) const { return (float)pos / (float)m_iHeight; }

    // exporting to an image
    bool ExportToImage(uint32 mipIndex, Image *pOutputImage) const;

protected:
    TEXTURE_ADDRESS_MODE m_eAddressModeU, m_eAddressModeV;

    uint32 m_iWidth, m_iHeight;
};

class Texture2DArray : public Texture
{
    DECLARE_RESOURCE_TYPE_INFO(Texture2DArray, Texture);
    DECLARE_RESOURCE_GENERIC_FACTORY(Texture2DArray);

public:
    Texture2DArray(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~Texture2DArray();

    const TEXTURE_ADDRESS_MODE GetAddressModeU() const { return m_eAddressModeU; }
    const TEXTURE_ADDRESS_MODE GetAddressModeV() const { return m_eAddressModeV; }
    const uint32 GetWidth() const { return m_iWidth; }
    const uint32 GetHeight() const { return m_iHeight; }
    const uint32 GetArraySize() const { return m_iArraySize; }

    // manually create a texture resource. the image data will only contain junk until it is properly filled.
    bool Create(const char *Name, TEXTURE_PLATFORM texturePlatform, TEXTURE_USAGE textureUsage, TEXTURE_FILTER textureFilter, MATERIAL_BLENDING_MODE blendingMode,
                TEXTURE_ADDRESS_MODE addressModeU, TEXTURE_ADDRESS_MODE addressModeV, int32 minLOD, int32 maxLOD, 
                PIXEL_FORMAT pixelFormat, uint32 width, uint32 height, uint32 arraySize, uint32 mipLevels);

    bool Load(const char *name, ByteStream *pStream);

    // mip level accessor
    const ImageData *GetMipLevelData(uint32 mipIndex, uint32 arrayIndex) const { DebugAssert(arrayIndex < m_iArraySize && mipIndex < m_nMipLevels); return &m_pImages[mipIndex * m_iArraySize + arrayIndex]; }
    ImageData *GetMipLevelData(uint32 mipIndex, uint32 arrayIndex) { DebugAssert(arrayIndex < m_iArraySize && mipIndex < m_nMipLevels); return &m_pImages[mipIndex * m_iArraySize + arrayIndex]; }

    // device resources
    virtual bool CreateDeviceResources() const;
    virtual void ReleaseDeviceResources() const;

    // calculate the U coordinate from this texture
    float GetUCoordinate(int32 pos) const { return (float)pos / (float)m_iWidth; }
    float GetVCoordinate(int32 pos) const { return (float)pos / (float)m_iHeight; }

    // exporting to an image
    bool ExportToImage(uint32 mipIndex, uint32 arrayIndex, Image *pOutputImage) const;

protected:
    TEXTURE_ADDRESS_MODE m_eAddressModeU, m_eAddressModeV;

    uint32 m_iWidth, m_iHeight;
    uint32 m_iArraySize;
};

class TextureCube : public Texture
{
    DECLARE_RESOURCE_TYPE_INFO(TextureCube, Texture);
    DECLARE_RESOURCE_GENERIC_FACTORY(TextureCube);

public:
    TextureCube(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~TextureCube();

    const TEXTURE_ADDRESS_MODE GetAddressModeU() const { return m_eAddressModeU; }
    const TEXTURE_ADDRESS_MODE GetAddressModeV() const { return m_eAddressModeV; }
    const uint32 GetWidth() const { return m_iWidth; }
    const uint32 GetHeight() const { return m_iHeight; }

    bool Load(const char *name, ByteStream *pStream);

    // mip level accessor
    const ImageData *GetMipLevelData(uint32 cubeFace, uint32 mipIndex) const { DebugAssert(cubeFace < CUBEMAP_FACE_COUNT && mipIndex < m_nMipLevels); return &m_pImages[(uint32)cubeFace * m_nMipLevels + mipIndex]; }
    ImageData *GetMipLevelData(uint32 cubeFace, uint32 mipIndex) { DebugAssert(cubeFace < CUBEMAP_FACE_COUNT && mipIndex < m_nMipLevels); return &m_pImages[(uint32)cubeFace * m_nMipLevels + mipIndex]; }

    // device resources
    virtual bool CreateDeviceResources() const;
    virtual void ReleaseDeviceResources() const;

    // calculate the U coordinate from this texture
    float GetUCoordinate(int32 pos) const { return (float)pos / (float)m_iWidth; }
    float GetVCoordinate(int32 pos) const { return (float)pos / (float)m_iHeight; }

    // exporting to an image
    bool ExportToImage(uint32 cubeFace, uint32 mipIndex, Image *pOutputImage) const;

protected:
    TEXTURE_ADDRESS_MODE m_eAddressModeU, m_eAddressModeV;

    uint32 m_iWidth, m_iHeight;
};

/*
    // mip level accessor
    const ImageData *GetMipLevelData(uint32 ArrayIndex, uint32 MipIndex) const { DebugAssert(ArrayIndex < m_iArraySize && MipIndex < m_nMipLevels); return &m_pImages[ArrayIndex * m_nMipLevels + MipIndex]; }
    ImageData *GetMipLevelData(uint32 ArrayIndex, uint32 MipIndex) { DebugAssert(ArrayIndex < m_iArraySize && MipIndex < m_nMipLevels); return &m_pImages[ArrayIndex * m_nMipLevels + MipIndex]; }
*/
