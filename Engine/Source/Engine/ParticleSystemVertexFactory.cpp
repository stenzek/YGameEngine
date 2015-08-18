#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemVertexFactory.h"
#include "Engine/Camera.h"
#include "Engine/MaterialShader.h"
#include "Renderer/ShaderCompilerFrontend.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(ParticleSystemSpriteVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(ParticleSystemSpriteVertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

uint32 ParticleSystemSpriteVertexFactory::GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    if (flags & Flag_RenderBasic) 
    {
        // position + texture coordinates + color for now
        return sizeof(float3) + sizeof(float2) + sizeof(uint32);
    }

    if (flags & Flag_RenderInstancedQuads)
    {
        // position/rotation + texture coordinate range in float4 + color
        return sizeof(float4) + sizeof(float4) + sizeof(float2) + sizeof(uint32);
    }

    // error
    Panic("Invalid flags");
    return 0;
}

uint32 ParticleSystemSpriteVertexFactory::GetVerticesPerSprite(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    if (flags & Flag_RenderBasic)
        return 6;

    if (flags & Flag_RenderInstancedQuads)
        return 1;

    // error
    Panic("Invalid flags");
    return 0;
}

DRAW_TOPOLOGY ParticleSystemSpriteVertexFactory::GetDrawTopology(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    if (flags & Flag_RenderBasic)
        return DRAW_TOPOLOGY_TRIANGLE_LIST;

    if (flags & Flag_RenderInstancedQuads)
        return DRAW_TOPOLOGY_TRIANGLE_STRIP;

    // error
    Panic("Invalid flags");
    return DRAW_TOPOLOGY_COUNT;
}

static inline void AppendSpriteVertexBasic(byte *&pCurrentPointer, const float3 &position, const float2 &texcoord, uint32 color)
{
#pragma pack(push, 1)
    struct Vertex
    {
        float Position[3];
        float TexCoord[2];
        uint32 Color;
    };
#pragma pack(pop)

    Vertex *v = reinterpret_cast<Vertex *>(pCurrentPointer);
    position.Store(v->Position);
    texcoord.Store(v->TexCoord);
    v->Color = color;

    pCurrentPointer += sizeof(Vertex);
}

static inline void AppendSpriteVertexInstancedQuads(byte *&pCurrentPointer, const float3 &position, float rotation, const float2 &minTextureCoordinates, const float2 &textureCoordinateRange, float halfWidth, float halfHeight, uint32 color)
{
#pragma pack(push, 1)
    struct Vertex
    {
        float Position[4];
        float TexCoordRange[4];
        float HalfSize[2];
        uint32 Color;
    };
#pragma pack(pop)

    Vertex *v = reinterpret_cast<Vertex *>(pCurrentPointer);
    position.Store(&v->Position[0]);
    v->Position[3] = rotation;
    minTextureCoordinates.Store(&v->TexCoordRange[0]);
    textureCoordinateRange.Store(&v->TexCoordRange[2]);
    v->HalfSize[0] = halfWidth;
    v->HalfSize[1] = halfHeight;
    v->Color = color;

    pCurrentPointer += sizeof(Vertex);
}

