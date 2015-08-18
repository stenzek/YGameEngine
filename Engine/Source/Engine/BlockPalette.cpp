#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockPalette.h"
#include "Engine/StaticMesh.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
#include "Core/ChunkFileReader.h"
Log_SetChannel(BlockPalette);

BlockPalette::BlockPalette()
{
    uint32 i;
    BlockPalette::BlockType *pBlockType;

    for (i = 0; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        pBlockType = &m_BlockTypes[i];
        pBlockType->IsAllocated = false;
        pBlockType->BlockTypeIndex = i;
        pBlockType->Flags = 0;
        pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE;
        Y_memzero(pBlockType->CubeShapeFaces, sizeof(pBlockType->CubeShapeFaces));
        Y_memzero(&pBlockType->SlabShapeSettings, sizeof(pBlockType->SlabShapeSettings));
        Y_memzero(&pBlockType->PlaneShapeSettings, sizeof(pBlockType->PlaneShapeSettings));
        Y_memzero(&pBlockType->MeshShapeSettings, sizeof(pBlockType->MeshShapeSettings));
        Y_memzero(&pBlockType->BlockLightEmitterSettings, sizeof(pBlockType->BlockLightEmitterSettings));
        Y_memzero(&pBlockType->PointLightEmitterSettings, sizeof(pBlockType->PointLightEmitterSettings));
    }

    // setup blocktype 0
    pBlockType = &m_BlockTypes[0];
    pBlockType->Name = "___NONE___";
    pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE;
}

BlockPalette::~BlockPalette()
{
    uint32 i;

    for (i = 0; i < m_materials.GetSize(); i++)
    {
        const Material *pMaterial = m_materials[i];
        if (pMaterial != NULL)
            pMaterial->Release();
    }

    for (i = 0; i < m_textures.GetSize(); i++)
    {
        const Texture *pTexture = m_textures[i];
        if (pTexture != NULL)
            pTexture->Release();
    }

    for (i = 0; i < m_meshes.GetSize(); i++)
    {
        const StaticMesh *pStaticMesh = m_meshes[i];
        if (pStaticMesh != NULL)
            pStaticMesh->Release();
    }

}

