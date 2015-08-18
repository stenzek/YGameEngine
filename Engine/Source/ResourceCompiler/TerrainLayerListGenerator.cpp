#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/TerrainLayerListGenerator.h"
#include "ResourceCompiler/TextureGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/Engine.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
#include "Core/ChunkFileWriter.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "YBaseLib/ZipArchive.h"
Log_SetChannel(TerrainLayerListGenerator);

static const PIXEL_FORMAT TERRAIN_LAYER_LIST_INTERNAL_FORMAT = PIXEL_FORMAT_R8G8B8_UNORM;

TerrainLayerListGenerator::TerrainLayerListGenerator()
    : m_baseLayerNormalMapping(false),
      m_baseLayerBaseMapResolution(256),
      m_baseLayerNormalMapResolution(256)
{
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        m_baseLayers[i].Index = i;
        m_baseLayers[i].Allocated = false;
        m_baseLayers[i].TextureRepeatInterval = 0;
        m_baseLayers[i].pBaseMap = nullptr;
        m_baseLayers[i].pNormalMap = nullptr;
    }
}

TerrainLayerListGenerator::~TerrainLayerListGenerator()
{
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        delete m_baseLayers[i].pBaseMap;
        delete m_baseLayers[i].pNormalMap;
    }
}

void TerrainLayerListGenerator::Create(uint32 diffuseMapSize, uint32 normalMapSize)
{
    DebugAssert(diffuseMapSize > 0 && normalMapSize > 0);
    m_baseLayerBaseMapResolution = diffuseMapSize;
    m_baseLayerNormalMapResolution = normalMapSize;

    // create the default layer
    CreateBaseLayer("default");
}

void TerrainLayerListGenerator::CreateCopy(const TerrainLayerListGenerator *pGenerator)
{
    m_baseLayerNormalMapping = pGenerator->m_baseLayerNormalMapping;
    m_baseLayerBaseMapResolution = pGenerator->m_baseLayerBaseMapResolution;
    m_baseLayerNormalMapResolution = pGenerator->m_baseLayerNormalMapResolution;

    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        const BaseLayer *pSourceBaseLayer = &pGenerator->m_baseLayers[i];
        BaseLayer *pDestinationBaseLayer = &m_baseLayers[i];

        delete pDestinationBaseLayer->pBaseMap;
        delete pDestinationBaseLayer->pNormalMap;

        pDestinationBaseLayer->Allocated = pSourceBaseLayer->Allocated;
        if (pDestinationBaseLayer->Allocated)
        {
            pDestinationBaseLayer->Name = pSourceBaseLayer->Name;
            pDestinationBaseLayer->TextureRepeatInterval = pSourceBaseLayer->TextureRepeatInterval;

            DebugAssert(pSourceBaseLayer->pBaseMap != nullptr);
            pDestinationBaseLayer->pBaseMap = new Image();
            pDestinationBaseLayer->pBaseMap->Copy(*pSourceBaseLayer->pBaseMap);

            if (pSourceBaseLayer->pNormalMap != nullptr)
            {
                pDestinationBaseLayer->pNormalMap = new Image();
                pDestinationBaseLayer->pNormalMap->Copy(*pSourceBaseLayer->pNormalMap);
            }
            else
            {
                pDestinationBaseLayer->pNormalMap = nullptr;
            }
        }
        else
        {
            pDestinationBaseLayer->Name.Obliterate();
            pDestinationBaseLayer->TextureRepeatInterval = 0;
            pDestinationBaseLayer->pBaseMap = nullptr;
            pDestinationBaseLayer->pNormalMap = nullptr;
        }
    }
}

bool TerrainLayerListGenerator::Load(const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    pProgressCallbacks->SetStatusText("Opening archive...");

    // open zip archive
    ZipArchive *pArchive = ZipArchive::OpenArchiveReadOnly(pStream);
    if (pArchive == NULL)
    {
        Log_ErrorPrintf("TerrainLayerListGenerator::Load: Could not load '%s': Could not open as archive.", FileName);
        delete pArchive;
        return false;
    }

    // load data
    pProgressCallbacks->SetProgressRange(1);
    pProgressCallbacks->SetProgressValue(0);

    // load base layers
    {
        pProgressCallbacks->PushState();
        pProgressCallbacks->SetStatusText("Loading base layers...");

        if (!LoadBaseLayers(pArchive, pProgressCallbacks))
        {
            Log_ErrorPrintf("TerrainLayerListGenerator::Load: Could not load '%s': Could not load base layers.", FileName);
            pProgressCallbacks->PopState();
            delete pArchive;
            return false;
        }

        pProgressCallbacks->PopState();
        pProgressCallbacks->SetProgressValue(1);
    }

    // done
    delete pArchive;
    return true;
}

