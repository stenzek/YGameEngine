#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
Log_SetChannel(GLGPUTexture);

//#undef GLEW_ARB_texture_storage
//#define GLEW_ARB_texture_storage 0

static const GLenum s_GLCubeMapFaceEnums[CUBEMAP_FACE_COUNT] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

static void SetOpenGLTextureState(GLenum textureTarget, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    glTexParameterfv(textureTarget, GL_TEXTURE_BORDER_COLOR, pSamplerStateDesc->BorderColor);
    glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_FUNC, OpenGLTypeConversion::GetOpenGLComparisonFunc(pSamplerStateDesc->ComparisonFunc));
    glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, OpenGLTypeConversion::GetOpenGLComparisonMode(pSamplerStateDesc->Filter));
    glTexParameterf(textureTarget, GL_TEXTURE_LOD_BIAS, pSamplerStateDesc->LODBias);
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, OpenGLTypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter));
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, OpenGLTypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glTexParameterf(textureTarget, GL_TEXTURE_MIN_LOD, (float)pSamplerStateDesc->MinLOD);
    glTexParameterf(textureTarget, GL_TEXTURE_MAX_LOD, (float)pSamplerStateDesc->MaxLOD);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_R, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressW));

    if (pSamplerStateDesc->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerStateDesc->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)pSamplerStateDesc->MaxAnisotropy);
    else
        glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
}

static void SetOpenGLTextureStateDSA(GLuint textureID, GLenum textureTarget, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    glTextureParameterfvEXT(textureID, textureTarget, GL_TEXTURE_BORDER_COLOR, pSamplerStateDesc->BorderColor);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_COMPARE_FUNC, OpenGLTypeConversion::GetOpenGLComparisonFunc(pSamplerStateDesc->ComparisonFunc));
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_COMPARE_MODE, OpenGLTypeConversion::GetOpenGLComparisonMode(pSamplerStateDesc->Filter));
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_LOD_BIAS, pSamplerStateDesc->LODBias);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_MIN_FILTER, OpenGLTypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter));
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_MAG_FILTER, OpenGLTypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MIN_LOD, (float)pSamplerStateDesc->MinLOD);
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MAX_LOD, (float)pSamplerStateDesc->MaxLOD);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_S, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_T, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_R, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressW));

    if (pSamplerStateDesc->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerStateDesc->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)pSamplerStateDesc->MaxAnisotropy);
    else
        glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
}

static void SetDefaultOpenGLTextureState(GLenum textureTarget)
{
    glTexParameterfv(textureTarget, GL_TEXTURE_BORDER_COLOR, float4::Zero);
    glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameterf(textureTarget, GL_TEXTURE_LOD_BIAS, 0.0f);
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(textureTarget, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(textureTarget, GL_TEXTURE_MAX_LOD, 1000.0f);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameterf(textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
}

static void SetDefaultOpenGLTextureStateDSA(GLuint textureID, GLenum textureTarget)
{
    glTextureParameterfvEXT(textureID, textureTarget, GL_TEXTURE_BORDER_COLOR, float4::Zero);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_LOD_BIAS, 0.0f);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MAX_LOD, 1000.0f);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTextureParameteriEXT(textureID, textureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTextureParameterfEXT(textureID, textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTexture1D::OpenGLGPUTexture1D(const GPU_TEXTURE1D_DESC *pDesc, GLuint glTextureId)
    : GPUTexture1D(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTexture1D::~OpenGLGPUTexture1D()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTexture1D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage;
    }
}

void OpenGLGPUTexture1D::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture1D *OpenGLGPUDevice::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && !pPixelFormatInfo->IsBlockCompressed);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTexture1D: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture1D: glGenTextures failed: ");
        return nullptr;
    }

    // have data for us?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage1DEXT(glTextureId, GL_TEXTURE_1D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width);
        
        // upload mip levels
        if (hasInitialData)
        {
            for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                glTextureSubImage1DEXT(glTextureId, GL_TEXTURE_1D, i, 0, Max((uint32)1, pTextureDesc->Width >> i), glFormat, glType, ppInitialData[i]);
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_1D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_1D);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_1D, glTextureId);

        // has storage extension?
        if (GLAD_GL_ARB_texture_storage)
        {
            // use texture storage to allocate
            glTexStorage1D(GL_TEXTURE_1D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width);

            // has data?
            if (hasInitialData)
            {
                // flip and upload mip levels
                for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                    glTexSubImage1D(GL_TEXTURE_1D, i, 0, mipWidth, glFormat, glType, ppInitialData[i]);
                }
            }
        }
        else
        {
            // flip and upload mip levels
            for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
            {
                // calculate dimensions
                uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                glTexImage1D(GL_TEXTURE_1D, i, glInternalFormat, mipWidth, 0, glFormat, glType, hasInitialData ? ppInitialData[i] : nullptr);
            }
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_1D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_1D);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture1D: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTexture1D(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count)
{
    OpenGLGPUTexture1D *pOpenGLTexture = static_cast<OpenGLGPUTexture1D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(count > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth)
        return false;

    // check destination size
    if (PixelFormat_CalculateRowPitch(pOpenGLTexture->GetDesc()->Format, mipWidth) > cbDestination)
        return false;

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // complete read?
    if (start == 0 && count == mipWidth)
    {
        BindMutatorTextureUnit();

        glGetTexImage(GL_TEXTURE_1D, mipIndex, glFormat, glType, pDestination);

        RestoreMutatorTextureUnit();
    }
    else
    {
        // bind fbo
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

        // bind to FBO
        glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_1D, pOpenGLTexture->GetGLTextureId(), mipIndex);

        // check fbo completeness
        DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // invoke the read
        glReadPixels(start, 0, count, 1, glFormat, glType, pDestination);

        // unbind texture
        glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

        // restore fbo state
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count)
{
    OpenGLGPUTexture1D *pOpenGLTexture = static_cast<OpenGLGPUTexture1D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(count > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth)
        return false;

    // check destination size
    if (PixelFormat_CalculateRowPitch(pOpenGLTexture->GetDesc()->Format, mipWidth) > cbSource)
        return false;

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage1DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_1D, mipIndex, start, count, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_1D, pOpenGLTexture->GetGLTextureId());
            glTexSubImage1D(GL_TEXTURE_1D, mipIndex, start, count, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTexture1DArray::OpenGLGPUTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pDesc, GLuint glTextureId)
    : GPUTexture1DArray(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTexture1DArray::~OpenGLGPUTexture1DArray()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTexture1DArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage * m_desc.ArraySize;
    }
}

