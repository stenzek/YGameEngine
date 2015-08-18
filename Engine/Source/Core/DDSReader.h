#pragma once
#include "Core/Common.h"
#include "Core/PixelFormat.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/String.h"

enum DDS_TEXTURE_TYPE
{
    DDS_TEXTURE_TYPE_UNKNOWN,
    DDS_TEXTURE_TYPE_1D,
    DDS_TEXTURE_TYPE_2D,
    DDS_TEXTURE_TYPE_3D,
    DDS_TEXTURE_TYPE_CUBE,
    DDS_TEXTURE_TYPE_COUNT,
};

class DDSReader
{
public:
    DDSReader();
    ~DDSReader();

    static DDS_TEXTURE_TYPE GetStreamDDSTextureType(const char *FileName, ByteStream *pStream);

    bool Open(const char *FileName, ByteStream *pStream);
    bool LoadMipLevel(uint32 MipLevel);
    bool LoadAllMipLevels();
    void Close();

    DDS_TEXTURE_TYPE GetType() const { return m_eTextureType; }
    uint32 GetWidth() const { return m_iWidth; }
    uint32 GetHeight() const { return m_iHeight; }
    uint32 GetDepth() const { return m_iDepth; }
    uint32 GetNumMipLevels() const { return m_nMipLevels; }
    uint32 GetArraySize() const { return m_iArraySize; }
    PIXEL_FORMAT GetPixelFormat() const { return m_ePixelFormat; }
    
    uint32 GetMipSize(uint32 ArrayIndex, uint32 MipLevel) const { return _GetMipLevel(ArrayIndex, MipLevel)->Size; }
    uint32 GetMipPitch(uint32 ArrayIndex, uint32 MipLevel) const { return _GetMipLevel(ArrayIndex, MipLevel)->Pitch; }
    const byte *GetMipLevelData(uint32 ArrayIndex, uint32 MipLevel) const { return _GetMipLevel(ArrayIndex, MipLevel)->pData; }

private:
    struct MipLevelData
    {
        uint32 Size;
        uint32 Pitch;
        uint32 OffsetInFile;

        byte *pData;
    };

    const MipLevelData *_GetMipLevel(uint32 ArrayIndex, uint32 MipLevel) const;
    void FreeMipLevels();

    String m_strFileName;
    ByteStream *m_pStream;

    DDS_TEXTURE_TYPE m_eTextureType;
    uint32 m_iWidth, m_iHeight, m_iDepth;
    uint32 m_nMipLevels;
    uint32 m_iArraySize;
    PIXEL_FORMAT m_ePixelFormat;
    MipLevelData *m_pMipLevels;
};