bool TerrainLayerListGenerator::LoadBaseLayers(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks)
{
    AutoReleasePtr<ByteStream> pBaseLayersStream = pArchive->OpenFile("baselayers.xml", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
    if (pBaseLayersStream == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::LoadBlockTypes: Could not open baselayers.xml.");
        return false;
    }

    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pBaseLayersStream, "baselayers.xml"))
    {
        xmlReader.PrintError("failed to create XML reader");
        return false;
    }

    // skip to correct node
    if (!xmlReader.SkipToElement("baselayers"))
    {
        xmlReader.PrintError("failed to skip to baselayers element");
        return false;
    }

    // read attributes
    {
        const char *baseMapSizeStr = xmlReader.FetchAttribute("base-map-resolution");
        const char *normalMapSizeStr = xmlReader.FetchAttribute("normal-map-resolution");
        const char *normalMapEnabledStr = xmlReader.FetchAttribute("normal-map-enabled");
        if (baseMapSizeStr == NULL || normalMapSizeStr == NULL || normalMapEnabledStr == NULL)
        {
            xmlReader.PrintError("missing texture size attributes");
            return false;
        }

        m_baseLayerNormalMapping = StringConverter::StringToBool(normalMapEnabledStr);
        m_baseLayerBaseMapResolution = StringConverter::StringToUInt32(baseMapSizeStr);
        m_baseLayerNormalMapResolution = StringConverter::StringToUInt32(normalMapSizeStr);
        if (m_baseLayerBaseMapResolution == 0 || m_baseLayerNormalMapResolution == 0)
        {
            xmlReader.PrintError("invalid texture size attributes");
            return false;
        }
    }

    // read nodes
    uint32 layerCount = 0;
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 baseLayersSelection = xmlReader.Select("baselayer");
                if (baseLayersSelection < 0)
                    return false;

                // read in all fields
                const char *layerIndex = xmlReader.FetchAttribute("index");
                const char *layerName = xmlReader.FetchAttribute("name");
                const char *textureRepeatIntervalStr = xmlReader.FetchAttribute("texture-repeat-interval");
                if (layerIndex == NULL || layerName == NULL || textureRepeatIntervalStr == NULL)
                {
                    xmlReader.PrintError("incomplete base layer declaration");
                    return false;
                }

                // check name isn't used
                if (GetBaseLayerIndexByName(layerName) >= 0)
                {
                    xmlReader.PrintError("layer name '%s' already used.", layerName);
                    return false;
                }

                // convert index to int
                uint32 intLayerIndex = StringConverter::StringToUInt32(layerIndex);
                if (intLayerIndex >= TERRAIN_MAX_LAYERS || m_baseLayers[intLayerIndex].Allocated)
                {
                    xmlReader.PrintError("layer index index out of range, or already used");
                    return false;
                }

                // fill in data
                BaseLayer *pBaseLayer = &m_baseLayers[intLayerIndex];
                pBaseLayer->Allocated = true;
                pBaseLayer->Name = layerName;
                pBaseLayer->TextureRepeatInterval = StringConverter::StringToUInt32(textureRepeatIntervalStr);
                layerCount++;

                // read the rest of the node
                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                    return false;

                // read textures
                if (!LoadBaseLayerTextures(pArchive, pBaseLayer, pProgressCallbacks))
                {
                    xmlReader.PrintError("failed to load any textures for this layer");
                    return false;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "baselayers") == 0);
                break;
            }
            else
            {
                UnreachableCode();
            }
        }
    }

    // should have at least one
    return (layerCount > 0);
}

bool TerrainLayerListGenerator::LoadBaseLayerTextures(ZipArchive *pArchive, BaseLayer *pBaseLayer, ProgressCallbacks *pProgressCallbacks)
{
    SmallString filename;

    // search for a diffuse map
    {
        filename.Format("baselayer_%u_base.png", pBaseLayer->Index);
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
            return false;

        ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(filename, pStream);
        if (pCodec == nullptr)
            return false;

        Image *pImage = new Image();
        if (!pCodec->DecodeImage(pImage, filename, pStream))
        {
            delete pImage;
            return false;
        }

        pBaseLayer->pBaseMap = pImage;
    }

    // search for a normal map
    {
        filename.Format("baselayer_%u_normal.png", pBaseLayer->Index);
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream != nullptr)
        {
            ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(filename, pStream);
            if (pCodec == nullptr)
                return false;

            Image *pImage = new Image();
            if (!pCodec->DecodeImage(pImage, filename, pStream))
            {
                delete pImage;
                return false;
            }

            pBaseLayer->pNormalMap = pImage;
        }
    }

    return true;
}

