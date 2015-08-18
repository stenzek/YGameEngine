#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"

class OpenGLES2GPUTexture2D : public GPUTexture2D
{
public:
    OpenGLES2GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLES2GPUTexture2D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLES2GPUTextureCube : public GPUTextureCube
{
public:
    OpenGLES2GPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLES2GPUTextureCube();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLES2GPUDepthTexture : public GPUDepthTexture
{
public:
    OpenGLES2GPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc, GLuint glRenderBufferId);
    virtual ~OpenGLES2GPUDepthTexture();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLRenderBufferId() const { return m_glRenderBufferId; }

private:
    GLuint m_glRenderBufferId;
};


class OpenGLES2GPURenderTargetView : public GPURenderTargetView
{
public:
    OpenGLES2GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLuint textureName);
    virtual ~OpenGLES2GPURenderTargetView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    TEXTURE_TYPE GetTextureType() const { return m_textureType; }
    GLuint GetTextureName() const { return m_textureName; }

protected:
    TEXTURE_TYPE m_textureType;
    GLuint m_textureName;
};

class OpenGLES2GPUDepthStencilBufferView : public GPUDepthStencilBufferView
{
public:
    OpenGLES2GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLenum attachmentPoint, GLuint textureName);
    virtual ~OpenGLES2GPUDepthStencilBufferView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    TEXTURE_TYPE GetTextureType() const { return m_textureType; }
    GLenum GetAttachmentPoint() const { return m_attachmentPoint; }
    GLuint GetTextureName() const { return m_textureName; }

protected:
    TEXTURE_TYPE m_textureType;
    GLenum m_attachmentPoint;
    GLuint m_textureName;
};
