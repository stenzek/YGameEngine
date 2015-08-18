#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"

class OpenGLGPUTexture1D : public GPUTexture1D
{
public:
    OpenGLGPUTexture1D(const GPU_TEXTURE1D_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTexture1D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTexture1DArray : public GPUTexture1DArray
{
public:
    OpenGLGPUTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTexture1DArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTexture2D : public GPUTexture2D
{
public:
    OpenGLGPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTexture2D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTexture2DArray : public GPUTexture2DArray
{
public:
    OpenGLGPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTexture2DArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTexture3D : public GPUTexture3D
{
public:
    OpenGLGPUTexture3D(const GPU_TEXTURE3D_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTexture3D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTextureCube : public GPUTextureCube
{
public:
    OpenGLGPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTextureCube();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUTextureCubeArray : public GPUTextureCubeArray
{
public:
    OpenGLGPUTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pDesc, GLuint glTextureId);
    virtual ~OpenGLGPUTextureCubeArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLTextureId() const { return m_glTextureId; }

private:
    GLuint m_glTextureId;
};

class OpenGLGPUDepthTexture : public GPUDepthTexture
{
public:
    OpenGLGPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc, GLuint glRenderBufferId);
    virtual ~OpenGLGPUDepthTexture();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLuint GetGLRenderBufferId() const { return m_glRenderBufferId; }

private:
    GLuint m_glRenderBufferId;
};

class OpenGLGPURenderTargetView : public GPURenderTargetView
{
public:
    OpenGLGPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLuint textureName, bool multiLayer);
    virtual ~OpenGLGPURenderTargetView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    TEXTURE_TYPE GetTextureType() const { return m_textureType; }
    GLuint GetTextureName() const { return m_textureName; }
    bool IsMultiLayer() const { return m_multiLayer; }

protected:
    TEXTURE_TYPE m_textureType;
    GLuint m_textureName;
    bool m_multiLayer;
};

class OpenGLGPUDepthStencilBufferView : public GPUDepthStencilBufferView
{
public:
    OpenGLGPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, TEXTURE_TYPE textureType, GLenum attachmentPoint, GLuint textureName, bool multiLayer);
    virtual ~OpenGLGPUDepthStencilBufferView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    TEXTURE_TYPE GetTextureType() const { return m_textureType; }
    GLenum GetAttachmentPoint() const { return m_attachmentPoint; }
    GLuint GetTextureName() const { return m_textureName; }
    bool IsMultiLayer() const { return m_multiLayer; }

protected:
    TEXTURE_TYPE m_textureType;
    GLenum m_attachmentPoint;
    GLuint m_textureName;
    bool m_multiLayer;
};

class OpenGLGPUComputeView : public GPUComputeView
{
public:
    OpenGLGPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc);
    virtual ~OpenGLGPUComputeView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

protected:
};