bool TerrainLayerListGenerator::Save(ByteStream *pStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */) const
{
    // open zip archive
    ZipArchive *pArchive = ZipArchive::CreateArchive(pStream);
    if (pArchive == nullptr)
    {
        Log_ErrorPrintf("TerrainLayerListGenerator::Save: Could not create archive");
        return false;
    }

    // save data
    pProgressCallbacks->SetProgressRange(2);
    pProgressCallbacks->SetProgressValue(0);

    // save base layers
    {
        pProgressCallbacks->PushState();
        pProgressCallbacks->SetStatusText("Saving base layers...");

        if (!SaveBaseLayers(pArchive, pProgressCallbacks))
        {
            Log_ErrorPrintf("TerrainLayerListGenerator::Save: Could not save base layers.");
            pProgressCallbacks->PopState();
            delete pArchive;
            return false;
        }

        pProgressCallbacks->PopState();
        pProgressCallbacks->SetProgressValue(1);
    }

    // commit archive
    {
        pProgressCallbacks->SetStatusText("Writing archive...");
        if (!pArchive->CommitChanges())
        {
            Log_ErrorPrintf("TerrainLayerListGenerator::Save: Could not commit archive");
            delete pArchive;
            return false;
        }

        pProgressCallbacks->SetProgressValue(2);
    }

    delete pArchive;
    return true;
}

bool TerrainLayerListGenerator::SaveBaseLayers(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const
{
    AutoReleasePtr<ByteStream> pBaseLayersStream = pArchive->OpenFile("baselayers.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    if (pBaseLayersStream == NULL)
        return false;

    // create xml writer
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pBaseLayersStream))
        return false;

    // write root element
    xmlWriter.StartElement("baselayers");
    {
        // write attributes
        xmlWriter.WriteAttribute("base-map-resolution", StringConverter::UInt32ToString(m_baseLayerBaseMapResolution));
        xmlWriter.WriteAttribute("normal-map-enabled", StringConverter::BoolToString(m_baseLayerNormalMapping));
        xmlWriter.WriteAttribute("normal-map-resolution", StringConverter::UInt32ToString(m_baseLayerNormalMapResolution));

        // write nodes
        for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
        {
            const BaseLayer *pBaseLayer = &m_baseLayers[i];
            if (!pBaseLayer->Allocated)
                continue;

            xmlWriter.StartElement("baselayer");
            {
                xmlWriter.WriteAttribute("index", StringConverter::UInt32ToString(i));
                xmlWriter.WriteAttribute("name", pBaseLayer->Name);
                xmlWriter.WriteAttribute("texture-repeat-interval", StringConverter::UInt32ToString(pBaseLayer->TextureRepeatInterval));
            }
            xmlWriter.EndElement();     // </baselayer>

            // write textures
            if (!SaveBaseLayerTextures(pArchive, pBaseLayer, pProgressCallbacks))
                return false;
        }
    }
    xmlWriter.EndElement();     // </baselayers>

    return (!xmlWriter.InErrorState() && !pBaseLayersStream->InErrorState());
}

bool TerrainLayerListGenerator::SaveBaseLayerTextures(ZipArchive *pArchive, const BaseLayer *pBaseLayer, ProgressCallbacks *pProgressCallbacks) const
{
    SmallString filename;
    DebugAssert(pBaseLayer->pBaseMap != nullptr);

    // search for a diffuse map
    {
        filename.Format("baselayer_%u_base.png", pBaseLayer->Index);
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
            return false;

        ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(filename, pStream);
        if (pCodec == nullptr)
            return false;

        if (!pCodec->EncodeImage(filename, pStream, pBaseLayer->pBaseMap))
            return false;
    }

    // search for a normal map
    if (pBaseLayer->pNormalMap != nullptr)
    {
        filename.Format("baselayer_%u_normal.png", pBaseLayer->Index);
        AutoReleasePtr<ByteStream> pStream = pArchive->OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
            return false;

        ImageCodec *pCodec = ImageCodec::GetImageCodecForStream(filename, pStream);
        if (pCodec == nullptr)
            return false;

        if (!pCodec->EncodeImage(filename, pStream, pBaseLayer->pNormalMap))
            return false;
    }

    return true;
}

