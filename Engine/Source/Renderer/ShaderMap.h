#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"

class ShaderComponentTypeInfo;
class VertexFactoryTypeInfo;
class MaterialShader;
class ShaderProgram;

class ShaderMap
{
    struct Key
    {
        Key() {}
        Key(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
            : pBaseShaderTypeInfo(pBaseShaderTypeInfo), pVertexFactoryTypeInfo(pVertexFactoryTypeInfo), pMaterialShader(pMaterialShader), GlobalShaderFlags(globalShaderFlags), BaseShaderFlags(baseShaderFlags), VertexFactoryFlags(vertexFactoryFlags), MaterialShaderFlags(materialShaderFlags) {}

        const ShaderComponentTypeInfo *pBaseShaderTypeInfo;
        const VertexFactoryTypeInfo *pVertexFactoryTypeInfo;
        const MaterialShader *pMaterialShader;
        uint32 GlobalShaderFlags;
        uint32 BaseShaderFlags; 
        uint32 VertexFactoryFlags;
        uint32 MaterialShaderFlags;

        static int32 Compare(const Key *a, const Key *b);

        bool operator<(const Key &other) const { return (Compare(this, &other) < 0); }
    };

public:
    ShaderMap();
    ~ShaderMap();

    ShaderProgram *GetShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const;
    ShaderProgram *GetShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const;
    void ReleaseGPUResources();

private:
    ShaderProgram *LoadShaderPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes) const;

    typedef KeyValuePair<Key, ShaderProgram *> ProgramEntry;
    typedef MemArray<ProgramEntry> LoadedShaderArray;
    mutable LoadedShaderArray m_arrLoadedShaders;
};