bool ParticleSystemSpriteVertexFactory::FillVertexBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Camera *pCamera, const ParticleData *pParticles, uint32 nParticles, void *pBuffer, uint32 bufferSize)
{
    uint32 vertexSize = GetVertexSize(platform, featureLevel, flags);
    if (bufferSize < (vertexSize * nParticles * GetVerticesPerSprite(platform, featureLevel, flags)))
        return false;

    // basic type
    if (flags & Flag_RenderBasic)
    {
        // calculate inverse rotation matrix
        float3x3 inverseViewMatrixRotation(pCamera->GetInverseViewMatrix());

        // loop each sprite
        byte *pCurrentPointer = reinterpret_cast<byte *>(pBuffer);
        for (uint32 spriteIndex = 0; spriteIndex < nParticles; spriteIndex++)
        {
            const ParticleData *pParticleData = &pParticles[spriteIndex];

            // find half width/height
            float halfWidth = pParticleData->Width * 0.5f;
            float halfHeight = pParticleData->Height * 0.5f;

            // find the four vertex positions
            // these coordinates have to be in y-up coordinate system, as we are working with the view matrix
            float3 vertexPositions[4];
            if (pParticleData->Rotation != 0.0f)
            {
                float theta = Math::DegreesToRadians(pParticleData->Rotation);
                float sinTheta, cosTheta;
                Math::SinCos(theta, &sinTheta, &cosTheta);

                vertexPositions[0] = inverseViewMatrixRotation * float3(-halfWidth * cosTheta + -halfHeight * sinTheta, -halfWidth * sinTheta + halfHeight * cosTheta, 0.0f) + pParticleData->Position;
                vertexPositions[1] = inverseViewMatrixRotation * float3(-halfWidth * cosTheta + halfHeight * sinTheta, -halfWidth * sinTheta + -halfHeight * cosTheta, 0.0f) + pParticleData->Position;
                vertexPositions[2] = inverseViewMatrixRotation * float3(halfWidth * cosTheta + -halfHeight * sinTheta, halfWidth * sinTheta + halfHeight * cosTheta, 0.0f) + pParticleData->Position;
                vertexPositions[3] = inverseViewMatrixRotation * float3(halfWidth * cosTheta + halfHeight * sinTheta, halfWidth * sinTheta + -halfHeight * cosTheta, 0.0f) + pParticleData->Position;
            }
            else
            {
                vertexPositions[0] = inverseViewMatrixRotation * float3(-halfWidth, halfHeight, 0.0f) + pParticleData->Position;
                vertexPositions[1] = inverseViewMatrixRotation * float3(-halfWidth, -halfHeight, 0.0f) + pParticleData->Position;
                vertexPositions[2] = inverseViewMatrixRotation * float3(halfWidth, halfHeight, 0.0f) + pParticleData->Position;
                vertexPositions[3] = inverseViewMatrixRotation * float3(halfWidth, -halfHeight, 0.0f) + pParticleData->Position;
            }

            // same for texture coordinates
            float2 vertexTextureCoordinates[4];
            vertexTextureCoordinates[0].Set(pParticleData->MinTextureCoordinates);
            vertexTextureCoordinates[1].Set(pParticleData->MinTextureCoordinates.x, pParticleData->MaxTextureCoordinates.y);
            vertexTextureCoordinates[2].Set(pParticleData->MaxTextureCoordinates.x, pParticleData->MinTextureCoordinates.y);
            vertexTextureCoordinates[3].Set(pParticleData->MaxTextureCoordinates.x, pParticleData->MaxTextureCoordinates.y);

            // first triangle
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[0], vertexTextureCoordinates[0], pParticleData->Color);
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[1], vertexTextureCoordinates[1], pParticleData->Color);
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[2], vertexTextureCoordinates[2], pParticleData->Color);

            // second triangle
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[2], vertexTextureCoordinates[2], pParticleData->Color);
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[1], vertexTextureCoordinates[1], pParticleData->Color);
            AppendSpriteVertexBasic(pCurrentPointer, vertexPositions[3], vertexTextureCoordinates[3], pParticleData->Color);
        }

        return true;
    }
    else if (flags & Flag_RenderInstancedQuads)
    {
        // loop each sprite
        byte *pCurrentPointer = reinterpret_cast<byte *>(pBuffer);
        for (uint32 spriteIndex = 0; spriteIndex < nParticles; spriteIndex++)
        {
            const ParticleData *pParticleData = &pParticles[spriteIndex];

            // find half width/height
            //float halfWidth = pParticleData->Width * 0.5f;
            //float halfHeight = pParticleData->Height * 0.5f;

            // find texture coordinate range
            float2 textureCoordinateRange(pParticleData->MaxTextureCoordinates - pParticleData->MinTextureCoordinates);
            
            // generate vertex
            AppendSpriteVertexInstancedQuads(pCurrentPointer, pParticleData->Position, Math::DegreesToRadians(pParticleData->Rotation), 
                                             pParticleData->MinTextureCoordinates, textureCoordinateRange,
                                             pParticleData->Width, pParticleData->Height, pParticleData->Color);
        }

        // done
        return true;
    }

    return false;
}

bool ParticleSystemSpriteVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    // needs a material to go with it
    if (pMaterialShader == nullptr)
        return false;

    // can't handle normal mapped materials - eek
    if (pMaterialShader->GetLightingNormalSpace() == MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE)
        return false;

    // only support emissive materials
    if (pMaterialShader->GetLightingType() != MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool ParticleSystemSpriteVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/ParticleSystemSpriteVertexFactory.hlsl");

    if (vertexFactoryFlags & Flag_RenderBasic)
        pParameters->AddPreprocessorMacro("PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_RENDER_BASIC", "1");

    if (vertexFactoryFlags & Flag_RenderInstancedQuads)
        pParameters->AddPreprocessorMacro("PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_RENDER_INSTANCED_QUADS", "1");

    if (vertexFactoryFlags & Flag_UseWorldTransform)
        pParameters->AddPreprocessorMacro("PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_USE_WORLD_TRANSFORM", "1");

    return true;
}

uint32 ParticleSystemSpriteVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
{
    GPU_VERTEX_ELEMENT_DESC *pElementDesc = &pElementsDesc[0];
    uint32 nElements = 0;
    uint32 streamOffset = 0;

    if (flags & Flag_RenderBasic)
    {
        // position
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // texcoord
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float2);
        pElementDesc++;
        nElements++;

        // color
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_COLOR;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UNORM4;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;
    }
    else if (flags & Flag_RenderInstancedQuads)
    {
        // position
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float4);
        pElementDesc++;
        nElements++;

        // texcoord range
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float4);
        pElementDesc++;
        nElements++;

        // half size
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 1;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float2);
        pElementDesc++;
        nElements++;

        // color
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_COLOR;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UNORM4;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;
    }

    return nElements;
}