bool BlockPalette::Load(const char *FileName, ByteStream *pStream)
{
    PathString tempString;  

#define ABORTREASON(Reason) Log_ErrorPrintf("Could not load BlockPalette '%s': %s", FileName, Reason)

    // set name
    m_strName = FileName;

    // read header
    DF_BLOCK_PALETTE_LIST_HEADER blockListHeader;
    if (!pStream->Read2(&blockListHeader, sizeof(blockListHeader)))
        return false;

    // validate header
    if (blockListHeader.Magic != DF_BLOCK_PALETTE_HEADER_MAGIC ||
        blockListHeader.HeaderSize != sizeof(blockListHeader))
    {
        return false;
    }

    // init chunkloader
    ChunkFileReader chunkReader;
    if (!chunkReader.Initialize(pStream))
        return false;

    // load block types
    uint32 nBlockTypes = blockListHeader.BlockTypeCount;
    if (nBlockTypes > 0 && chunkReader.LoadChunk(DF_BLOCK_PALETTE_CHUNK_BLOCK_TYPES) && chunkReader.GetCurrentChunkTypeCount<DF_BLOCK_PALETTE_BLOCK_TYPE>() == nBlockTypes)
    {
        const DF_BLOCK_PALETTE_BLOCK_TYPE *pSourceBlockType = chunkReader.GetCurrentChunkTypePointer<DF_BLOCK_PALETTE_BLOCK_TYPE>();
        for (uint32 i = 0; i < nBlockTypes; i++, pSourceBlockType++)
        {
            if (pSourceBlockType->BlockTypeIndex == 0 || pSourceBlockType->BlockTypeIndex >= BLOCK_MESH_MAX_BLOCK_TYPES ||
                m_BlockTypes[pSourceBlockType->BlockTypeIndex].IsAllocated)
            {
                ABORTREASON("invalid block type index");
                return false;
            }

            BlockPalette::BlockType *pDestinationBlockType = &m_BlockTypes[pSourceBlockType->BlockTypeIndex];
            pDestinationBlockType->IsAllocated = true;
            pDestinationBlockType->Name = chunkReader.GetStringByIndex(pSourceBlockType->NameStringIndex);
            pDestinationBlockType->Flags = pSourceBlockType->Flags;
            pDestinationBlockType->ShapeType = (BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE)pSourceBlockType->ShapeType;
            
            if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE ||
                pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB ||
                pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
            {
                if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                    pDestinationBlockType->SlabShapeSettings.Height = pSourceBlockType->SlabSettings.Height;

                for (uint32 j = 0; j < CUBE_FACE_COUNT; j++)
                {
                    const DF_BLOCK_PALETTE_BLOCK_TYPE::CubeShapeFace *inCubeFaceDef = &pSourceBlockType->CubeShapeFaces[j];
                    BlockPalette::BlockType::CubeShapeFace *outCubeFaceDef = &pDestinationBlockType->CubeShapeFaces[j];

                    outCubeFaceDef->Visual.Type = (BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE)inCubeFaceDef->Visual.Type;
                    outCubeFaceDef->Visual.MaterialIndex = inCubeFaceDef->Visual.MaterialIndex;
                    outCubeFaceDef->Visual.Color = inCubeFaceDef->Visual.Color;
                    outCubeFaceDef->Visual.MinUV.Load(inCubeFaceDef->Visual.MinUV);
                    outCubeFaceDef->Visual.MaxUV.Load(inCubeFaceDef->Visual.MaxUV);
                    outCubeFaceDef->Visual.AtlasUVRange.Load(inCubeFaceDef->Visual.AtlasUVRange);
                }
            }
            else if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
            {
                const DF_BLOCK_PALETTE_BLOCK_TYPE::PlaneShape &inPlaneSettings = pSourceBlockType->PlaneSettings;
                BlockPalette::BlockType::PlaneShape &outPlaneSettings = pDestinationBlockType->PlaneShapeSettings;

                outPlaneSettings.Visual.Type = (BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE)inPlaneSettings.Visual.Type;
                outPlaneSettings.Visual.MaterialIndex = inPlaneSettings.Visual.MaterialIndex;
                outPlaneSettings.Visual.Color = inPlaneSettings.Visual.Color;
                outPlaneSettings.Visual.MinUV.Load(inPlaneSettings.Visual.MinUV);
                outPlaneSettings.Visual.MaxUV.Load(inPlaneSettings.Visual.MaxUV);
                outPlaneSettings.Visual.AtlasUVRange.Load(inPlaneSettings.Visual.AtlasUVRange);

                outPlaneSettings.OffsetX = inPlaneSettings.OffsetX;
                outPlaneSettings.OffsetY = inPlaneSettings.OffsetY;
                outPlaneSettings.Width = inPlaneSettings.Width;
                outPlaneSettings.Height = inPlaneSettings.Height;

                outPlaneSettings.BaseRotation = inPlaneSettings.BaseRotation;
                outPlaneSettings.RepeatCount = inPlaneSettings.RepeatCount;
                outPlaneSettings.RepeatRotation = inPlaneSettings.RepeatRotation;
            }
            else if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
            {
                const DF_BLOCK_PALETTE_BLOCK_TYPE::MeshShape &inMeshSettings = pSourceBlockType->MeshSettings;
                BlockPalette::BlockType::MeshShape &outMeshSettings = pDestinationBlockType->MeshShapeSettings;

                outMeshSettings.MeshIndex = inMeshSettings.MeshIndex;
                outMeshSettings.Scale = inMeshSettings.Scale;
            }

            // block light emitter
            pDestinationBlockType->BlockLightEmitterSettings.Radius = pSourceBlockType->BlockLightEmitterSettings.Radius;

            // point light emitter
            pDestinationBlockType->PointLightEmitterSettings.Offset.Load(pSourceBlockType->PointLightEmitterSettings.Offset);
            pDestinationBlockType->PointLightEmitterSettings.Color = pSourceBlockType->PointLightEmitterSettings.Color;
            pDestinationBlockType->PointLightEmitterSettings.Brightness = pSourceBlockType->PointLightEmitterSettings.Brightness;
            pDestinationBlockType->PointLightEmitterSettings.Range = pSourceBlockType->PointLightEmitterSettings.Range;
            pDestinationBlockType->PointLightEmitterSettings.Falloff = pSourceBlockType->PointLightEmitterSettings.Falloff;
        }
    }
    else
    {
        ABORTREASON("invalid block list");
        return false;
    }

    // load textures
    uint32 nTextures = blockListHeader.TextureCount;
    if (nTextures > 0)
    {
        if (chunkReader.LoadChunk(DF_BLOCK_PALETTE_CHUNK_TEXTURES) && chunkReader.GetCurrentChunkSize() >= (sizeof(DF_BLOCK_PALETTE_TEXTURE) * nTextures))
        {
            m_textures.Resize(nTextures);
            Y_memzero(m_textures.GetBasePointer(), sizeof(const Texture *) * nTextures);

            const byte *pChunkBasePointer = (const byte *)chunkReader.GetCurrentChunkPointer();
            uint32 chunkSize = chunkReader.GetCurrentChunkSize();

            const DF_BLOCK_PALETTE_TEXTURE *pTextureHeader = (const DF_BLOCK_PALETTE_TEXTURE *)pChunkBasePointer;
            for (uint32 i = 0; i < nTextures; i++, pTextureHeader++)
            {
                // validate range
                if ((pTextureHeader->TextureOffset + pTextureHeader->TextureSize) > chunkSize ||
                    pTextureHeader->TextureSize == 0)
                {
                    ABORTREASON("corrupted texture chunk");
                    return false;
                }

                // create stream
                AutoReleasePtr<ByteStream> pTextureStream = ByteStream_CreateReadOnlyMemoryStream(pChunkBasePointer + pTextureHeader->TextureOffset, pTextureHeader->TextureSize);
                DebugAssert(pTextureStream != NULL);

                // work out texture type
                TEXTURE_TYPE textureType = Texture::GetTextureTypeForStream("BLOCKLIST_INTERNAL_TEXTURE", pTextureStream);
                if (textureType == TEXTURE_TYPE_COUNT)
                {
                    ABORTREASON("corrupted internal texture");
                    return false;
                }
                    
                // create an internal texture
                Texture *pInternalTexture = Texture::CreateTextureObjectForType(textureType);
                DebugAssert(pInternalTexture != NULL);

                // gen texture name
                tempString.Format("%s:InternalTexture%u", m_strName.GetCharArray(), i);

                // load it
                if (!pInternalTexture->Load(tempString, pTextureStream))
                {
                    ABORTREASON("corrupted internal texture");
                    pInternalTexture->Release();
                    return false;
                }

                // store it
                m_textures[i] = pInternalTexture;
            }
        }
        else
        {
            ABORTREASON("invalid texture chunk");
            return false;
        }
    }

    // load materials
    uint32 nMaterials = blockListHeader.MaterialCount;
    if (nMaterials > 0)
    {
        if (chunkReader.LoadChunk(DF_BLOCK_PALETTE_CHUNK_MATERIALS) && chunkReader.GetCurrentChunkTypeCount<DF_BLOCK_PALETTE_MATERIAL>() == nMaterials)
        {
            m_materials.Resize(nMaterials);
            Y_memzero(m_materials.GetBasePointer(), sizeof(const Material *) * nMaterials);

            const DF_BLOCK_PALETTE_MATERIAL *pSourceMaterial = chunkReader.GetCurrentChunkTypePointer<DF_BLOCK_PALETTE_MATERIAL>();
            for (uint32 i = 0; i < nMaterials; i++, pSourceMaterial++)
            {
                DebugAssert(pSourceMaterial->MaterialType < DF_BLOCK_PALETTE_MATERIAL_TYPE_COUNT);

                // validate everything is in range
                if ((pSourceMaterial->DiffuseMapTextureIndex != 0xFFFFFFFF && pSourceMaterial->DiffuseMapTextureIndex >= m_textures.GetSize()) ||
                    (pSourceMaterial->SpecularMapTextureIndex != 0xFFFFFFFF && pSourceMaterial->SpecularMapTextureIndex >= m_textures.GetSize()) ||
                    (pSourceMaterial->NormalMapTextureIndex != 0xFFFFFFFF && pSourceMaterial->NormalMapTextureIndex >= m_textures.GetSize()))
                {
                    ABORTREASON("internal material has out-of-range texture indices");
                    return false;
                }

                // lookup supershader
                if (pSourceMaterial->MaterialType == DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL)
                {
                    // external material
                    if ((m_materials[i] = g_pResourceManager->GetMaterial(chunkReader.GetStringByIndex(pSourceMaterial->MaterialNameStringIndex))) == NULL)
                        m_materials[i] = g_pResourceManager->GetDefaultMaterial();
                }
                else
                {
                    // autogen materials
                    // material name array
                    const char *materialNames[DF_BLOCK_PALETTE_MATERIAL_TYPE_COUNT] = {
                        NULL,                                                                           // DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL
                        "shaders/engine/block_mesh/color",                                              // DF_BLOCK_PALETTE_MATERIAL_TYPE_COLOR
                        "shaders/engine/block_mesh/translucent_color",                                  // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_COLOR
                        "shaders/engine/block_mesh/texture",                                            // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE
                        "shaders/engine/block_mesh/texture_array",                                      // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY
                        "shaders/engine/block_mesh/texture_atlas",                                      // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS
                        "shaders/engine/block_mesh/masked_texture",                                     // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE
                        "shaders/engine/block_mesh/masked_texture_array",                               // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY
                        "shaders/engine/block_mesh/masked_texture_atlas",                               // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS
                        "shaders/engine/block_mesh/translucent_texture",                                // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE
                        "shaders/engine/block_mesh/translucent_texture_array",                          // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY
                        "shaders/engine/block_mesh/translucent_texture_atlas",                          // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS
                        "shaders/engine/block_mesh/texture_with_normal_map",                            // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/texture_array_with_normal_map",                      // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/texture_atlas_with_normal_map",                      // DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/masked_texture_with_normal_map",                     // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/masked_texture_array_with_normal_map",               // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/masked_texture_atlas_with_normal_map",               // DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/translucent_texture_with_normal_map",                // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/translucent_texture_array_with_normal_map",          // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY_WITH_NORMAL_MAP
                        "shaders/engine/block_mesh/translucent_texture_atlas_with_normal_map",          // DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS_WITH_NORMAL_MAP
                    };

                    // get shader
                    AutoReleasePtr<const MaterialShader> pMaterialShader = g_pResourceManager->GetMaterialShader(materialNames[pSourceMaterial->MaterialType]);
                
                    // found?
                    if (pMaterialShader == NULL)
                    {
                        // use default material as a fallback
                        Log_WarningPrintf("Unable to load blockmesh material shader %s, using default", materialNames[pSourceMaterial->MaterialType]);
                        m_materials[i] = g_pResourceManager->GetDefaultMaterial();
                    }
                    else
                    {
                        // autogen material
                        Material *pInternalMaterial = new Material();
                        pInternalMaterial->Create(chunkReader.GetStringByIndex(pSourceMaterial->MaterialNameStringIndex), pMaterialShader);

                        // static switches
                        if (pSourceMaterial->MaterialFlags & DF_BLOCK_PALETTE_MATERIAL_FLAG_SCROLLED_TEXTURE)
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("EnableTextureScrolling", true);
                        else if (pSourceMaterial->MaterialFlags & DF_BLOCK_PALETTE_MATERIAL_FLAG_ANIMATED_TEXTURE)
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("EnableTextureAnimation", true);

                        // blend vertex colours on everything for now
                        pInternalMaterial->SetShaderStaticSwitchParameterByName("BlendVertexColors", true);

                        // bind textures
                        // diffuse
                        if (pSourceMaterial->DiffuseMapTextureIndex != 0xFFFFFFFF)
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseDiffuseMap", true);
                            pInternalMaterial->SetShaderTextureParameterByName("DiffuseMap", m_textures[pSourceMaterial->DiffuseMapTextureIndex]);
                        }
                        else
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseDiffuseMap", false);
                        }

                        // specular
                        if (pSourceMaterial->SpecularMapTextureIndex != 0xFFFFFFFF)
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseSpecularMap", true);
                            pInternalMaterial->SetShaderTextureParameterByName("SpecularMap", m_textures[pSourceMaterial->SpecularMapTextureIndex]);
                        }
                        else
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseSpecularMap", false);
                        }

                        // normal
                        if (pSourceMaterial->NormalMapTextureIndex != 0xFFFFFFFF)
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseNormalMap", true);
                            pInternalMaterial->SetShaderTextureParameterByName("NormalMap", m_textures[pSourceMaterial->NormalMapTextureIndex]);
                        }
                        else
                        {
                            pInternalMaterial->SetShaderStaticSwitchParameterByName("UseNormalMap", false);
                        }

                        // scroll vector
                        pInternalMaterial->SetShaderUniformParameterByName("ScrollVector", SHADER_PARAMETER_TYPE_FLOAT2, float2(pSourceMaterial->TextureScrollVector));

                        // store internal material
                        m_materials[i] = pInternalMaterial;
                    }
                }
            }
        }
        else
        {
            ABORTREASON("invalid material chunk");
            return false;
        }
    }

    // load meshes
    uint32 nMeshes = blockListHeader.MeshCount;
    if (nMeshes > 0)
    {
        if (chunkReader.LoadChunk(DF_BLOCK_PALETTE_CHUNK_MESHES) && chunkReader.GetCurrentChunkTypeCount<DF_BLOCK_PALETTE_MESH>() == nMeshes)
        {
            m_meshes.Resize(nMeshes);
            m_meshes.ZeroContents();

            const DF_BLOCK_PALETTE_MESH *pSourceMesh = chunkReader.GetCurrentChunkTypePointer<DF_BLOCK_PALETTE_MESH>();
            for (uint32 i = 0; i < nMeshes; i++, pSourceMesh++)
            {
                if ((m_meshes[i] = g_pResourceManager->GetStaticMesh(chunkReader.GetStringByIndex(pSourceMesh->MeshNameStringIndex))) == NULL)
                    m_meshes[i] = g_pResourceManager->GetDefaultStaticMesh();
            }
        }
        else
        {
            ABORTREASON("invalid mesh chunk");
            return false;
        }
    }

    // validate that everything points to valid ranges @todo move this to the compiler
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        const BlockPalette::BlockType *pBlockType = &m_BlockTypes[i];
        if (!pBlockType->IsAllocated)
            continue;

        switch (pBlockType->ShapeType)
        {
        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE:
            break;

        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB:
            {
                if (pBlockType->SlabShapeSettings.Height <= 0.0f || pBlockType->SlabShapeSettings.Height > 1.0f)
                {
                    ABORTREASON("invalid slab height");
                    return false;
                }
            }
            // break deliberately ommitted to fall through
            //break;

        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE:
        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS:
            {
                for (uint32 j = 0; j < CUBE_FACE_COUNT; j++)
                {
                    if (pBlockType->CubeShapeFaces[j].Visual.MaterialIndex >= m_materials.GetSize())
                    {
                        ABORTREASON("block type has out-of-range material");
                        return false;
                    }
                }
            }
            break;

        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE:
            {
                if (pBlockType->PlaneShapeSettings.Visual.MaterialIndex >= m_materials.GetSize())
                {
                    ABORTREASON("block type has out-of-range material");
                    return false;
                }
            }
            break;

        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH:
            {
                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH && pBlockType->MeshShapeSettings.MeshIndex >= m_meshes.GetSize())
                {
                    ABORTREASON("block type has out-of-range mesh");
                    return false;
                }
            }
            break;

        default:
            {
                ABORTREASON("unhandled block visual type");
                return false;
            }
            break;
        }
    }

#undef ABORTREASON

    return true;
}

const BlockPalette::BlockType *BlockPalette::GetBlockTypeByName(const char *name) const
{
    for (uint32 i = 0; i < countof(m_BlockTypes); i++)
    {
        if (m_BlockTypes[i].Name.Compare(name))
            return &m_BlockTypes[i];
    }

    return nullptr;
}

bool BlockPalette::CreateGPUResources() const
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (!m_textures[i]->CreateDeviceResources())
            return false;
    }

    for (uint32 i = 0; i < m_materials.GetSize(); i++)
    {
        if (!m_materials[i]->CreateDeviceResources())
            return false;
    }

    for (uint32 i = 0; i < m_meshes.GetSize(); i++)
    {
        if (!m_meshes[i]->CreateGPUResources())
            return false;
    }

    return true;
}

void BlockPalette::ReleaseGPUResources() const
{
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
        m_materials[i]->ReleaseDeviceResources();

    for (uint32 i = 0; i < m_textures.GetSize(); i++)
        m_textures[i]->ReleaseDeviceResources();
}