void OpenGLGPUTexture1DArray::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture1DArray *OpenGLGPUDevice::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && !pPixelFormatInfo->IsBlockCompressed);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTexture1DArray: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture1DArray: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage2DEXT(glTextureId, GL_TEXTURE_1D_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->ArraySize);

        // has data?
        if (hasInitialData)
        {
            // flip and upload mip levels
            for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
            {
                // calculate dimensions
                uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                {
                    uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_1D_ARRAY, mipIndex, 0, arrayIndex, mipWidth, 1, glFormat, glType, ppInitialData[dataIndex]);
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_1D_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_1D_ARRAY);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_1D_ARRAY, glTextureId);

        // has storage extension?
        if (GLAD_GL_ARB_texture_storage)
        {
            // use texture storage to allocate
            glTexStorage2D(GL_TEXTURE_1D_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->ArraySize);

            // has data?
            if (hasInitialData)
            {
                // flip and upload mip levels
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glTexSubImage2D(GL_TEXTURE_1D_ARRAY, mipIndex, 0, arrayIndex, mipWidth, 1, glFormat, glType, ppInitialData[dataIndex]);
                    }
                }
            }
        }
        else
        {
            // flip and upload mip levels
            for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
            {
                // calculate dimensions
                uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);

                // allocate the mip level
                glTexImage2D(GL_TEXTURE_1D_ARRAY, mipIndex, glInternalFormat, mipWidth, pTextureDesc->ArraySize, 0, glFormat, glType, nullptr);

                // upload array
                if (hasInitialData)
                {
                    for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glTexSubImage2D(GL_TEXTURE_1D_ARRAY, mipIndex, 0, arrayIndex, mipWidth, 1, glFormat, glType, ppInitialData[dataIndex]);
                    }
                }
            }
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_1D_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_1D_ARRAY);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture1DArray: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTexture1DArray(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    OpenGLGPUTexture1DArray *pOpenGLTexture = static_cast<OpenGLGPUTexture1DArray *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(count > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if (PixelFormat_CalculateRowPitch(pOpenGLTexture->GetDesc()->Format, count) > cbDestination)
        return false;

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pOpenGLTexture->GetGLTextureId(), mipIndex, arrayIndex);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(start, 0, count, 0, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    OpenGLGPUTexture1DArray *pOpenGLTexture = static_cast<OpenGLGPUTexture1DArray *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(count > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if (PixelFormat_CalculateRowPitch(pOpenGLTexture->GetDesc()->Format, count) > cbSource)
        return false;

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage2DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_1D_ARRAY, mipIndex, start, arrayIndex, count, 1, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_1D_ARRAY, pOpenGLTexture->GetGLTextureId());
            glTexSubImage2D(GL_TEXTURE_1D_ARRAY, mipIndex, start, arrayIndex, count, 1, glFormat, glType, pSource);
            glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
        }
        RestoreMutatorTextureUnit();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTexture2D::OpenGLGPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc, GLuint glTextureId)
    : GPUTexture2D(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTexture2D::~OpenGLGPUTexture2D()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTexture2D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
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

void OpenGLGPUTexture2D::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture2D *OpenGLGPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTexture2D: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture2D: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage2DEXT(glTextureId, GL_TEXTURE_2D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

        // upload mip levels
        if (hasInitialData)
        {
            // texture is compressed?
            if (pPixelFormatInfo->IsBlockCompressed)
            {
                for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_2D, i, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[i] * blocksHigh, ppInitialData[i]);
                }
            }
            else
            {
                for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_2D, i, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[i]);
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_2D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_2D);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_2D, glTextureId);

        // texture is compressed?
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage
                glTexStorage2D(GL_TEXTURE_2D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

                // fill mip levels if provided
                if (hasInitialData)
                {
                    for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                    {
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
                        uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);
                        glCompressedTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[i] * blocksHigh, ppInitialData[i]);
                    }
                }
            }
            else
            {
                // use traditional method
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
        }
        else
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage to allocate
                glTexStorage2D(GL_TEXTURE_2D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

                // has data?
                if (hasInitialData)
                {
                    // flip and upload mip levels
                    for (uint32 i = 0; i < pTextureDesc->MipLevels; i++)
                    {
                        // calculate dimensions
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> i);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> i);
                        glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[i]);
                    }
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
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_2D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_2D);
        }

        // restore mutator texture unit
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture2D: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTexture2D(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTexture2D *pOpenGLTexture = static_cast<OpenGLGPUTexture2D *>(pTexture);
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
    if (pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // complete read?
    if (startX == 0 && startY == 0 && countX == mipWidth && countY == mipHeight)
    {
        BindMutatorTextureUnit();

        glGetTexImage(GL_TEXTURE_2D, mipIndex, glFormat, glType, pDestination);

        RestoreMutatorTextureUnit();
    }
    else
    {
        // bind fbo
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

        // bind to FBO
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pOpenGLTexture->GetGLTextureId(), mipIndex);

        // check fbo completeness
        DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // invoke the read
        glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

        // unbind texture
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

        // restore fbo state
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTexture2D *pOpenGLTexture = static_cast<OpenGLGPUTexture2D *>(pTexture);
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
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
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
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage2DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_2D, mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_2D, pOpenGLTexture->GetGLTextureId());
            glTexSubImage2D(GL_TEXTURE_2D, mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTexture2DArray::OpenGLGPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc, GLuint glTextureId)
    : GPUTexture2DArray(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTexture2DArray::~OpenGLGPUTexture2DArray()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTexture2DArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage * m_desc.ArraySize;
    }
}