int32 TerrainLayerListGenerator::GetBaseLayerIndexByName(const char *layerName) const
{
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        if (m_baseLayers[i].Allocated && m_baseLayers[i].Name.Compare(layerName))
            return (int32)i;
    }

    return -1;
}

const TerrainLayerListGenerator::BaseLayer *TerrainLayerListGenerator::GetBaseLayerByName(const char *layerName) const
{
    int32 layerIndex = GetBaseLayerIndexByName(layerName);
    return (layerIndex >= 0) ? &m_baseLayers[layerIndex] : nullptr;
}

TerrainLayerListGenerator::BaseLayer *TerrainLayerListGenerator::GetBaseLayerByName(const char *layerName)
{
    int32 layerIndex = GetBaseLayerIndexByName(layerName);
    return (layerIndex >= 0) ? &m_baseLayers[layerIndex] : nullptr;
}

TerrainLayerListGenerator::BaseLayer *TerrainLayerListGenerator::CreateBaseLayer(const char *layerName)
{
    Assert(GetBaseLayerIndexByName(layerName) < 0);
    
    uint32 index;
    for (index = 0; index < TERRAIN_MAX_LAYERS; index++)
    {
        if (!m_baseLayers[index].Allocated)
            break;
    }
    if (index == TERRAIN_MAX_LAYERS)
        return nullptr;

    BaseLayer *pBaseLayer = &m_baseLayers[index];
    pBaseLayer->Allocated = true;
    pBaseLayer->Name = layerName;
    pBaseLayer->pBaseMap = nullptr;
    pBaseLayer->pNormalMap = nullptr;
    pBaseLayer->TextureRepeatInterval = 1;

    // use a 1x1 white image for the base map
    pBaseLayer->pBaseMap = new Image();
    pBaseLayer->pBaseMap->Create(TERRAIN_LAYER_LIST_INTERNAL_FORMAT, 1, 1, 1);
    Y_memset(pBaseLayer->pBaseMap->GetData(), 0xFF, pBaseLayer->pBaseMap->GetDataSize());

    // done
    return pBaseLayer;
}

void TerrainLayerListGenerator::DeleteBaseLayer(uint32 index)
{
    Assert(index < TERRAIN_MAX_LAYERS && m_baseLayers[index].Allocated);
    
    BaseLayer *pBaseLayer = &m_baseLayers[index];
    delete pBaseLayer->pBaseMap;
    delete pBaseLayer->pNormalMap;

    pBaseLayer->Name.Obliterate();
    pBaseLayer->pBaseMap = nullptr;
    pBaseLayer->pNormalMap = nullptr;
    pBaseLayer->TextureRepeatInterval = 0;
}

bool TerrainLayerListGenerator::SetBaseLayerBaseMap(uint32 index, const Image *pImage)
{
    BaseLayer *pBaseLayer = &m_baseLayers[index];
    DebugAssert(pBaseLayer->Allocated);

    Image *pNewImage = nullptr;
    if (pImage != nullptr)
    {
        pNewImage = new Image();
        if (pImage->GetPixelFormat() != TERRAIN_LAYER_LIST_INTERNAL_FORMAT)
        {
            if (!pNewImage->CopyAndConvertPixelFormat(*pImage, TERRAIN_LAYER_LIST_INTERNAL_FORMAT))
            {
                delete pNewImage;
                return false;
            }
        }
        else
        {
            pNewImage->Copy(*pImage);
        }
    }

    delete pBaseLayer->pBaseMap;
    pBaseLayer->pBaseMap = pNewImage;

    return true;
}

bool TerrainLayerListGenerator::SetBaseLayerNormalMap(uint32 index, const Image *pImage)
{
    BaseLayer *pBaseLayer = &m_baseLayers[index];
    DebugAssert(pBaseLayer->Allocated);

    Image *pNewImage = nullptr;
    if (pImage != nullptr)
    {
        pNewImage = new Image();
        if (pImage->GetPixelFormat() != TERRAIN_LAYER_LIST_INTERNAL_FORMAT)
        {
            if (!pNewImage->CopyAndConvertPixelFormat(*pImage, TERRAIN_LAYER_LIST_INTERNAL_FORMAT))
            {
                delete pNewImage;
                return false;
            }
        }
        else
        {
            pNewImage->Copy(*pImage);
        }
    }

    delete pBaseLayer->pNormalMap;
    pBaseLayer->pNormalMap = pNewImage;

    return true;
}

