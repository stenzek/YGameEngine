#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2GPUDevice.h"
Log_SetChannel(OpenGLES2RenderBackend);

static const GLenum s_GLCubeMapFaceEnums[CUBEMAP_FACE_COUNT] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

static void SetOpenGLTextureState(GLenum textureTarget, uint32 mipLevels, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, OpenGLES2TypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter, (mipLevels > 1)));
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, OpenGLES2TypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, OpenGLES2TypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, OpenGLES2TypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));

#ifdef GL_EXT_texture_filter_anisotropic
    if (GLAD_GL_EXT_texture_filter_anisotropic)
    {
        if (pSamplerStateDesc->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerStateDesc->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
            glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)pSamplerStateDesc->MaxAnisotropy);
        else
            glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }
#endif
}

static void SetDefaultOpenGLTextureState(GLenum textureTarget)
{
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef GL_EXT_texture_filter_anisotropic
    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES2GPUTexture2D::OpenGLES2GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc, GLuint glTextureId)
    : GPUTexture2D(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLES2GPUTexture2D::~OpenGLES2GPUTexture2D()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLES2GPUTexture2D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage;
    }
}

void OpenGLES2GPUTexture2D::SetDebugName(const char *name)
{
    OpenGLES2Helpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture2D *OpenGLES2GPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // validate mip levels if using mipmapped sample filter
    if (pSamplerStateDesc != nullptr && Renderer::TextureFilterRequiresMips(pSamplerStateDesc->Filter) && pTextureDesc->MipLevels > 1 && 
        pTextureDesc->MipLevels != Renderer::CalculateMipCount(pTextureDesc->Width, pTextureDesc->Height))
    { 
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture2D: GLES 2.0 requires a full texture mip chain if a mip filter is used.");
        return nullptr;
    }

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLES2TypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture2D: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateTexture2D: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // switch to the mutator texture unit
    BindMutatorTextureUnit();

    // bind texture
    glBindTexture(GL_TEXTURE_2D, glTextureId);

    // texture is compressed?
    if (pPixelFormatInfo->IsBlockCompressed)
    {
        for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
        {
            uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
            uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
            uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);
            uint32 imageSize = (hasInitialData) ? pInitialDataPitch[i] * blocksHigh : 0;
            const void *pInitialData = (hasInitialData) ? ppInitialData[i] : nullptr;
            glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, imageSize, pInitialData);
        }
    }
    else
    {
        // flip and upload mip levels
        for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
        {
            // calculate dimensions
            uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
            uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
            glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, (hasInitialData) ? ppInitialData[i] : nullptr);
        }
    }

    // setup sampler state
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        if (pSamplerStateDesc != nullptr)
            SetOpenGLTextureState(GL_TEXTURE_2D, pTextureDesc->MipLevels, pSamplerStateDesc);
        else
            SetDefaultOpenGLTextureState(GL_TEXTURE_2D);
    }

    // restore mutator texture unit
    RestoreMutatorTextureUnit();

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateTexture2D: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLES2GPUTexture2D(pTextureDesc, glTextureId);
}

bool OpenGLES2GPUContext::ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLES2GPUTexture2D *pOpenGLTexture = static_cast<OpenGLES2GPUTexture2D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    if (pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLES2TypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pOpenGLTexture->GetGLTextureId(), mipIndex);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_FRAMEBUFFER, (m_usingFrameBufferObject) ? m_drawFrameBufferObjectId : 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLES2GPUTexture2D *pOpenGLTexture = static_cast<OpenGLES2GPUTexture2D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // check destination size
    if ((countY * sourceRowPitch) > cbSource)
        return false;

    // flip pixels if we have more than one row
    byte *pFlippedPixels = nullptr;
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // flip y coordinate
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);

        // flip rows
        pFlippedPixels = new byte[cbSource];
        PixelFormat_FlipImage(pFlippedPixels, pSource, sourceRowPitch, countY);
        pSource = pFlippedPixels;
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLES2TypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    BindMutatorTextureUnit();
    {
        glBindTexture(GL_TEXTURE_2D, pOpenGLTexture->GetGLTextureId());
        glTexSubImage2D(GL_TEXTURE_2D, mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
    }
    RestoreMutatorTextureUnit();

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES2GPUTextureCube::OpenGLES2GPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc, GLuint glTextureId)
    : GPUTextureCube(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLES2GPUTextureCube::~OpenGLES2GPUTextureCube()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLES2GPUTextureCube::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage * CUBEMAP_FACE_COUNT;
    }
}