void OpenGLGPUTexture2DArray::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture2DArray *OpenGLGPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTexture2DArray: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture2DArray: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // use texture storage
        glTextureStorage3DEXT(glTextureId, GL_TEXTURE_2D_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->ArraySize);

        // texture is compressed?
        if (hasInitialData)
        {
            if (pPixelFormatInfo->IsBlockCompressed)
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                    for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glCompressedTextureSubImage3DEXT(glTextureId, GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                    }
                }
            }
            else
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glTextureSubImage3DEXT(glTextureId, GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                    }
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_2D_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_2D_ARRAY);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_2D_ARRAY, glTextureId);

        // texture is compressed?
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage
                glTexStorage3D(GL_TEXTURE_2D_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->ArraySize);

                // fill mip levels if provided
                if (hasInitialData)
                {
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                        for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
            else
            {
                // use traditional method
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                    // allocate mip level
                    glTexImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, glInternalFormat, mipWidth, mipHeight, pTextureDesc->ArraySize, 0, glFormat, glType, nullptr);

                    // upload each array texture
                    if (hasInitialData)
                    {
                        for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
        }
        else
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage to allocate
                glTexStorage3D(GL_TEXTURE_2D_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->ArraySize);

                // has data?
                if (hasInitialData)
                {
                    // flip and upload mip levels
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        // calculate dimensions
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
            else
            {
                // flip and upload mip levels
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);

                    // allocate the mip level
                    glTexImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, glInternalFormat, mipWidth, mipHeight, pTextureDesc->ArraySize, 0, glFormat, glType, nullptr);

                    // upload array
                    if (hasInitialData)
                    {
                        for (uint32 arrayIndex = 0; arrayIndex < pTextureDesc->ArraySize; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_2D_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_2D_ARRAY);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture2DArray: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTexture2DArray(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTexture2DArray *pOpenGLTexture = static_cast<OpenGLGPUTexture2DArray *>(pTexture);
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
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    if (pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pOpenGLTexture->GetGLTextureId(), mipIndex, arrayIndex);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTexture2DArray *pOpenGLTexture = static_cast<OpenGLGPUTexture2DArray *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if ((countY * sourceRowPitch) > cbSource)
        return false;

    // flip pixels if we have more than one row
    byte *pFlippedPixels = nullptr;
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
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
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage3DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_2D_ARRAY, mipIndex, startX, startY, arrayIndex, countX, countY, 1, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_2D_ARRAY, pOpenGLTexture->GetGLTextureId());
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, startX, startY, arrayIndex, countX, countY, 1, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTexture3D::OpenGLGPUTexture3D(const GPU_TEXTURE3D_DESC *pDesc, GLuint glTextureId)
    : GPUTexture3D(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTexture3D::~OpenGLGPUTexture3D()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTexture3D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), Max(m_desc.Depth >> j, (uint32)1));

        *gpuMemoryUsage = memoryUsage;
    }
}