class TerrainLayerListCompiler
{
public:
    TerrainLayerListCompiler(const TerrainLayerListGenerator *pGenerator);
    ~TerrainLayerListCompiler();

    bool Compile(ByteStream *pOutputStream);

private:
    struct TextureEntry
    {
        uint32 Index;
        TextureGenerator GeneratedTexture;
    };

    struct BaseLayerEntry
    {
        uint32 Index;
        String Name;
        uint32 TextureRepeatInterval;
        
        int32 BaseTextureIndex;
        int32 BaseTextureArrayIndex;

        int32 NormalTextureIndex;
        int32 NormalTextureArrayIndex;
    };

    bool CopyResizeInsertNullNormalMapTexture(uint32 resolution, TextureEntry *pDestinationTexture, int32 *pArrayIndex, int32 *pTextureIndex);
    bool CopyResizeInsertTexture(const Image *pImage, uint32 resolution, TextureEntry *pDestinationTexture, int32 *pArrayIndex, int32 *pTextureIndex);

    bool CompileBaseLayer(const TerrainLayerListGenerator::BaseLayer *pBaseLayer);

    bool GenerateTextureMipmaps();
    
    bool WriteHeader(ByteStream *pStream);
    bool WriteTextures(ByteStream *pStream, ChunkFileWriter &cfw);
    bool WriteBaseLayers(ByteStream *pStream, ChunkFileWriter &cfw);

    const TerrainLayerListGenerator *m_pGenerator;

    PODArray<TextureEntry *> m_textures;
    PODArray<BaseLayerEntry *> m_baseLayers;

    int32 m_baseLayerCombinedBaseTexture;
    int32 m_baseLayerCombinedNormalTexture;
    int32 m_baseLayerCombinedNormalTextureNullArrayIndex;
};

TerrainLayerListCompiler::TerrainLayerListCompiler(const TerrainLayerListGenerator *pGenerator)
    : m_pGenerator(pGenerator),
      m_baseLayerCombinedBaseTexture(-1),
      m_baseLayerCombinedNormalTexture(-1),
      m_baseLayerCombinedNormalTextureNullArrayIndex(-1)
{

}

TerrainLayerListCompiler::~TerrainLayerListCompiler()
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
        delete m_textures[i];

    for (uint32 i = 0; i < m_baseLayers.GetSize(); i++)
        delete m_baseLayers[i];
}

bool TerrainLayerListCompiler::Compile(ByteStream *pOutputStream)
{
    // add block types
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        const TerrainLayerListGenerator::BaseLayer *pBaseLayer = &m_pGenerator->m_baseLayers[i];
        if (!pBaseLayer->Allocated)
            continue;

        if (!CompileBaseLayer(pBaseLayer))
        {
            Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to compile base layer %u (%s)", i, pBaseLayer->Name.GetCharArray());
            return false;
        }
    }

    if (!GenerateTextureMipmaps())
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to generate texture mipmaps.");
        return false;
    }

    if (!WriteHeader(pOutputStream))
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to write header.");
        return false;
    }

    // initialize chunk file writer
    ChunkFileWriter cfw;
    if (!cfw.Initialize(pOutputStream, DF_TERRAIN_LAYER_LIST_CHUNK_COUNT))
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to initialize chunk file writer.");
        return false;
    }

    if (!WriteTextures(pOutputStream, cfw))
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to write textures.");
        return false;
    }

    if (!WriteBaseLayers(pOutputStream, cfw))
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to write base layers.");
        return false;
    }

    // close chunk writer
    if (!cfw.Close() || pOutputStream->InErrorState())
    {
        Log_ErrorPrintf("TerrainLayerListCompiler::Compile: Failed to close chunk file writer.");
        return false;
    }

    Log_DevPrintf("TerrainLayerListCompiler::Compile: Layer list compiled, size = %u bytes", (uint32)pOutputStream->GetSize());
    return true;
}

