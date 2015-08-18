#pragma once
#include "Renderer/ShaderComponent.h"

class GPUTexture;
class Texture;
class ShaderProgram;

class OverlayShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(OverlayShader, ShaderComponent);

public:
    struct Vertex2D
    {
        float2 Position;
        float2 TexCoord;
        uint32 Color;

        Vertex2D() {}
        Vertex2D(const Vertex2D &v) { Y_memcpy(this, &v, sizeof(*this)); }
        Vertex2D(const float2 &position, const float2 &texcoord, uint32 color) : Position(position), TexCoord(texcoord), Color(color) {}
        Vertex2D(float x, float y, float u, float v, uint32 color) : Position(x, y), TexCoord(u, v), Color(color) {}

        void Set(float x, float y, float u, float v, uint32 color) { Position.Set(x, y); TexCoord.Set(u, v); Color = color; }
        void Set(float x, float y, float u, float v, uint32 r, uint32 g, uint32 b, uint32 a) { Set(x, y, u, v, MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a)); }
        void Set(float x, float y, float u, float v, float r, float g, float b, float a) { Set(x, y, u, v, PixelFormatHelpers::ConvertFloat4ToRGBA(float4(r, g, b, a))); }
        void Set(const float2 &position, const float2 &texcoord, uint32 color) { Position = position; TexCoord = texcoord; Color = color; }
        void Set(const float2 &position, const float2 &texcoord, const float4 &color) { Position = position; TexCoord = texcoord; Color = PixelFormatHelpers::ConvertFloat4ToRGBA(color); }
        void SetColor(float x, float y, uint32 color) { Set(x, y, 0.0f, 0.0f, color); }
        void SetColor(float x, float y, uint32 r, uint32 g, uint32 b, uint32 a) { Set(x, y, 0.0f, 0.0f, r, g, b, a); }
        void SetColor(float x, float y, float r, float g, float b, float a) { Set(x, y, 0.0f, 0.0f, r, g, b, a); }
        void SetColor(const float2 &position, uint32 color) { Set(position, float2::Zero, color); }
        void SetColor(const float2 &position, const float4 &color) { Set(position, float2::Zero, color); }
        void SetUV(float x, float y, float u, float v) { Set(x, y, u, v, 0xFFFFFFFF); }
        void SetUV(const float2 &position, const float2 &uv) { Set(position, uv, 0xFFFFFFFF); }

        // skip float comparisons
        bool operator==(const Vertex2D &v) const { return (Y_memcmp(this, &v, sizeof(*this)) == 0); }
        bool operator!=(const Vertex2D &v) const { return (Y_memcmp(this, &v, sizeof(*this)) != 0); }
    };

    struct Vertex3D
    {
        float3 Position;
        float2 TexCoord;
        uint32 Color;

        Vertex3D() {}
        Vertex3D(const Vertex3D &v) { Y_memcpy(this, &v, sizeof(*this)); }
        Vertex3D(const float3 &position, const float2 &texcoord, uint32 color) : Position(position), TexCoord(texcoord), Color(color) {}
        Vertex3D(float x, float y, float z, float u, float v, uint32 color) : Position(x, y, z), TexCoord(u, v), Color(color) {}

        void Set(float x, float y, float z, float u, float v, uint32 color) { Position.Set(x, y, z); TexCoord.Set(u, v); Color = color; }
        void Set(float x, float y, float z, float u, float v, uint32 r, uint32 g, uint32 b, uint32 a) { Set(x, y, z, u, v, MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a)); }
        void Set(float x, float y, float z, float u, float v, float r, float g, float b, float a) { Set(x, y, z, u, v, PixelFormatHelpers::ConvertFloat4ToRGBA(float4(r, g, b, a))); }
        void Set(const float3 &position, const float2 &texcoord, uint32 color) { Position = position; TexCoord = texcoord; Color = color; }
        void Set(const float3 &position, const float2 &texcoord, const float4 &color) { Position = position; TexCoord = texcoord; Color = PixelFormatHelpers::ConvertFloat4ToRGBA(color); }
        void SetColor(float x, float y, float z, uint32 color) { Set(x, y, z, 0.0f, 0.0f, color); }
        void SetColor(float x, float y, float z, uint32 r, uint32 g, uint32 b, uint32 a) { Set(x, y, z, 0.0f, 0.0f, r, g, b, a); }
        void SetColor(float x, float y, float z, float r, float g, float b, float a) { Set(x, y, z, 0.0f, 0.0f, r, g, b, a); }
        void SetColor(const float3 &position, uint32 color) { Set(position, float2::Zero, color); }
        void SetColor(const float3 &position, const float4 &color) { Set(position, float2::Zero, color); }
        void SetUV(float x, float y, float z, float u, float v) { Set(x, y, z, u, v, 0xFFFFFFFF); }
        void SetUV(const float3 &position, const float2 &uv) { Set(position, uv, 0xFFFFFFFF); }

        // skip float comparisons
        bool operator==(const Vertex3D &v) const { return (Y_memcmp(this, &v, sizeof(*this)) == 0); }
        bool operator!=(const Vertex3D &v) const { return (Y_memcmp(this, &v, sizeof(*this)) != 0); }
    };

public:
    enum FLAGS
    {
        WITH_3D_VERTEX                      = (1 << 0),
        WITH_TEXTURE                        = (1 << 1),
    };

public:
    OverlayShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) {}

    static void SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture *pTexture);
    static void SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, Texture *pTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};