void OpenGLGPUTexture3D::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTexture3D *OpenGLGPUDevice::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */, const uint32 *pInitialDataSlicePitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->Depth > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTexture3D: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture3D: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage3DEXT(glTextureId, GL_TEXTURE_3D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->Depth);

        // upload mip levels
        if (hasInitialData)
        {
            // texture is compressed?
            if (pPixelFormatInfo->IsBlockCompressed)
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);
                    uint32 blocksDeep = Max((uint32)1, mipDepth / pPixelFormatInfo->BlockSize);
                    glCompressedTextureSubImage3DEXT(glTextureId, GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glInternalFormat, pInitialDataSlicePitch[mipIndex] * blocksDeep, ppInitialData[mipIndex]);
                }
            }
            else
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);

                    // upload image
                    glTextureSubImage3DEXT(glTextureId, GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glFormat, glType, ppInitialData[mipIndex]);
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_3D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_3D);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_3D, glTextureId);

        // texture is compressed?
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage
                glTexStorage3D(GL_TEXTURE_3D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->Depth);

                // fill mip levels if provided
                if (hasInitialData)
                {
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);
                        uint32 blocksDeep = Max((uint32)1, mipDepth / pPixelFormatInfo->BlockSize);
                        glCompressedTexSubImage3D(GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glInternalFormat, pInitialDataSlicePitch[mipIndex] * blocksDeep, ppInitialData[mipIndex]);
                    }
                }
            }
            else
            {
                // use traditional method
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);
                    uint32 blocksDeep = Max((uint32)1, mipDepth / pPixelFormatInfo->BlockSize);

                    // allocate mip level
                    if (hasInitialData)
                        glCompressedTexImage3D(GL_TEXTURE_3D, mipIndex, glInternalFormat, mipWidth, mipHeight, mipDepth, 0, pInitialDataSlicePitch[mipIndex] * blocksDeep, ppInitialData[mipIndex]);
                    else
                        glCompressedTexImage3D(GL_TEXTURE_3D, mipIndex, glInternalFormat, mipWidth, mipHeight, mipDepth, 0, 0, nullptr);
                }
            }
        }
        else
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage to allocate
                glTexStorage3D(GL_TEXTURE_3D, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, pTextureDesc->Depth);

                // has data?
                if (hasInitialData)
                {
                    // flip and upload mip levels
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        // calculate dimensions
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);

                        // upload image
                        glTexSubImage3D(GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glFormat, glType, ppInitialData[mipIndex]);
                    }
                }
            }
            else
            {
                // flip and upload mip levels
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 mipDepth = Max((uint32)1, pTextureDesc->Depth >> mipIndex);

                    if (hasInitialData)
                        glTexSubImage3D(GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glFormat, glType, ppInitialData[mipIndex]);
                    else
                        glTexSubImage3D(GL_TEXTURE_3D, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, glFormat, glType, nullptr);
                }
            }
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_3D, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_3D);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTexture3D: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTexture3D(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    OpenGLGPUTexture3D *pOpenGLTexture = static_cast<OpenGLGPUTexture3D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0 && countZ > 0);

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
    uint32 mipDepth = Max(pOpenGLTexture->GetDesc()->Depth >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || (startZ + countZ) > mipDepth)
        return false;

    // check destination size
    if ((countZ * destinationSlicePitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    if (pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // read each slice
    byte *pWritePointer = reinterpret_cast<byte *>(pDestination);
    for (uint32 i = 0; i < countZ; i++)
    {
        glFramebufferTexture3D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, pOpenGLTexture->GetGLTextureId(), mipIndex, startZ + i);
        DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glReadPixels(startX, startY, countX, countY, glFormat, glType, pWritePointer);
        pWritePointer += destinationSlicePitch;
    }

    // unbind texture
    glFramebufferTexture3D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        pWritePointer = reinterpret_cast<byte *>(pDestination);
        for (uint32 i = 0; i < countZ; i++)
        {
            PixelFormat_FlipImageInPlace(pWritePointer, destinationRowPitch, countY);
            pWritePointer += destinationSlicePitch;
        }
    }

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    OpenGLGPUTexture3D *pOpenGLTexture = static_cast<OpenGLGPUTexture3D *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0 && countZ > 0);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    uint32 mipDepth = Max(pOpenGLTexture->GetDesc()->Depth >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || (startZ + countZ) > mipDepth)
        return false;

    // check destination size
    if ((countZ * sourceSlicePitch) > cbSource)
        return false;

    // flip pixels if we have more than one row
    byte *pFlippedPixels = nullptr;
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        // flip y coordinate
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);

        // flip rows
        pFlippedPixels = new byte[cbSource];
        const byte *pReadPointer = reinterpret_cast<const byte *>(pSource);
        byte *pWritePointer = reinterpret_cast<byte *>(pFlippedPixels);
        for (uint32 i = 0; i < countZ; i++)
        {
            PixelFormat_FlipImage(pWritePointer, pReadPointer, sourceRowPitch, countY);
            pReadPointer += sourceSlicePitch;
            pWritePointer += sourceSlicePitch;
        }
        pSource = pFlippedPixels;
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage3DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_3D, mipIndex, startX, startY, startZ, countX, countY, countZ, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_3D, pOpenGLTexture->GetGLTextureId());
            glTexSubImage3D(GL_TEXTURE_3D, mipIndex, startX, startY, startZ, countX, countY, countZ, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTextureCube::OpenGLGPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc, GLuint glTextureId)
    : GPUTextureCube(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTextureCube::~OpenGLGPUTextureCube()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTextureCube::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
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

void OpenGLGPUTextureCube::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTextureCube *OpenGLGPUDevice::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTextureCube: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTextureCube: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

        // upload mip levels
        if (hasInitialData)
        {
            if (pPixelFormatInfo->IsBlockCompressed)
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                    // upload each face
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                    glCompressedTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                }
            }
            else
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);

                    // upload data
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                    glTextureSubImage2DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_CUBE_MAP, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_CUBE_MAP);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, glTextureId);

        // texture is compressed?
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage
                glTexStorage2D(GL_TEXTURE_CUBE_MAP, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

                // fill mip levels if provided
                if (hasInitialData)
                {
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                        // upload each face
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                        glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glInternalFormat, pInitialDataPitch[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex] * blocksHigh, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                    }
                }
            }
            else
            {
                // use traditional method
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
        }
        else
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage to allocate
                glTexStorage2D(GL_TEXTURE_CUBE_MAP, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);

                // has data?
                if (hasInitialData)
                {
                    // upload mip levels
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        // calculate dimensions
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);

                        // upload data
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_X * pTextureDesc->MipLevels + mipIndex]);
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_X * pTextureDesc->MipLevels + mipIndex]);
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Y * pTextureDesc->MipLevels + mipIndex]);
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_POSITIVE_Z * pTextureDesc->MipLevels + mipIndex]);
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, mipIndex, 0, 0, mipWidth, mipHeight, glFormat, glType, ppInitialData[CUBEMAP_FACE_NEGATIVE_Z * pTextureDesc->MipLevels + mipIndex]);
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
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_CUBE_MAP, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_CUBE_MAP);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTextureCube: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTextureCube(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTextureCube *pOpenGLTexture = static_cast<OpenGLGPUTextureCube *>(pTexture);
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
    if (pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_GLCubeMapFaceEnums[face], pOpenGLTexture->GetGLTextureId(), mipIndex);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTextureCube *pOpenGLTexture = static_cast<OpenGLGPUTextureCube *>(pTexture);
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
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET )
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
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage2DEXT(pOpenGLTexture->GetGLTextureId(), s_GLCubeMapFaceEnums[face], mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, pOpenGLTexture->GetGLTextureId());
            glTexSubImage2D(s_GLCubeMapFaceEnums[face], mipIndex, startX, startY, countX, countY, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUTextureCubeArray::OpenGLGPUTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pDesc, GLuint glTextureId)
    : GPUTextureCubeArray(pDesc),
      m_glTextureId(glTextureId)
{

}