bool TerrainLayerListCompiler::CopyResizeInsertNullNormalMapTexture(uint32 resolution, TextureEntry *pDestinationTexture, int32 *pArrayIndex, int32 *pTextureIndex)
{
    // create a 1x1 rgb8 image, with the z set to 1
    Image tempImage;
    tempImage.Create(TERRAIN_LAYER_LIST_INTERNAL_FORMAT, 1, 1, 1);
    tempImage.GetData()[0] = 0x00;
    tempImage.GetData()[1] = 0x00;
    tempImage.GetData()[2] = 0xFF;

    // pass through
    return CopyResizeInsertTexture(&tempImage, resolution, pDestinationTexture, pArrayIndex, pTextureIndex);
}

bool TerrainLayerListCompiler::CopyResizeInsertTexture(const Image *pImage, uint32 resolution, TextureEntry *pDestinationTexture, int32 *pArrayIndex, int32 *pTextureIndex)
{
    // texture exists?
    bool isFirstImage = false;
    if (pDestinationTexture == nullptr)
    {
        // create it
        pDestinationTexture = new TextureEntry();
        pDestinationTexture->Index = m_textures.GetSize();
        m_textures.Add(pDestinationTexture);
        isFirstImage = true;

        // create the actual texture
        if (!pDestinationTexture->GeneratedTexture.Create(TEXTURE_TYPE_2D_ARRAY, TERRAIN_LAYER_LIST_INTERNAL_FORMAT, resolution, resolution, 1, 1))
            return false;

        // set properties on it
        pDestinationTexture->GeneratedTexture.SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_WRAP);
        pDestinationTexture->GeneratedTexture.SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_WRAP);
        pDestinationTexture->GeneratedTexture.SetGenerateMipmaps(true);
    }

    // ensure it's in the correct format
    Image *pTempImage = new Image();
    if (pImage->GetPixelFormat() == TERRAIN_LAYER_LIST_INTERNAL_FORMAT)
    {
        // just copy it in
        pTempImage->Copy(*pImage);
    }
    else
    {
        if (!pTempImage->CopyAndConvertPixelFormat(*pImage, TERRAIN_LAYER_LIST_INTERNAL_FORMAT))
        {
            delete pTempImage;
            return false;
        }
    }

    // ensure it's the correct dimensions
    if ((pTempImage->GetWidth() != resolution || pTempImage->GetHeight() != resolution) &&
        !pTempImage->Resize(IMAGE_RESIZE_FILTER_LANCZOS3, resolution, resolution, 1))
    {
        delete pTempImage;
        return false;
    }

    // insert it into the texture
    bool result;
    int32 arrayIndex;
    if (isFirstImage)
    {
        arrayIndex = 0;
        result = pDestinationTexture->GeneratedTexture.SetImage(0, pTempImage);
    }
    else
    {
        arrayIndex = pDestinationTexture->GeneratedTexture.GetArraySize();
        result = pDestinationTexture->GeneratedTexture.AddImage(pTempImage);
    }

    delete pTempImage;
    if (!result)
        return false;

    // store results
    *pArrayIndex = arrayIndex;
    *pTextureIndex = pDestinationTexture->Index;
    return true;
}