void OpenGLES2GPUTextureCube::SetDebugName(const char *name)
{
    OpenGLES2Helpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTextureCube *OpenGLES2GPUDevice::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // validate mip levels if using mipmapped sample filter
    if (pSamplerStateDesc != nullptr && Renderer::TextureFilterRequiresMips(pSamplerStateDesc->Filter) && pTextureDesc->MipLevels > 1 && 
        pTextureDesc->MipLevels != Renderer::CalculateMipCount(pTextureDesc->Width, pTextureDesc->Height))
    {
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture2D: GLES 2.0 requires a full texture mip chain if a mip filter is used.");
        return nullptr;
    }

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLES2TypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTextureCube: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateTextureCube: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // switch to the mutator texture unit
    BindMutatorTextureUnit();

    // bind texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, glTextureId);

    // texture is compressed?
    if (pPixelFormatInfo->IsBlockCompressed)
    {
        for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
        {
            uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
            uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
            uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

            // upload each array texture
            if (hasInitialData)
            {
                // upload each face
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
            }
        }
    }
    else
    {
        // upload mip levels
        for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
        {
            // calculate dimensions
            uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
            uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);

            // upload array
            if (hasInitialData)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, nullptr);
            }
        }
    }

    // setup sampler state
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        if (pSamplerStateDesc != nullptr)
            SetOpenGLTextureState(GL_TEXTURE_CUBE_MAP, pTextureDesc->MipLevels, pSamplerStateDesc);
        else
            SetDefaultOpenGLTextureState(GL_TEXTURE_CUBE_MAP);
    }

    // restore currently-bound texture
    RestoreMutatorTextureUnit();

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateTextureCube: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLES2GPUTextureCube(pTextureDesc, glTextureId);
}

bool OpenGLES2GPUContext::ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLES2GPUTextureCube *pOpenGLTexture = static_cast<OpenGLES2GPUTextureCube *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0 && face < CUBEMAP_FACE_COUNT);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    if (pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLES2TypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_GLCubeMapFaceEnums[face], pOpenGLTexture->GetGLTextureId(), mipIndex);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_FRAMEBUFFER, (m_usingFrameBufferObject) ? m_drawFrameBufferObjectId : 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLES2GPUTextureCube *pOpenGLTexture = static_cast<OpenGLES2GPUTextureCube *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0 && face < CUBEMAP_FACE_COUNT);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // check destination size
    if ((countY * sourceRowPitch) > cbSource)
        return false;

    // flip pixels if we have more than one row
    byte *pFlippedPixels = nullptr;
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // flip y coordinate
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);

        // flip rows
        pFlippedPixels = new byte[cbSource];
        PixelFormat_FlipImage(pFlippedPixels, pSource, sourceRowPitch, countY);
        pSource = pFlippedPixels;
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLES2TypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    BindMutatorTextureUnit();
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, pOpenGLTexture->GetGLTextureId());
        glTexSubImage2D(s_GLCubeMapFaceEnums[face], mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
    }
    RestoreMutatorTextureUnit();

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES2GPUDepthTexture::OpenGLES2GPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc, GLuint glRenderBufferId)
    : GPUDepthTexture(pDesc),
      m_glRenderBufferId(glRenderBufferId)
{

}

OpenGLES2GPUDepthTexture::~OpenGLES2GPUDepthTexture()
{
    glDeleteRenderbuffers(1, &m_glRenderBufferId);
}

void OpenGLES2GPUDepthTexture::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        *gpuMemoryUsage = PixelFormat_CalculateImageSize(m_desc.Format, m_desc.Width, m_desc.Height, 1);
    }
}

void OpenGLES2GPUDepthTexture::SetDebugName(const char *name)
{
    OpenGLES2Helpers::SetObjectDebugName(GL_RENDERBUFFER, m_glRenderBufferId, name);
}

GPUDepthTexture *OpenGLES2GPUDevice::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0);

    // convert to opengl types
    GLint glInternalFormat;
    if (!OpenGLES2TypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, nullptr, nullptr))
    {
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateDepthTexture: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glRenderBufferId;
    glGenRenderbuffers(1, &glRenderBufferId);
    if (glRenderBufferId == 0)
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateDepthTexture: glGenTextures failed: ");
        return nullptr;
    }

    // bind texture, and allocate storage
    glBindRenderbuffer(GL_RENDERBUFFER, glRenderBufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLES2GPUDevice::CreateDepthTexture: One or more GL errors occured: ");
        glDeleteTextures(1, &glRenderBufferId);
        return nullptr;
    }

    // create texture class
    return new OpenGLES2GPUDepthTexture(pTextureDesc, glRenderBufferId);
}