OpenGLGPUTextureCubeArray::~OpenGLGPUTextureCubeArray()
{
    glDeleteTextures(1, &m_glTextureId);
}

void OpenGLGPUTextureCubeArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage * m_desc.ArraySize * CUBEMAP_FACE_COUNT;
    }
}

void OpenGLGPUTextureCubeArray::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_TEXTURE, m_glTextureId, name);
}

GPUTextureCubeArray *OpenGLGPUDevice::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0);
    uint32 faceLayerCount = pTextureDesc->ArraySize * CUBEMAP_FACE_COUNT;

    // convert to opengl types
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, &glFormat, &glType))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateTextureCubeArray: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glTextureId;
    glGenTextures(1, &glTextureId);
    if (glTextureId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTextureCubeArray: glGenTextures failed: ");
        return nullptr;
    }

    // has data to upload?
    bool hasInitialData = (ppInitialData != nullptr && pInitialDataPitch != nullptr);

    // using DSA? we'll also require texture storage since if we have DSA we'll likely have texture storage
    if (GLAD_GL_EXT_direct_state_access && GLAD_GL_ARB_texture_storage)
    {
        // allocate storage
        glTextureStorage3DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, faceLayerCount);

        // upload mip levels
        if (hasInitialData)
        {
            if (pPixelFormatInfo->IsBlockCompressed)
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                    for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glCompressedTextureSubImage3DEXT(glTextureId, GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                    }
                }
            }
            else
            {
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                    {
                        uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                        glTextureSubImage3DEXT(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                    }
                }
            }
        }

        // set texture parameters
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteriEXT(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureStateDSA(glTextureId, GL_TEXTURE_CUBE_MAP_ARRAY);
        }
    }
    else
    {
        // switch to the mutator texture unit
        BindMutatorTextureUnit();

        // bind texture
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, glTextureId);

        // texture is compressed?
        if (pPixelFormatInfo->IsBlockCompressed)
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage
                glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, faceLayerCount);

                // fill mip levels if provided
                if (hasInitialData)
                {
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                        for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
            else
            {
                // use traditional method
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                    uint32 blocksHigh = Max((uint32)1, mipHeight / pPixelFormatInfo->BlockSize);

                    // allocate mip level
                    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, glInternalFormat, mipWidth, mipHeight, faceLayerCount, 0, glFormat, glType, nullptr);

                    // upload each array texture
                    if (hasInitialData)
                    {
                        for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glInternalFormat, pInitialDataPitch[dataIndex] * blocksHigh, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
        }
        else
        {
            // has storage extension?
            if (GLAD_GL_ARB_texture_storage)
            {
                // use texture storage to allocate
                glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, pTextureDesc->MipLevels, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height, faceLayerCount);

                // has data?
                if (hasInitialData)
                {
                    // flip and upload mip levels
                    for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                    {
                        // calculate dimensions
                        uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                        uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);
                        for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
            else
            {
                // flip and upload mip levels
                for (uint32 mipIndex = 0; mipIndex < pTextureDesc->MipLevels; mipIndex++)
                {
                    // calculate dimensions
                    uint32 mipWidth = Max((uint32)1, pTextureDesc->Width >> mipIndex);
                    uint32 mipHeight = Max((uint32)1, pTextureDesc->Height >> mipIndex);

                    // allocate the mip level
                    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, glInternalFormat, mipWidth, mipHeight, faceLayerCount, 0, glFormat, glType, nullptr);

                    // upload array
                    if (hasInitialData)
                    {
                        for (uint32 arrayIndex = 0; arrayIndex < faceLayerCount; arrayIndex++)
                        {
                            uint32 dataIndex = pTextureDesc->MipLevels * arrayIndex + mipIndex;
                            glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, 0, 0, arrayIndex, mipWidth, mipHeight, 1, glFormat, glType, ppInitialData[dataIndex]);
                        }
                    }
                }
            }
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, pTextureDesc->MipLevels - 1);

        // setup sampler state
        if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        {
            if (pSamplerStateDesc != nullptr)
                SetOpenGLTextureState(GL_TEXTURE_CUBE_MAP_ARRAY, pSamplerStateDesc);
            else
                SetDefaultOpenGLTextureState(GL_TEXTURE_CUBE_MAP_ARRAY);
        }

        // restore currently-bound texture
        RestoreMutatorTextureUnit();
    }

    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateTextureCubeArray: One or more GL errors occured: ");
        glDeleteTextures(1, &glTextureId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUTextureCubeArray(pTextureDesc, glTextureId);
}