bool TerrainLayerListCompiler::CompileBaseLayer(const TerrainLayerListGenerator::BaseLayer *pBaseLayer)
{
    BaseLayerEntry *pBaseLayerEntry = new BaseLayerEntry();
    pBaseLayerEntry->Index = pBaseLayer->Index;
    pBaseLayerEntry->Name = pBaseLayer->Name;
    pBaseLayerEntry->TextureRepeatInterval = pBaseLayer->TextureRepeatInterval;
    pBaseLayerEntry->BaseTextureIndex = -1;
    pBaseLayerEntry->BaseTextureArrayIndex = -1;
    pBaseLayerEntry->NormalTextureIndex = -1;
    pBaseLayerEntry->NormalTextureArrayIndex = -1;
    m_baseLayers.Add(pBaseLayerEntry);

    // is normal mapping enabled?
    if (m_pGenerator->IsBaseLayerNormalMappingEnabled())
    {
        // does this layer even have a normal map?
        if (pBaseLayer->pNormalMap != nullptr)
        {
            // add it to the combined texture
            if (m_baseLayerCombinedNormalTexture >= 0)
            {
                // add it to the combined texture
                if (!CopyResizeInsertTexture(pBaseLayer->pNormalMap, m_pGenerator->m_baseLayerNormalMapResolution, m_textures[m_baseLayerCombinedNormalTexture], &pBaseLayerEntry->NormalTextureArrayIndex, &m_baseLayerCombinedNormalTexture))
                    return false;
            }
            else
            {
                // create the combined texture
                if (!CopyResizeInsertTexture(pBaseLayer->pNormalMap, m_pGenerator->m_baseLayerNormalMapResolution, nullptr, &pBaseLayerEntry->NormalTextureArrayIndex, &m_baseLayerCombinedNormalTexture))
                    return false;
            }

            pBaseLayerEntry->NormalTextureIndex = m_baseLayerCombinedNormalTexture;
        }
        else
        {
            // use a 1x1 normal map with z pointing out
            if (m_baseLayerCombinedNormalTextureNullArrayIndex < 0)
            {
                // has combined texture?
                if (m_baseLayerCombinedNormalTexture >= 0)
                {
                    // add it to the combined texture
                    if (!CopyResizeInsertNullNormalMapTexture(m_pGenerator->m_baseLayerNormalMapResolution, m_textures[m_baseLayerCombinedNormalTexture], &m_baseLayerCombinedNormalTextureNullArrayIndex, &m_baseLayerCombinedNormalTexture))
                        return false;
                }
                else
                {
                    // create the combined texture
                    if (!CopyResizeInsertNullNormalMapTexture(m_pGenerator->m_baseLayerNormalMapResolution, nullptr, &m_baseLayerCombinedNormalTextureNullArrayIndex, &m_baseLayerCombinedNormalTexture))
                        return false;
                }
            }

            pBaseLayerEntry->NormalTextureIndex = m_baseLayerCombinedNormalTexture;
            pBaseLayerEntry->NormalTextureArrayIndex = m_baseLayerCombinedNormalTextureNullArrayIndex;
        }
    }

    // add the base layer to the combined texture
    if (m_baseLayerCombinedBaseTexture >= 0)
    {
        // add it to the combined texture
        if (!CopyResizeInsertTexture(pBaseLayer->pBaseMap, m_pGenerator->m_baseLayerNormalMapResolution, m_textures[m_baseLayerCombinedBaseTexture], &pBaseLayerEntry->BaseTextureArrayIndex, &m_baseLayerCombinedBaseTexture))
            return false;
    }
    else
    {
        // create the combined texture
        if (!CopyResizeInsertTexture(pBaseLayer->pBaseMap, m_pGenerator->m_baseLayerNormalMapResolution, nullptr, &pBaseLayerEntry->BaseTextureArrayIndex, &m_baseLayerCombinedBaseTexture))
            return false;
    }

    // store the texture index
    pBaseLayerEntry->BaseTextureIndex = m_baseLayerCombinedBaseTexture;   
    return true;
}

bool TerrainLayerListCompiler::GenerateTextureMipmaps()
{
    uint32 i;

    for (i = 0; i < m_textures.GetSize(); i++)
    {
        TextureEntry *pSourceTextureEntry = m_textures[i];
        TextureGenerator &sourceTextureGenerator = pSourceTextureEntry->GeneratedTexture;

        if (!sourceTextureGenerator.GenerateMipmaps())
            return false;
    }

    return true;
}

bool TerrainLayerListCompiler::WriteHeader(ByteStream *pStream)
{
    // determine array size
    uint32 baseLayerArraySize = 0;
    uint32 baseLayerCount = 0;
    for (uint32 i = 0; i < TERRAIN_MAX_LAYERS; i++)
    {
        if (m_pGenerator->m_baseLayers[i].Allocated)
        {
            baseLayerArraySize = i + 1;
            baseLayerCount++;
        }
    }

    // write it out to the file
    DF_TERRAIN_LAYER_LIST_HEADER header;
    header.Magic = DF_TERRAIN_LAYER_LIST_HEADER_MAGIC;
    header.HeaderSize = sizeof(header);
    header.TextureCount = m_textures.GetSize();
    header.BaseLayerArraySize = baseLayerArraySize;
    header.BaseLayerCount = baseLayerCount;
    header.CombinedBaseLayerBaseTextureIndex = m_baseLayerCombinedBaseTexture;
    header.CombinedBaseLayerNormalTextureIndex = m_baseLayerCombinedNormalTexture;
    if (!pStream->Write2(&header, sizeof(header)))
        return false;

    return true;
}

