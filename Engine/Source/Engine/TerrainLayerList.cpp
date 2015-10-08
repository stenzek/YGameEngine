#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainLayerList.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
#include "Renderer/Renderer.h"
#include "Core/ChunkFileReader.h"
Log_SetChannel(TerrainLayerList);

DEFINE_RESOURCE_TYPE_INFO(TerrainLayerList);
DEFINE_RESOURCE_GENERIC_FACTORY(TerrainLayerList);

TerrainLayerList::TerrainLayerList(const ResourceTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pBaseLayers(NULL),
      m_baseLayerArraySize(0),
      m_ppTextures(NULL),
      m_textureCount(0),
      m_combinedBaseLayerBaseTextureIndex(-1),
      m_combinedBaseLayerNormalTextureIndex(-1)
{

}

TerrainLayerList::~TerrainLayerList()
{
    delete[] m_pBaseLayers;

    for (uint32 i = 0; i < m_textureCount; i++)
    {
        if (m_ppTextures[i] != NULL)
            m_ppTextures[i]->Release();
    }
    delete[] m_ppTextures;
}

const TerrainLayerListBaseLayer *TerrainLayerList::GetBaseLayerByName(const char *name) const
{
    for (uint32 i = 0; i < m_baseLayerArraySize; i++)
    {
        if (m_pBaseLayers[i].Allocated && m_pBaseLayers[i].Name.Compare(name))
            return &m_pBaseLayers[i];
    }

    return NULL;
}

bool TerrainLayerList::Load(const char *FileName, ByteStream *pStream)
{
    PathString tempString;  

#define ABORTREASON(Reason) Log_ErrorPrintf("Could not load TerrainLayerList '%s': %s", FileName, Reason)

    // set name
    m_strName = FileName;

    // read header
    DF_TERRAIN_LAYER_LIST_HEADER header;
    if (!pStream->Read2(&header, sizeof(header)))
        return false;

    // validate header
    if (header.Magic != DF_TERRAIN_LAYER_LIST_HEADER_MAGIC ||
        header.HeaderSize != sizeof(header))
    {
        return false;
    }

    // init chunkloader
    ChunkFileReader chunkReader;
    if (!chunkReader.Initialize(pStream))
        return false;

    // load textures
    uint32 nTextures = header.TextureCount;
    if (nTextures > 0)
    {
        if (chunkReader.LoadChunk(DF_TERRAIN_LAYER_LIST_CHUNK_TEXTURES) && chunkReader.GetCurrentChunkSize() >= (sizeof(DF_TERRAIN_LAYER_LIST_TEXTURE) * nTextures))
        {
            m_ppTextures = new const Texture *[nTextures];
            m_textureCount = nTextures;
            Y_memzero(m_ppTextures, sizeof(const Texture *) * nTextures);

            const byte *pChunkBasePointer = (const byte *)chunkReader.GetCurrentChunkPointer();
            uint32 chunkSize = chunkReader.GetCurrentChunkSize();

            const DF_TERRAIN_LAYER_LIST_TEXTURE *pTextureHeader = (const DF_TERRAIN_LAYER_LIST_TEXTURE *)pChunkBasePointer;
            for (uint32 i = 0; i < nTextures; i++, pTextureHeader++)
            {
                // generate name
                tempString.Format("%s:%u", m_strName.GetCharArray(), i);

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
                TEXTURE_TYPE textureType = Texture::GetTextureTypeForStream(tempString, pTextureStream);
                if (textureType == TEXTURE_TYPE_COUNT)
                {
                    ABORTREASON("corrupted internal texture");
                    return false;
                }

                // create an internal texture
                Texture *pInternalTexture = Texture::CreateTextureObjectForType(textureType);
                DebugAssert(pInternalTexture != NULL);

                // load it
                if (!pInternalTexture->Load(tempString, pTextureStream))
                {
                    ABORTREASON("corrupted internal texture");
                    pInternalTexture->Release();
                    return false;
                }

                // store it
                m_ppTextures[i] = pInternalTexture;
            }
        }
        else
        {
            ABORTREASON("invalid texture chunk");
            return false;
        }
    }

    // load base layers
    uint32 baseLayerCount = header.BaseLayerCount;
    uint32 baseLayerArraySize = header.BaseLayerArraySize;
    if (baseLayerCount > 0 && chunkReader.LoadChunk(DF_TERRAIN_LAYER_LIST_CHUNK_BASE_LAYERS) && chunkReader.GetCurrentChunkTypeCount<DF_TERRAIN_LAYER_LIST_BASE_LAYER>() == baseLayerCount)
    {
        DebugAssert(baseLayerArraySize < TERRAIN_MAX_LAYERS);
        m_pBaseLayers = new TerrainLayerListBaseLayer[baseLayerArraySize];
        m_baseLayerArraySize = baseLayerArraySize;

        // init layers as blank
        for (uint32 i = 0; i < baseLayerArraySize; i++)
        {
            m_pBaseLayers[i].Index = i;
            m_pBaseLayers[i].Allocated = false;
            m_pBaseLayers[i].TextureRepeatInterval = 0;
            m_pBaseLayers[i].BaseTextureIndex = -1;
            m_pBaseLayers[i].BaseTextureArrayIndex = -1;
            m_pBaseLayers[i].NormalTextureIndex = -1;
            m_pBaseLayers[i].NormalTextureArrayIndex = -1;
        }

        // read them in
        const DF_TERRAIN_LAYER_LIST_BASE_LAYER *pSourceBaseLayer = chunkReader.GetCurrentChunkTypePointer<DF_TERRAIN_LAYER_LIST_BASE_LAYER>();
        for (uint32 i = 0; i < baseLayerCount; i++, pSourceBaseLayer++)
        {
            if (pSourceBaseLayer->LayerIndex >= baseLayerArraySize || m_pBaseLayers[pSourceBaseLayer->LayerIndex].Allocated)
            {
                ABORTREASON("invalid base layer index");
                return false;
            }

            // store fields
            TerrainLayerListBaseLayer *pDestinationBaseLayer = &m_pBaseLayers[pSourceBaseLayer->LayerIndex];
            pDestinationBaseLayer->Allocated = true;
            pDestinationBaseLayer->Name = chunkReader.GetStringByIndex(pSourceBaseLayer->NameStringIndex);
            pDestinationBaseLayer->TextureRepeatInterval = pSourceBaseLayer->TextureRepeatInterval;
            pDestinationBaseLayer->BaseTextureIndex = pSourceBaseLayer->BaseTextureIndex;
            pDestinationBaseLayer->BaseTextureArrayIndex = pSourceBaseLayer->BaseTextureArrayIndex;
            pDestinationBaseLayer->NormalTextureIndex = pSourceBaseLayer->NormalTextureIndex;
            pDestinationBaseLayer->NormalTextureArrayIndex = pSourceBaseLayer->NormalTextureArrayIndex;

            // validate data
            if ((pDestinationBaseLayer->BaseTextureIndex >= 0 && (uint32)pDestinationBaseLayer->BaseTextureIndex >= m_textureCount) ||
                (pDestinationBaseLayer->NormalTextureIndex >= 0 && (uint32)pDestinationBaseLayer->NormalTextureIndex >= m_textureCount))
            {
                ABORTREASON("invalid base layer texture index");
                return false;
            }
        }
    }
    else
    {
        ABORTREASON("invalid base layers chunk");
        return false;
    }

    // store fields
    m_combinedBaseLayerBaseTextureIndex = header.CombinedBaseLayerBaseTextureIndex;
    m_combinedBaseLayerNormalTextureIndex = header.CombinedBaseLayerNormalTextureIndex;

    // validate fields
    if ((m_combinedBaseLayerBaseTextureIndex >= 0 && (uint32)m_combinedBaseLayerBaseTextureIndex >= m_textureCount) ||
        (m_combinedBaseLayerNormalTextureIndex >= 0 && (uint32)m_combinedBaseLayerNormalTextureIndex >= m_textureCount))
    {
        ABORTREASON("invalid combined texture indices");
        return false;
    }

#undef ABORTREASON

    // create on gpu
    if (!CreateGPUResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool TerrainLayerList::CreateGPUResources()
{
    uint32 i;

    for (i = 0; i < m_textureCount; i++)
    {
        if (!m_ppTextures[i]->CreateDeviceResources())
            return false;
    }

    return true;
}

void TerrainLayerList::ReleaseGPUResources()
{

}

uint32 TerrainLayerList::GetFirstLayerIndex() const
{
    for (uint32 i = 0; i < m_baseLayerArraySize; i++)
    {
        if (m_pBaseLayers[i].Allocated)
            return i;
    }

    Panic("No base layers");
    return 0;
}

const Material *TerrainLayerList::CreateBaseLayerRenderMaterial(const int32 *pLayerIndices, uint32 nLayerIndices, GPUTexture2D *pNormalMapTexture, GPUTexture **pWeightTextures, uint32 nWeightTextures) const
{
    DebugAssert(nLayerIndices > 0 && nWeightTextures > 0);

    // expect a 2d texture and <= 4 weights
    if (nWeightTextures == 1 && pWeightTextures[0]->GetTextureType() == TEXTURE_TYPE_2D)
    {
        // get textures
        const Texture *pBaseTexture = (m_combinedBaseLayerBaseTextureIndex >= 0) ? m_ppTextures[m_combinedBaseLayerBaseTextureIndex] : nullptr;
        //const Texture *pNormalTexture = (m_combinedBaseLayerNormalTextureIndex >= 0) ? m_ppTextures[m_combinedBaseLayerNormalTextureIndex] : nullptr;

        // todo: handle normal maps
        AutoReleasePtr<const MaterialShader> pShader = g_pResourceManager->GetMaterialShader("shaders/engine/terrain/section_texture_array");
        if (pShader == nullptr)
            return nullptr;

        // create an instance of it
        Material *pMaterial = new Material();
        pMaterial->Create("<terrain section render material>", pShader);

        float4 uniformTerrainTextureArrayIndices;
        float4 uniformTerrainTextureArrayRepeatIntervals;

        for (uint32 j = 0; j < 4; j++)
        {
            if (j < nLayerIndices && pLayerIndices[j] >= 0 && (uint32)pLayerIndices[j] < m_baseLayerArraySize && m_pBaseLayers[pLayerIndices[j]].Allocated)
            {
                uniformTerrainTextureArrayIndices[j] = (float)pLayerIndices[j];
                uniformTerrainTextureArrayRepeatIntervals[j] = 1.0f / (float)m_pBaseLayers[pLayerIndices[j]].TextureRepeatInterval;
            }
            else
            {
                uniformTerrainTextureArrayIndices[j] = 0.0f;
                uniformTerrainTextureArrayRepeatIntervals[j] = 0.0f;
            }
        }

        pMaterial->SetShaderUniformParameterByName("TerrainTextureArrayIndices", SHADER_PARAMETER_TYPE_FLOAT4, &uniformTerrainTextureArrayIndices);
        pMaterial->SetShaderUniformParameterByName("TerrainTextureArrayRepeatIntervals", SHADER_PARAMETER_TYPE_FLOAT4, &uniformTerrainTextureArrayRepeatIntervals);
        pMaterial->SetShaderTextureParameterByName("NormalMap", pNormalMapTexture);
        pMaterial->SetShaderTextureParameterByName("BaseLayerAlphaMap", pWeightTextures[0]);
        pMaterial->SetShaderTextureParameterByName("BaseLayerDiffuseMap", pBaseTexture);

        // done
        return pMaterial;
    }

    // nothing created
    return nullptr;
}