bool OpenGLGPUContext::ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTextureCubeArray *pOpenGLTexture = static_cast<OpenGLGPUTextureCubeArray *>(pTexture);
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
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // flip the coordinates around, since opengl's origin is at the bottom-left.
    if (pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        startY = mipHeight - startY - countY;
        DebugAssert(startY < mipHeight);
    }

    // get gl formats
    GLenum glFormat;
    GLenum glType;
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pOpenGLTexture->GetGLTextureId(), mipIndex, (arrayIndex * CUBEMAP_FACE_COUNT) + (uint32)face);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUTextureCubeArray *pOpenGLTexture = static_cast<OpenGLGPUTextureCubeArray *>(pTexture);
    DebugAssert(pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0 && face < CUBEMAP_FACE_COUNT);

    // not handling compressed textures at the moment
    if (PixelFormat_GetPixelFormatInfo(pOpenGLTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // calculate mip level
    uint32 mipWidth = Max(pOpenGLTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pOpenGLTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pOpenGLTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if ((countY * sourceRowPitch) > cbSource)
        return false;

    // flip pixels if we have more than one row
    byte *pFlippedPixels = nullptr;
    if (countY > 0 && pOpenGLTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
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
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // update the texture
    if (GLAD_GL_EXT_direct_state_access)
    {
        glTextureSubImage3DEXT(pOpenGLTexture->GetGLTextureId(), GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, startX, startY, (arrayIndex * CUBEMAP_FACE_COUNT) + (uint32)face, countX, countY, 1, glFormat, glType, pSource);
    }
    else
    {
        BindMutatorTextureUnit();
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pOpenGLTexture->GetGLTextureId());
            glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipIndex, startX, startY, (arrayIndex * CUBEMAP_FACE_COUNT) + (uint32)face, countX, countY, 1, glFormat, glType, pSource);
        }
        RestoreMutatorTextureUnit();
    }

    // free temporary buffer
    delete[] pFlippedPixels;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLGPUDepthTexture::OpenGLGPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc, GLuint glRenderBufferId)
    : GPUDepthTexture(pDesc),
      m_glRenderBufferId(glRenderBufferId)
{

}