bool OpenGLES2GPUContext::ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLES2GPUDepthTexture *pOpenGLTexture = static_cast<OpenGLES2GPUDepthTexture *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    if ((startX + countX) > pOpenGLTexture->GetDesc()->Width || (startY + countY) > pOpenGLTexture->GetDesc()->Height)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    startY = pOpenGLTexture->GetDesc()->Height - startY - countY;
    DebugAssert(startY < pOpenGLTexture->GetDesc()->Height);

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLES2TypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    if (glFormat == GL_DEPTH_COMPONENT)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pOpenGLTexture->GetGLRenderBufferId());
    //else if (glFormat == GL_DEPTH_STENCIL)
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pOpenGLTexture->GetGLRenderBufferId());

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    if (glFormat == GL_DEPTH_COMPONENT)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    //else if (glFormat == GL_DEPTH_STENCIL)
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

    // restore fbo state
    glBindFramebuffer(GL_FRAMEBUFFER, (m_usingFrameBufferObject) ? m_drawFrameBufferObjectId : 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLES2GPUContext::WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    // can't write to renderbuffers
    return false;
}


static GLenum GetTextureDepthStencilAttachmentPoint(PIXEL_FORMAT pixelFormat)
{
    // todo: move to texture?
    switch (pixelFormat)
    {
    case PIXEL_FORMAT_D16_UNORM:
    case PIXEL_FORMAT_D32_FLOAT:
        return GL_DEPTH_ATTACHMENT;
    }

    UnreachableCode();
    return (GLenum)0;
}


OpenGLES2GPURenderTargetView::OpenGLES2GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLuint textureName)
    : GPURenderTargetView(pTexture, pDesc),
      m_textureType(textureType),
      m_textureName(textureName)
{

}

OpenGLES2GPURenderTargetView::~OpenGLES2GPURenderTargetView()
{
    
}

void OpenGLES2GPURenderTargetView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void OpenGLES2GPURenderTargetView::SetDebugName(const char *name)
{

}

GPURenderTargetView *OpenGLES2GPUDevice::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // fill in DSV structure
    TEXTURE_TYPE textureType;
    GLuint textureName;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE2D:
        textureType = TEXTURE_TYPE_2D;
        textureName = static_cast<OpenGLES2GPUTexture2D *>(pTexture)->GetGLTextureId();
        break;
        
    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        textureType = TEXTURE_TYPE_CUBE;
        textureName = static_cast<OpenGLES2GPUTextureCube *>(pTexture)->GetGLTextureId();
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        textureType = TEXTURE_TYPE_DEPTH;
        textureName = static_cast<OpenGLES2GPUDepthTexture *>(pTexture)->GetGLRenderBufferId();
        break;

    default:
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateRenderTargetView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    return new OpenGLES2GPURenderTargetView(pTexture, pDesc, textureType, textureName);
}

OpenGLES2GPUDepthStencilBufferView::OpenGLES2GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLenum attachmentPoint, GLuint textureName)
    : GPUDepthStencilBufferView(pTexture, pDesc),
      m_textureType(textureType),
      m_attachmentPoint(attachmentPoint),
      m_textureName(textureName)
{

}

OpenGLES2GPUDepthStencilBufferView::~OpenGLES2GPUDepthStencilBufferView()
{

}

void OpenGLES2GPUDepthStencilBufferView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void OpenGLES2GPUDepthStencilBufferView::SetDebugName(const char *name)
{

}

GPUDepthStencilBufferView *OpenGLES2GPUDevice::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // fill in DSV structure
    TEXTURE_TYPE textureType;
    GLenum attachmentPoint;
    GLuint textureName;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE2D:
        textureType = TEXTURE_TYPE_2D;
        attachmentPoint = GetTextureDepthStencilAttachmentPoint(static_cast<OpenGLES2GPUTexture2D *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLES2GPUTexture2D *>(pTexture)->GetGLTextureId();
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        textureType = TEXTURE_TYPE_CUBE;
        attachmentPoint = GetTextureDepthStencilAttachmentPoint(static_cast<OpenGLES2GPUTextureCube *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLES2GPUTextureCube *>(pTexture)->GetGLTextureId();
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        textureType = TEXTURE_TYPE_DEPTH;
        attachmentPoint = GetTextureDepthStencilAttachmentPoint(static_cast<OpenGLES2GPUDepthTexture *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLES2GPUDepthTexture *>(pTexture)->GetGLRenderBufferId();
        break;

    default:
        Log_ErrorPrintf("OpenGLES2GPUDevice::CreateDepthBufferView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    return new OpenGLES2GPUDepthStencilBufferView(pTexture, pDesc, textureType, attachmentPoint, textureName);
}

