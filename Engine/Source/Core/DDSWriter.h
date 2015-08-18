#pragma once
#include "Core/Common.h"
#include "Core/PixelFormat.h"
#include "Core/DDSReader.h"
#include "YBaseLib/ByteStream.h"

class DDSWriter
{
public:
    DDSWriter();
    ~DDSWriter();

    void Initialize(ByteStream *pOutputStream);
    bool WriteHeader(DDS_TEXTURE_TYPE Type, PIXEL_FORMAT Format, uint32 Width, uint32 Height, uint32 DepthOrArraySize, uint32 MipCount);
    bool WriteMipLevel(uint32 MipLevel, const void *pData, uint32 cbData);
    bool Finalize();

private:
    ByteStream *m_pOutputStream;
    DDS_TEXTURE_TYPE m_eType;
    PIXEL_FORMAT m_pfFormat;
    uint32 m_uWidth, m_uHeight, m_uDepth;
    uint32 m_uArraySize, m_uMipCount;
    uint32 m_uCurrentMipLevel;
    uint32 m_uCurrentWidth, m_uCurrentHeight, m_uCurrentDepth;
};