OpenGLGPUDepthTexture::~OpenGLGPUDepthTexture()
{
    glDeleteRenderbuffers(1, &m_glRenderBufferId);
}

void OpenGLGPUDepthTexture::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        *gpuMemoryUsage = PixelFormat_CalculateImageSize(m_desc.Format, m_desc.Width, m_desc.Height, 1);
    }
}

void OpenGLGPUDepthTexture::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_RENDERBUFFER, m_glRenderBufferId, name);
}

GPUDepthTexture *OpenGLGPUDevice::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    // get pixel format info
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0);

    // convert to opengl types
    GLint glInternalFormat;
    if (!OpenGLTypeConversion::GetOpenGLTextureFormat(pTextureDesc->Format, &glInternalFormat, nullptr, nullptr))
    {
        Log_ErrorPrintf("OpenGLGPUDevice::CreateDepthTexture: Could not get mapping of texture format for %s", pPixelFormatInfo->Name);
        return nullptr;
    }

    GL_CHECKED_SECTION_BEGIN();

    // allocate object id
    GLuint glRenderBufferId;
    glGenRenderbuffers(1, &glRenderBufferId);
    if (glRenderBufferId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateDepthTexture: glGenTextures failed: ");
        return nullptr;
    }

    // bind texture, and allocate storage
    glBindRenderbuffer(GL_RENDERBUFFER, glRenderBufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, glInternalFormat, pTextureDesc->Width, pTextureDesc->Height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    // check state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateDepthTexture: One or more GL errors occured: ");
        glDeleteTextures(1, &glRenderBufferId);
        return nullptr;
    }

    // create texture class
    return new OpenGLGPUDepthTexture(pTextureDesc, glRenderBufferId);
}

bool OpenGLGPUContext::ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    OpenGLGPUDepthTexture *pOpenGLTexture = static_cast<OpenGLGPUDepthTexture *>(pTexture);
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
    OpenGLTypeConversion::GetOpenGLTextureFormat(pOpenGLTexture->GetDesc()->Format, nullptr, &glFormat, &glType);

    // bind fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);

    // bind to FBO
    if (glFormat == GL_DEPTH_COMPONENT)
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pOpenGLTexture->GetGLRenderBufferId());
    else if (glFormat == GL_DEPTH_STENCIL)
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pOpenGLTexture->GetGLRenderBufferId());

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the read
    glReadPixels(startX, startY, countX, countY, glFormat, glType, pDestination);

    // unbind texture
    if (glFormat == GL_DEPTH_COMPONENT)
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    else if (glFormat == GL_DEPTH_STENCIL)
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

    // restore fbo state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // flip the rows as opengl's coordinate system starts at the bottom not the top
    if (countY > 1)
        PixelFormat_FlipImageInPlace(pDestination, destinationRowPitch, countY);

    return true;
}

bool OpenGLGPUContext::WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    // can't write to renderbuffers
    return false;
}

static GLenum GetDepthStencilFramebufferAttachmentPoint(PIXEL_FORMAT pixelFormat)
{
    switch (pixelFormat)
    {
    case PIXEL_FORMAT_D16_UNORM:
    case PIXEL_FORMAT_D32_FLOAT:
        return GL_DEPTH_ATTACHMENT;

    case PIXEL_FORMAT_D24_UNORM_S8_UINT:
    case PIXEL_FORMAT_D32_FLOAT_S8X24_UINT:
        return GL_DEPTH_STENCIL_ATTACHMENT;
    }

    UnreachableCode();
    return 0;
}