bool TerrainLayerListCompiler::WriteTextures(ByteStream *pStream, ChunkFileWriter &cfw)
{
    // for textures
    cfw.BeginChunk(DF_TERRAIN_LAYER_LIST_CHUNK_TEXTURES);
    if (m_textures.GetSize() > 0)
    {
        ByteStream **ppGeneratedTextureStreams = new ByteStream *[m_textures.GetSize()];
        uint32 nGeneratedTextureStreams = 0;
        uint32 nUsedTextureStreams = 0;

        for (uint32 i = 0; i < m_textures.GetSize(); i++)
        {
            Log_DevPrintf("Compiling terrain layer list internal texture %u...", i);

            // TEMP
            m_textures[i]->GeneratedTexture.SetEnableTextureCompression(false);

            ppGeneratedTextureStreams[nGeneratedTextureStreams] = ByteStream_CreateGrowableMemoryStream(NULL, 0);
            if (!m_textures[nGeneratedTextureStreams]->GeneratedTexture.Compile(ppGeneratedTextureStreams[nGeneratedTextureStreams]))
            {
                delete[] ppGeneratedTextureStreams;
                return false;
            }

            nGeneratedTextureStreams++;
        }

        uint32 currentTextureOffset = sizeof(DF_TERRAIN_LAYER_LIST_TEXTURE) * m_textures.GetSize();
        for (uint32 i = 0; i < m_textures.GetSize(); i++)
        {
            DF_TERRAIN_LAYER_LIST_TEXTURE outTexture;

            outTexture.TextureOffset = currentTextureOffset;
            outTexture.TextureSize = (uint32)ppGeneratedTextureStreams[nUsedTextureStreams]->GetSize();
            currentTextureOffset += outTexture.TextureSize;
            nUsedTextureStreams++;

            cfw.WriteChunkData(&outTexture, sizeof(outTexture));
        }

        // write texture data
        DebugAssert(nUsedTextureStreams == nGeneratedTextureStreams);
        for (uint32 i = 0; i < nUsedTextureStreams; i++)
        {
            static const uint32 CHUNKSIZE = 4096;
            byte buffer[CHUNKSIZE];

            ByteStream *pTextureStream = ppGeneratedTextureStreams[i];
            uint32 remaining = (uint32)pTextureStream->GetSize();
            pTextureStream->SeekAbsolute(0);

            while (remaining > 0)
            {
                uint32 toCopy = Min(CHUNKSIZE, remaining);
                if (!pTextureStream->Read2(buffer, toCopy))
                {
                    delete[] ppGeneratedTextureStreams;
                    return false;
                }

                remaining -= toCopy;
                cfw.WriteChunkData(buffer, toCopy);
            }

            pTextureStream->Release();
        }
        delete[] ppGeneratedTextureStreams;
    }
    cfw.EndChunk();

    return true;
}

bool TerrainLayerListCompiler::WriteBaseLayers(ByteStream *pStream, ChunkFileWriter &cfw)
{
    // for block types
    cfw.BeginChunk(DF_TERRAIN_LAYER_LIST_CHUNK_BASE_LAYERS);
    for (uint32 i = 0; i < m_baseLayers.GetSize(); i++)
    {
        const BaseLayerEntry *inBaseLayer = m_baseLayers[i];
        DF_TERRAIN_LAYER_LIST_BASE_LAYER outBaseLayer;

        outBaseLayer.LayerIndex = inBaseLayer->Index;
        outBaseLayer.NameStringIndex = cfw.AddString(inBaseLayer->Name);
        outBaseLayer.TextureRepeatInterval = inBaseLayer->TextureRepeatInterval;
        outBaseLayer.BaseTextureIndex = inBaseLayer->BaseTextureIndex;
        outBaseLayer.BaseTextureArrayIndex = inBaseLayer->BaseTextureArrayIndex;
        outBaseLayer.NormalTextureIndex = inBaseLayer->NormalTextureIndex;
        outBaseLayer.NormalTextureArrayIndex = inBaseLayer->NormalTextureArrayIndex;

        cfw.WriteChunkData(&outBaseLayer, sizeof(outBaseLayer));
    }
    cfw.EndChunk();

    return true;
}

bool TerrainLayerListGenerator::Compile(ByteStream *pOutputStream) const
{
    TerrainLayerListCompiler compiler(this);
    return compiler.Compile(pOutputStream);
}

// Interface
BinaryBlob *ResourceCompiler::CompileTerrainLayerList(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.layerlist.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileTerrainLayerList: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    TerrainLayerListGenerator *pGenerator = new TerrainLayerListGenerator();
    if (!pGenerator->Load(sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    pStream->Release();
    pSourceData->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pOutputStream))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