OpenGLGPURenderTargetView::OpenGLGPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLuint textureName, bool multiLayer)
    : GPURenderTargetView(pTexture, pDesc),
      m_textureType(textureType),
      m_textureName(textureName),
      m_multiLayer(multiLayer)
{

}

OpenGLGPURenderTargetView::~OpenGLGPURenderTargetView()
{
    
}

void OpenGLGPURenderTargetView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void OpenGLGPURenderTargetView::SetDebugName(const char *name)
{

}

GPURenderTargetView *OpenGLGPUDevice::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // fill in DSV structure
    TEXTURE_TYPE textureType;
    GLuint textureName;
    bool multiLayer;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        textureType = TEXTURE_TYPE_1D;
        textureName = static_cast<OpenGLGPUTexture1D *>(pTexture)->GetGLTextureId();
        multiLayer = false;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        textureType = TEXTURE_TYPE_1D_ARRAY;
        textureName = static_cast<OpenGLGPUTexture1DArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        textureType = TEXTURE_TYPE_2D;
        textureName = static_cast<OpenGLGPUTexture2D *>(pTexture)->GetGLTextureId();
        multiLayer = false;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        textureType = TEXTURE_TYPE_2D_ARRAY;
        textureName = static_cast<OpenGLGPUTexture2DArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        textureType = TEXTURE_TYPE_CUBE;
        textureName = static_cast<OpenGLGPUTextureCube *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        textureType = TEXTURE_TYPE_CUBE_ARRAY;
        textureName = static_cast<OpenGLGPUTextureCubeArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        textureType = TEXTURE_TYPE_DEPTH;
        textureName = static_cast<OpenGLGPUDepthTexture *>(pTexture)->GetGLRenderBufferId();
        multiLayer = false;
        break;

    default:
        Log_ErrorPrintf("OpenGLGPUDevice::CreateRenderTargetView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    return new OpenGLGPURenderTargetView(pTexture, pDesc, textureType, textureName, multiLayer);
}

OpenGLGPUDepthStencilBufferView::OpenGLGPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLenum attachmentPoint, GLuint textureName, bool multiLayer)
    : GPUDepthStencilBufferView(pTexture, pDesc),
      m_textureType(textureType),
      m_attachmentPoint(attachmentPoint),
      m_textureName(textureName),
      m_multiLayer(multiLayer)
{

}

OpenGLGPUDepthStencilBufferView::~OpenGLGPUDepthStencilBufferView()
{

}

void OpenGLGPUDepthStencilBufferView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void OpenGLGPUDepthStencilBufferView::SetDebugName(const char *name)
{

}

GPUDepthStencilBufferView *OpenGLGPUDevice::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // fill in DSV structure
    TEXTURE_TYPE textureType;
    GLenum attachmentPoint;
    GLuint textureName;
    bool multiLayer;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        textureType = TEXTURE_TYPE_1D;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTexture1D *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTexture1D *>(pTexture)->GetGLTextureId();
        multiLayer = false;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        textureType = TEXTURE_TYPE_1D_ARRAY;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTexture1DArray *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTexture1DArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        textureType = TEXTURE_TYPE_2D;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTexture2D *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTexture2D *>(pTexture)->GetGLTextureId();
        multiLayer = false;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        textureType = TEXTURE_TYPE_2D_ARRAY;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTexture2DArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        textureType = TEXTURE_TYPE_CUBE;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTextureCube *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTextureCube *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        textureType = TEXTURE_TYPE_CUBE_ARRAY;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUTextureCubeArray *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUTextureCubeArray *>(pTexture)->GetGLTextureId();
        multiLayer = (pDesc->NumLayers > 1);
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        textureType = TEXTURE_TYPE_DEPTH;
        attachmentPoint = GetDepthStencilFramebufferAttachmentPoint(static_cast<OpenGLGPUDepthTexture *>(pTexture)->GetDesc()->Format);
        textureName = static_cast<OpenGLGPUDepthTexture *>(pTexture)->GetGLRenderBufferId();
        multiLayer = false;
        break;

    default:
        Log_ErrorPrintf("OpenGLGPUDevice::CreateDepthBufferView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    return new OpenGLGPUDepthStencilBufferView(pTexture, pDesc, textureType, attachmentPoint, textureName, multiLayer);
}

OpenGLGPUComputeView::OpenGLGPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
    : GPUComputeView(pResource, pDesc)
{

}

OpenGLGPUComputeView::~OpenGLGPUComputeView()
{

}

void OpenGLGPUComputeView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void OpenGLGPUComputeView::SetDebugName(const char *name)
{

}

GPUComputeView *OpenGLGPUDevice::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    Panic("Unimplemented");
    return nullptr;
}
