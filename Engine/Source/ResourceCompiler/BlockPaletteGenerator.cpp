#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/BlockPaletteGenerator.h"
#include "ResourceCompiler/TextureGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/Engine.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "YBaseLib/ZipArchive.h"
#include "Core/ChunkFileWriter.h"
#include "Core/ImageCodec.h"
Log_SetChannel(BlockPaletteGenerator);

static const PIXEL_FORMAT BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;
static const IMAGE_RESIZE_FILTER BLOCK_MESH_BLOCK_LIST_TEXTURE_RESIZE_FILTER = IMAGE_RESIZE_FILTER_LANCZOS3;
static const uint32 BLOCK_MESH_BLOCK_LIST_TEXTURE_COMPRESSION_LEVEL = 6;

namespace NameTables {
    Y_Define_NameTable(BlockMeshTextureBlending)
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_BLENDING_NONE, "none")
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_BLENDING_MASKED, "masked")
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_BLENDING_TRANSLUCENT, "translucent")
    Y_NameTable_End()
    Y_Define_NameTable(BlockMeshTextureEffect)
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_EFFECT_NONE, "none")
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_EFFECT_SCROLL, "scroll")
        Y_NameTable_VEntry(BLOCK_MESH_TEXTURE_EFFECT_ANIMATION, "animation")
    Y_NameTable_End()
}

static void ResetBlockType(BlockPaletteGenerator::BlockType *pBlockType)
{
    pBlockType->Name.Obliterate();
    pBlockType->Flags = 0;
    pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE;

    for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
    {
        BlockPaletteGenerator::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[i];
        pFaceDef->Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
        pFaceDef->Visual.Color = 0xFFFFFFFF;
        pFaceDef->Visual.TextureIndex = 0xFFFFFFFF;
        pFaceDef->Visual.MaterialName.Obliterate();
    }

    pBlockType->SlabShapeSettings.Height = 1.0f;

    pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
    pBlockType->PlaneShapeSettings.Visual.Color = 0xFFFFFFFF;
    pBlockType->PlaneShapeSettings.Visual.TextureIndex = 0xFFFFFFFF;
    pBlockType->PlaneShapeSettings.Visual.MaterialName.Obliterate();
    pBlockType->PlaneShapeSettings.OffsetX = 0.0f;
    pBlockType->PlaneShapeSettings.OffsetY = 0.0f;
    pBlockType->PlaneShapeSettings.BaseRotation = 0;
    pBlockType->PlaneShapeSettings.RepeatCount = 0;
    pBlockType->PlaneShapeSettings.RepeatRotation = 0;

    pBlockType->MeshShapeSettings.Scale = 1.0f;

    pBlockType->BlockLightEmitterSettings.Enabled = false;
    pBlockType->BlockLightEmitterSettings.Radius = 0;

    pBlockType->PointLightEmitterSettings.Enabled = false;
    pBlockType->PointLightEmitterSettings.Offset.SetZero();
    pBlockType->PointLightEmitterSettings.Color = 0;
    pBlockType->PointLightEmitterSettings.Brightness = 0.0f;
    pBlockType->PointLightEmitterSettings.Range = 0.0f;
    pBlockType->PointLightEmitterSettings.Falloff = 0.0f;
}

BlockPaletteGenerator::BlockPaletteGenerator()
    : m_diffuseMapSize(0),
      m_specularMapSize(0),
      m_normalMapSize(0)
{
    // reset blocks
    for (uint32 i = 0; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        m_blockTypes[i].BlockTypeIndex = i;
        ResetBlockType(&m_blockTypes[i]);
        m_allocatedBlockTypes[i] = false;        
    }
}

BlockPaletteGenerator::~BlockPaletteGenerator()
{
    uint32 i;

    for (i = 0; i < m_textures.GetSize(); i++)
    {
        Texture *pTexture = m_textures[i];
        
        delete pTexture->pDiffuseMap;
        delete pTexture->pSpecularMap;
        delete pTexture->pNormalMap;
    }
}

void BlockPaletteGenerator::Create(uint32 diffuseMapSize, uint32 specularMapSize, uint32 normalMapSize)
{
    // setup default/none block
    m_allocatedBlockTypes[0] = true;
    m_diffuseMapSize = diffuseMapSize;
    m_specularMapSize = specularMapSize;
    m_normalMapSize = normalMapSize;
}

bool BlockPaletteGenerator::Load(const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    pProgressCallbacks->SetStatusText("Opening archive...");
    pProgressCallbacks->SetProgressRange(3);
    pProgressCallbacks->SetProgressValue(0);

    // open zip archive
    ZipArchive *pArchive = ZipArchive::OpenArchiveReadOnly(pStream);
    if (pArchive == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Load: Could not load '%s': Could not open as archive.", FileName);
        delete pArchive;
        return false;
    }

    pProgressCallbacks->SetProgressValue(1);
    pProgressCallbacks->SetStatusText("Loading textures...");
    pProgressCallbacks->PushState();

    // load textures from archive
    if (!LoadTextures(pArchive, pProgressCallbacks))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Load: Could not load '%s': Could not load textures.", FileName);
        pProgressCallbacks->PopState();
        delete pArchive;
        return false;
    }

    pProgressCallbacks->PopState();
    pProgressCallbacks->SetProgressValue(2);
    pProgressCallbacks->SetStatusText("Loading block types...");

    // load xml
    if (!LoadXML(pArchive, pProgressCallbacks))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Load: Could not load '%s': Could not load XML.", FileName);
        delete pArchive;
        return false;
    }

    pProgressCallbacks->SetProgressValue(3);

    delete pArchive;
    return true;
}

bool BlockPaletteGenerator::LoadTextures(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks)
{
    SmallString textureName;
    PathString textureFileName;
    FileSystem::FindResultsArray findResults;
    pArchive->FindFiles("textures", "*.xml", FILESYSTEM_FIND_RELATIVE_PATHS, &findResults);

    pProgressCallbacks->SetProgressRange(findResults.GetSize());
    pProgressCallbacks->SetProgressValue(0);

    for (uint32 i = 0; i < findResults.GetSize(); i++)
    {
        const FILESYSTEM_FIND_DATA &findData = findResults[i];
        pProgressCallbacks->SetFormattedStatusText("Loading texture from '%s'", findData.FileName);

        // parse texture name from the filename
        textureName.Assign(findData.FileName);
        DebugAssert(textureName.EndsWith(".xml", false));
        textureName.Erase(-4);

        // texture object
        Texture *pTexture = new Texture();
        pTexture->Index = m_textures.GetSize();
        pTexture->Name = textureName;
        pTexture->Blending = BLOCK_MESH_TEXTURE_BLENDING_NONE;
        pTexture->Effect = BLOCK_MESH_TEXTURE_EFFECT_NONE;
        pTexture->AllowTextureFiltering = true;
        Y_memzero(pTexture->ScrollDirection, sizeof(pTexture->ScrollDirection));
        pTexture->ScrollSpeed = 0.0f;
        pTexture->AnimationFrameCount = 0;
        pTexture->AnimationSpeed = 0.0f;
        pTexture->pDiffuseMap = nullptr;
        pTexture->pNormalMap = nullptr;
        pTexture->pSpecularMap = nullptr;
        m_textures.Add(pTexture);

        // parse the xml file
        {
            // open it
            textureFileName.Format("textures/%s.xml", textureName.GetCharArray());
            AutoReleasePtr<ByteStream> pXMLStream = pArchive->OpenFile(textureFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
            if (pXMLStream == nullptr)
            {
                Log_ErrorPrintf("BlockPaletteGenerator::LoadTextures: Could not open file '%s'.", textureFileName.GetCharArray());
                return false;
            }

            // parse it
            XMLReader xmlReader;
            if (!xmlReader.Create(pXMLStream, findData.FileName) || !xmlReader.SkipToElement("block-palette-texture"))
            {
                Log_ErrorPrintf("BlockPaletteGenerator::LoadTextures: Could not parse file '%s'.", textureFileName.GetCharArray());
                return false;
            }

            // read elements
            if (!xmlReader.IsEmptyElement())
            {
                for (;;)
                {
                    if (!xmlReader.NextToken())
                        return false;

                    if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                    {
                        int32 textureSelection = xmlReader.Select("blending|effect|filtering");
                        if (textureSelection < 0)
                            return false;

                        switch (textureSelection)
                        {
                            // blending
                        case 0:
                            {
                                const char *blendingTypeStr = xmlReader.FetchAttribute("type");
                                if (blendingTypeStr == nullptr || !NameTable_TranslateType(NameTables::BlockMeshTextureBlending, blendingTypeStr, &pTexture->Blending, true))
                                {
                                    xmlReader.PrintError("missing type attribute");
                                    return false;
                                }
                            }
                            break;

                            // effect
                        case 1:
                            {
                                const char *effectTypeStr = xmlReader.FetchAttribute("type");
                                if (effectTypeStr == nullptr || !NameTable_TranslateType(NameTables::BlockMeshTextureEffect, effectTypeStr, &pTexture->Effect, true))
                                {
                                    xmlReader.PrintError("missing type attribute");
                                    return false;
                                }

                                if (pTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_SCROLL)
                                {
                                    const char *scrollDirectionStr = xmlReader.FetchAttribute("scroll-direction");
                                    const char *scrollSpeedStr = xmlReader.FetchAttribute("scroll-speed");
                                    if (scrollDirectionStr == nullptr || scrollSpeedStr == nullptr)
                                    {
                                        xmlReader.PrintError("missing scroll attributes");
                                        return false;
                                    }

                                    StringConverter::StringToFloat2(scrollSpeedStr).Store(pTexture->ScrollDirection);
                                    pTexture->ScrollSpeed = StringConverter::StringToFloat(scrollSpeedStr);
                                }
                                else if (pTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_ANIMATION)
                                {
                                    const char *animationFrameCountStr = xmlReader.FetchAttribute("animation-frames");
                                    const char *animationSpeedStr = xmlReader.FetchAttribute("animation-speed");
                                    if (animationFrameCountStr == nullptr || animationSpeedStr == nullptr)
                                    {
                                        xmlReader.PrintError("missing animation attributes");
                                        return false;
                                    }

                                    pTexture->AnimationFrameCount = StringConverter::StringToUInt32(animationFrameCountStr);
                                    pTexture->AnimationSpeed = StringConverter::StringToFloat(animationSpeedStr);
                                }

                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                    return false;
                            }
                            break;

                            // filtering
                        case 2:
                            {
                                const char *enabledStr = xmlReader.FetchAttributeDefault("enabled", "false");
                                pTexture->AllowTextureFiltering = StringConverter::StringToBool(enabledStr);
                            }
                            break;
                        }                        
                    }
                    else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                    {
                        DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "block-palette-texture") == 0);
                        break;
                    }
                    else
                    {
                        UnreachableCode();
                    }
                }
            }
        }

        // get images
        ByteStream *pImageStream;
        
        // diffuse
        textureFileName.Format("textures/%s_diffuse.png", pTexture->Name.GetCharArray());
        if ((pImageStream = pArchive->OpenFile(textureFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE)) != nullptr)
        {
            if ((pTexture->pDiffuseMap = LoadTextureImage(pImageStream, textureFileName)) == nullptr)
            {
                pImageStream->Release();
                return false;
            }

            pImageStream->Release();
        }

        // specular
        textureFileName.Format("textures/%s_specular.png", pTexture->Name.GetCharArray());
        if ((pImageStream = pArchive->OpenFile(textureFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE)) != nullptr)
        {
            if ((pTexture->pSpecularMap = LoadTextureImage(pImageStream, textureFileName)) == nullptr)
            {
                pImageStream->Release();
                return false;
            }

            pImageStream->Release();
        }

        // normal
        textureFileName.Format("textures/%s_normal.png", pTexture->Name.GetCharArray());
        if ((pImageStream = pArchive->OpenFile(textureFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE)) != nullptr)
        {
            if ((pTexture->pNormalMap = LoadTextureImage(pImageStream, textureFileName)) == nullptr)
            {
                pImageStream->Release();
                return false;
            }

            pImageStream->Release();
        }

        // did we get any images?
        if (pTexture->pDiffuseMap == nullptr && 
            pTexture->pSpecularMap == nullptr && 
            pTexture->pNormalMap == nullptr)
        {
            Log_ErrorPrintf("BlockPaletteGenerator::LoadTextures: No texture images loaded for texture '%s'", textureName.GetCharArray());
            return false;
        }

        pProgressCallbacks->IncrementProgressValue();
    }

    return true;
}

Image *BlockPaletteGenerator::LoadTextureImage(ByteStream *pStream, const char *filename)
{
    Image *pDecodedImage = new Image();
    ImageCodec *pImageCodec = ImageCodec::GetImageCodecForStream(filename, pStream);
    if (pImageCodec == NULL || !pImageCodec->DecodeImage(pDecodedImage, filename, pStream))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::LoadTextureImage: Could not decode texture file name '%s'.", filename);
        delete pDecodedImage;
        return nullptr;
    }

    // check image format
    if (pDecodedImage->GetPixelFormat() != BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT && !pDecodedImage->ConvertPixelFormat(BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT))
    {
        Log_ErrorPrintf("BlockMeshBlockListGenerator::LoadTextures: Could not convert texture file name '%s' to internal format.", filename);
        delete pDecodedImage;
        return nullptr;
    }

    return pDecodedImage;
}

bool BlockPaletteGenerator::LoadXML(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks)
{
    AutoReleasePtr<ByteStream> pBlockTypesStream = pArchive->OpenFile("palette.xml", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
    if (pBlockTypesStream == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::LoadBlockTypes: Could not open palette.xml.");
        return false;
    }

    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pBlockTypesStream, "palette.xml"))
    {
        xmlReader.PrintError("failed to create XML reader");
        return false;
    }

    // skip to correct node
    if (!xmlReader.SkipToElement("palette"))
    {
        xmlReader.PrintError("failed to skip to palette element");
        return false;
    }

    // read attributes
    {
        const char *diffuseMapSizeStr = xmlReader.FetchAttribute("diffuse-map-size");
        const char *specularMapSizeStr = xmlReader.FetchAttribute("specular-map-size");
        const char *normalMapSizeStr = xmlReader.FetchAttribute("normal-map-size");
        if (diffuseMapSizeStr == NULL || specularMapSizeStr == NULL || normalMapSizeStr == NULL)
        {
            xmlReader.PrintError("missing texture size attributes");
            return false;
        }

        m_diffuseMapSize = StringConverter::StringToUInt32(diffuseMapSizeStr);
        m_specularMapSize = StringConverter::StringToUInt32(specularMapSizeStr);
        m_normalMapSize = StringConverter::StringToUInt32(normalMapSizeStr);
        if (m_diffuseMapSize == 0 || m_specularMapSize == 0 || m_normalMapSize == 0)
        {
            xmlReader.PrintError("invalid texture size attributes");
            return false;
        }
    }

    if (!xmlReader.IsEmptyElement())
    {      
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 blockTypesSelection = xmlReader.Select("blocktype");
                if (blockTypesSelection < 0)
                    return false;

                switch (blockTypesSelection)
                {
                    // blocktype
                case 0:
                    {
                        // read in all fields
                        const char *blockTypeIndex = xmlReader.FetchAttribute("index");
                        const char *blockTypeName = xmlReader.FetchAttribute("name");
                        if (blockTypeIndex == NULL || blockTypeName == NULL || xmlReader.IsEmptyElement())
                        {
                            xmlReader.PrintError("incomplete block type declaration");
                            return false;
                        }

                        // check name isn't used
                        if (GetBlockTypeByName(blockTypeName) != nullptr)
                        {
                            xmlReader.PrintError("block name '%s' already used.", blockTypeName);
                            return false;
                        }

                        // convert index to int
                        uint32 intBlockTypeIndex = StringConverter::StringToUInt32(blockTypeIndex);
                        if (intBlockTypeIndex == 0 || intBlockTypeIndex >= BLOCK_MESH_MAX_BLOCK_TYPES)
                        {
                            xmlReader.PrintError("block type index out of range.");
                            return false;
                        }

                        // check for allocation
                        if (m_allocatedBlockTypes[intBlockTypeIndex])
                        {
                            xmlReader.PrintError("double declaration of blocktype %u, ignoring duplicate declaration", intBlockTypeIndex);
                            continue;
                        }

                        // get blocktype ptr
                        BlockType *pBlockType = &m_blockTypes[intBlockTypeIndex];
                        m_allocatedBlockTypes[intBlockTypeIndex] = true;

                        // set up blocktype ptr
                        pBlockType->Name = blockTypeName;
                        pBlockType->Flags = 0;

                        // read rest of element
                        if (!xmlReader.IsEmptyElement())
                        {                                    
                            // read remaining info
                            for (;;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 blockTypeSelection = xmlReader.Select("flags|shape|block-light-emitter|point-light-emitter");
                                    if (blockTypeSelection < 0)
                                        return false;

                                    switch (blockTypeSelection)
                                    {
                                        // flags
                                    case 0:
                                        {
                                            if (!xmlReader.IsEmptyElement())
                                            {
                                                for (;;)
                                                {
                                                    if (!xmlReader.NextToken())
                                                        return false;

                                                    if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                    {
                                                        int32 flagsSelection = xmlReader.Select("solid|cast-shadows");
                                                        if (flagsSelection < 0)
                                                            return false;

                                                        switch (flagsSelection)
                                                        {
                                                            // solid
                                                        case 0:
                                                            pBlockType->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_SOLID;
                                                            break;

                                                            // cast-shadows
                                                        case 1:
                                                            pBlockType->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS;
                                                            break;
                                                        }

                                                        if (!xmlReader.SkipCurrentElement())
                                                            return false;
                                                    }
                                                    else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                    {
                                                        DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "flags") == 0);
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        // end case flags
                                        break;

                                        // shape
                                    case 1:
                                        {
                                            const char *shapeType = xmlReader.FetchAttribute("type");
                                            if (shapeType == NULL)
                                            {
                                                xmlReader.PrintError("invalid shape type");
                                                return false;
                                            }

                                            if (Y_stricmp(shapeType, "none") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE;
                                            }
                                            else if (Y_stricmp(shapeType, "cube") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE;
                                            }
                                            else if (Y_stricmp(shapeType, "slab") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB;

                                                const char *slabHeightStr = xmlReader.FetchAttribute("height");
                                                if (slabHeightStr == nullptr)
                                                {
                                                    xmlReader.PrintError("missing attributes");
                                                    return false;
                                                }

                                                pBlockType->SlabShapeSettings.Height = StringConverter::StringToFloat(slabHeightStr);
                                            }
                                            else if (Y_stricmp(shapeType, "stairs") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS;
                                            }
                                            else if (Y_stricmp(shapeType, "plane") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE;
                                            }
                                            else if (Y_stricmp(shapeType, "mesh") == 0)
                                            {
                                                pBlockType->ShapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH;
                                            }
                                            else
                                            {
                                                xmlReader.PrintError("invalid shape type '%s'", shapeType);
                                                return false;
                                            }

                                            // none visuals can skip the faces element
                                            if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE ||
                                                pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB ||
                                                pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
                                            {
                                                if (xmlReader.IsEmptyElement())
                                                {
                                                    xmlReader.PrintError("incomplete cube visual declaration");
                                                    return false;
                                                }

                                                bool facesDefined[6];
                                                Y_memzero(facesDefined, sizeof(facesDefined));
                                            
                                                for (;;)
                                                {
                                                    if (!xmlReader.NextToken())
                                                        return false;

                                                    if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                    {
                                                        int32 shapeSelection = xmlReader.Select("flags|faces");
                                                        if (shapeSelection < 0)
                                                            return false;

                                                        switch (shapeSelection)
                                                        {
                                                            // flags
                                                        case 0:
                                                            {
                                                                if (!xmlReader.IsEmptyElement())
                                                                {
                                                                    for (;;)
                                                                    {
                                                                        if (!xmlReader.NextToken())
                                                                            return false;

                                                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                                        {
                                                                            int32 flagsSelection = xmlReader.Select("volume|untileable");
                                                                            if (flagsSelection < 0)
                                                                                return false;

                                                                            switch (flagsSelection)
                                                                            {
                                                                                // volume
                                                                            case 0:
                                                                                pBlockType->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME;
                                                                                break;

                                                                                // untileable
                                                                            case 1:
                                                                                pBlockType->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_UNTILEABLE;
                                                                                break;
                                                                            }

                                                                            if (!xmlReader.SkipCurrentElement())
                                                                                return false;
                                                                        }
                                                                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                                        {
                                                                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "flags") == 0);
                                                                            break;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                            break;

                                                            // faces
                                                        case 1:
                                                            {
                                                                if (!xmlReader.IsEmptyElement())
                                                                {
                                                                    for (;;)
                                                                    {
                                                                        if (!xmlReader.NextToken())
                                                                            return false;

                                                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                                        {
                                                                            int32 faceIndex = xmlReader.Select("right|left|back|front|top|bottom");
                                                                            if (faceIndex < 0)
                                                                                return false;

                                                                            DebugAssert(faceIndex < CUBE_FACE_COUNT);

                                                                            // read attributes
                                                                            const char *visualTypeStr = xmlReader.FetchAttribute("visual");
                                                                            const char *colorStr = xmlReader.FetchAttribute("color");
                                                                            const char *textureNameStr = xmlReader.FetchAttribute("texture-name");
                                                                            const char *materialNameStr = xmlReader.FetchAttribute("material-name");
                                                                            if (visualTypeStr == NULL || colorStr == NULL)
                                                                            {
                                                                                xmlReader.PrintError("missing visual type");
                                                                                return false;
                                                                            }

                                                                            BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[faceIndex];
                                                                            pFaceDef->Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COUNT;
                                                                            pFaceDef->Visual.Color = StringConverter::StringToColor(colorStr);
                                                                            pFaceDef->Visual.TextureIndex = 0xFFFFFFFF;

                                                                            // type specific stuff
                                                                            if (Y_stricmp(visualTypeStr, "color") == 0)
                                                                            {
                                                                                pFaceDef->Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
                                                                            }
                                                                            else if (Y_stricmp(visualTypeStr, "texture") == 0)
                                                                            {
                                                                                if (textureNameStr == NULL)
                                                                                {
                                                                                    xmlReader.PrintError("texture-name is required.");
                                                                                    return false;
                                                                                }

                                                                                for (uint32 i = 0; i < m_textures.GetSize(); i++)
                                                                                {
                                                                                    if (m_textures[i]->Name.Compare(textureNameStr))
                                                                                    {
                                                                                        pFaceDef->Visual.TextureIndex = i;
                                                                                        break;
                                                                                    }
                                                                                }

                                                                                if (pFaceDef->Visual.TextureIndex == 0xFFFFFFFF)
                                                                                {
                                                                                    xmlReader.PrintError("texture named '%s' not found", textureNameStr);
                                                                                    return false;
                                                                                }

                                                                                pFaceDef->Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE;
                                                                            }
                                                                            else if (Y_stricmp(visualTypeStr, "material") == 0)
                                                                            {
                                                                                if (materialNameStr == nullptr)
                                                                                {
                                                                                    xmlReader.PrintError("material-name is required");
                                                                                    return false;
                                                                                }

                                                                                pFaceDef->Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL;
                                                                                pFaceDef->Visual.MaterialName = materialNameStr;
                                                                            }
                                                                            else
                                                                            {
                                                                                xmlReader.PrintError("unknown visual type '%s'", visualTypeStr);
                                                                                return false;
                                                                            }

                                                                            facesDefined[faceIndex] = true;
                                                                        }
                                                                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                                        {
                                                                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "faces") == 0);
                                                                            break;
                                                                        }
                                                                    }   // end loop over faces
                                                                }
                                                            }
                                                            break;
                                                        }
                                                    }
                                                    else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                    {
                                                        DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "shape") == 0);
                                                        break;
                                                    }
                                                }

                                                // check faces are defined
                                                for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
                                                {
                                                    if (!facesDefined[i])
                                                    {
                                                        xmlReader.PrintError("face not defined: %u", i);
                                                        return true;
                                                    }
                                                }
                                            }
                                            else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                                            {
                                                if (xmlReader.IsEmptyElement())
                                                {
                                                    xmlReader.PrintError("incomplete plane visual declaration");
                                                    return false;
                                                }

                                                // skip to settings element
                                                if (!xmlReader.NextToken() || xmlReader.Select("settings") < 0)
                                                    return false;

                                                // read attributes
                                                const char *visualTypeStr = xmlReader.FetchAttribute("visual");
                                                const char *colorStr = xmlReader.FetchAttribute("color");
                                                const char *textureNameStr = xmlReader.FetchAttribute("texture-name");
                                                const char *materialNameStr = xmlReader.FetchAttribute("material-name");
                                                const char *xOffsetStr = xmlReader.FetchAttribute("offset-x");
                                                const char *yOffsetStr = xmlReader.FetchAttribute("offset-y");
                                                const char *widthStr = xmlReader.FetchAttribute("width");
                                                const char *heightStr = xmlReader.FetchAttribute("height");
                                                const char *baseRotationStr = xmlReader.FetchAttribute("base-rotation");
                                                const char *repeatCountStr = xmlReader.FetchAttribute("repeat-count");
                                                const char *repeatRotationStr = xmlReader.FetchAttribute("repeat-rotation");
                                                if (visualTypeStr == NULL || colorStr == NULL || xOffsetStr == NULL || yOffsetStr == NULL ||
                                                    widthStr == NULL || heightStr == NULL || baseRotationStr == NULL || repeatCountStr == NULL || repeatRotationStr == NULL)
                                                {
                                                    xmlReader.PrintError("missing attributes");
                                                    return false;
                                                }

                                                pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COUNT;
                                                pBlockType->PlaneShapeSettings.Visual.Color = StringConverter::StringToColor(colorStr);
                                                pBlockType->PlaneShapeSettings.Visual.TextureIndex = 0xFFFFFFFF;

                                                if (Y_stricmp(visualTypeStr, "color") == 0)
                                                {
                                                    pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
                                                }
                                                else if (Y_stricmp(visualTypeStr, "texture") == 0)
                                                {
                                                    if (textureNameStr == NULL)
                                                    {
                                                        xmlReader.PrintError("texture-name is required.");
                                                        return false;
                                                    }

                                                    for (uint32 i = 0; i < m_textures.GetSize(); i++)
                                                    {
                                                        if (m_textures[i]->Name.Compare(textureNameStr))
                                                        {
                                                            pBlockType->PlaneShapeSettings.Visual.TextureIndex = i;
                                                            break;
                                                        }
                                                    }

                                                    if (pBlockType->PlaneShapeSettings.Visual.TextureIndex == 0xFFFFFFFF)
                                                    {
                                                        xmlReader.PrintError("texture named '%s' not found", textureNameStr);
                                                        return false;
                                                    }

                                                    pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE;
                                                }
                                                else if (Y_stricmp(visualTypeStr, "material") == 0)
                                                {
                                                    if (materialNameStr == nullptr)
                                                    {
                                                        xmlReader.PrintError("material-name is required");
                                                        return false;
                                                    }

                                                    pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL;
                                                    pBlockType->PlaneShapeSettings.Visual.MaterialName = materialNameStr;
                                                }
                                                else
                                                {
                                                    xmlReader.PrintError("unknown visual type '%s'", visualTypeStr);
                                                    return false;
                                                }

                                                pBlockType->PlaneShapeSettings.OffsetX = StringConverter::StringToFloat(xOffsetStr);
                                                pBlockType->PlaneShapeSettings.OffsetY = StringConverter::StringToFloat(yOffsetStr);
                                                pBlockType->PlaneShapeSettings.Width = StringConverter::StringToFloat(widthStr);
                                                pBlockType->PlaneShapeSettings.Height = StringConverter::StringToFloat(heightStr);
                                                pBlockType->PlaneShapeSettings.BaseRotation = StringConverter::StringToFloat(baseRotationStr);
                                                pBlockType->PlaneShapeSettings.RepeatCount = StringConverter::StringToUInt32(repeatCountStr);
                                                pBlockType->PlaneShapeSettings.RepeatRotation = StringConverter::StringToFloat(repeatRotationStr);

                                                // skip the rest of the settings element
                                                if (!xmlReader.SkipCurrentElement() || !xmlReader.NextToken())
                                                    return false;

                                                // should be the end of the shape element
                                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "shape") == 0);
                                            }
                                            else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
                                            {
                                                // skip to settings element
                                                if (!xmlReader.NextToken() || xmlReader.Select("settings") < 0)
                                                    return false;

                                                // read attributes
                                                const char *nameStr = xmlReader.FetchAttribute("name");
                                                const char *scaleStr = xmlReader.FetchAttribute("scale");
                                                if (nameStr == NULL || scaleStr == NULL)
                                                {
                                                    xmlReader.PrintError("missing attributes");
                                                    return false;
                                                }

                                                // insert mesh name and scale
                                                pBlockType->MeshShapeSettings.MeshName = nameStr;
                                                pBlockType->MeshShapeSettings.Scale = StringConverter::StringToFloat(scaleStr);

                                                // skip the rest of the settings element
                                                if (!xmlReader.SkipCurrentElement() || !xmlReader.NextToken())
                                                    return false;

                                                // should be the end of the shape element
                                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "shape") == 0);
                                            }
                                            else
                                            {
                                                // skip the rest of the shape element
                                                if (!xmlReader.SkipCurrentElement())
                                                    return false;
                                            }
                                        }
                                        // end case shape
                                        break;

                                        // block-light-emitter
                                    case 2:
                                        {
                                            const char *enabledStr = xmlReader.FetchAttribute("enabled");
                                            const char *radiusStr = xmlReader.FetchAttribute("radius");
                                            if (enabledStr == NULL || radiusStr == NULL)
                                            {
                                                xmlReader.PrintError("missing attributes");
                                                return false;
                                            }

                                            // set properties
                                            pBlockType->BlockLightEmitterSettings.Enabled = StringConverter::StringToBool(enabledStr);
                                            pBlockType->BlockLightEmitterSettings.Radius = StringConverter::StringToUInt32(radiusStr);
                                        }
                                        // end case point-light-emitter
                                        break;

                                        // point-light-emitter
                                    case 3:
                                        {
                                            const char *enabledStr = xmlReader.FetchAttribute("enabled");
                                            const char *offsetStr = xmlReader.FetchAttribute("offset");
                                            const char *colorStr = xmlReader.FetchAttribute("color");
                                            const char *brightnessStr = xmlReader.FetchAttribute("brightness");
                                            const char *rangeStr = xmlReader.FetchAttribute("range");
                                            const char *falloffStr = xmlReader.FetchAttribute("falloff");
                                            if (enabledStr == NULL || offsetStr == NULL || colorStr == NULL || brightnessStr == NULL || rangeStr == NULL || falloffStr == NULL)
                                            {
                                                xmlReader.PrintError("missing attributes");
                                                return false;
                                            }

                                            // set properties
                                            pBlockType->PointLightEmitterSettings.Enabled = StringConverter::StringToBool(enabledStr);
                                            pBlockType->PointLightEmitterSettings.Offset = StringConverter::StringToFloat3(offsetStr);
                                            pBlockType->PointLightEmitterSettings.Color = StringConverter::StringToColor(colorStr);
                                            pBlockType->PointLightEmitterSettings.Brightness = StringConverter::StringToFloat(brightnessStr);
                                            pBlockType->PointLightEmitterSettings.Range = StringConverter::StringToFloat(rangeStr);
                                            pBlockType->PointLightEmitterSettings.Falloff = StringConverter::StringToFloat(falloffStr);
                                        }
                                        // end case point-light-emitter
                                        break;


                                    default:
                                        UnreachableCode();
                                        break;
                                    }
                                }
                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                {
                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "blocktype") == 0);
                                    break;
                                }
                                else
                                {
                                    UnreachableCode();
                                }
                            }   // end blocktype loop
                        }
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "palette") == 0);
                break;
            }
            else
            {
                UnreachableCode();
            }
        }
    }

    /*
    // validate texture atlas indices
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        BlockType *pBlockType = &m_BlockTypes[i];
        if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        {
            for (uint32 j = 0; j < CUBE_FACE_COUNT; j++)
            {
                const BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[j];
                if (pFaceDef->VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_STATIC_TEXTURE ||
                    pFaceDef->VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_HORIZONTAL_SCROLL_TEXTURE ||
                    pFaceDef->VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_VERTICAL_SCROLL_TEXTURE)
                {
                    if (pFaceDef->TextureIndex >= m_Textures.GetSize())
                    {
                        xmlReader.PrintError("block %u face %u has texture index out of range.", i, j);
                        return false;
                    }
                }
            }
        }
        else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
        {
            if (pBlockType->PlaneShapeSettings.VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_STATIC_TEXTURE ||
                pBlockType->PlaneShapeSettings.VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_HORIZONTAL_SCROLL_TEXTURE ||
                pBlockType->PlaneShapeSettings.VisualType == BLOCK_MESH_BLOCK_TYPE_CUBE_VISUAL_TYPE_VERTICAL_SCROLL_TEXTURE)
            {
                if (pBlockType->PlaneShapeSettings.TextureIndex >= m_Textures.GetSize())
                {
                    xmlReader.PrintError("block %u plane has texture index out of range.", i);
                    return false;
                }
            }
        }
    }
    */
    return true;
}

bool BlockPaletteGenerator::Save(ByteStream *pStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */) const
{
    pProgressCallbacks->SetStatusText("Opening archive...");
    pProgressCallbacks->SetProgressRange(4);
    pProgressCallbacks->SetProgressValue(0);

    ZipArchive *pArchive = ZipArchive::CreateArchive(pStream);
    if (pArchive == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Save: Could not create archive.");
        delete pArchive;
        return false;
    }

    pProgressCallbacks->SetProgressValue(1);
    pProgressCallbacks->SetStatusText("Saving textures...");
    pProgressCallbacks->PushState();

    // load textures from archive
    if (!SaveTextures(pArchive, pProgressCallbacks))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Save: Could not save textures.");
        pProgressCallbacks->PopState();
        delete pArchive;
        return false;
    }

    pProgressCallbacks->PopState();
    pProgressCallbacks->SetProgressValue(2);
    pProgressCallbacks->SetStatusText("Saving block types...");

    // load xml
    if (!SaveXML(pArchive, pProgressCallbacks))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Save: Could not save block types.");
        delete pArchive;
        return false;
    }

    pProgressCallbacks->SetProgressValue(3);
    pProgressCallbacks->SetStatusText("Committing changes to archive...");

    if (!pArchive->CommitChanges())
    {
        Log_ErrorPrintf("BlockPaletteGenerator::Save: Could not commit changes.");
        delete pArchive;
        return false;
    }

    pProgressCallbacks->SetProgressValue(4);

    delete pArchive;
    return true;
}

bool BlockPaletteGenerator::SaveTextures(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const
{
    if (m_textures.GetSize() == 0)
        return true;

    pProgressCallbacks->SetProgressRange(m_textures.GetSize());
    pProgressCallbacks->SetProgressValue(0);

    // iterate textures
    PathString textureFileName;
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        Texture *pTexture = m_textures[i];
        pProgressCallbacks->SetFormattedStatusText("Saving texture '%s'...", pTexture->Name.GetCharArray());

        // create texture xml
        textureFileName.Format("textures/%s.xml", pTexture->Name.GetCharArray());
        {
            ByteStream *pXMLStream = pArchive->OpenFile(textureFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_STREAMED);
            if (pXMLStream == nullptr)
                return false;

            XMLWriter xmlWriter;
            if (!xmlWriter.Create(pXMLStream))
            {
                pXMLStream->Release();
                return false;
            }

            xmlWriter.StartElement("block-palette-texture");
            {
                xmlWriter.StartElement("blending");
                {
                    xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::BlockMeshTextureBlending, pTexture->Blending));
                }
                xmlWriter.EndElement();
                xmlWriter.StartElement("effect");
                {
                    xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::BlockMeshTextureEffect, pTexture->Effect));

                    if (pTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_SCROLL)
                    {
                        xmlWriter.WriteAttribute("scroll-direction", StringConverter::Float2ToString(pTexture->ScrollDirection));
                        xmlWriter.WriteAttribute("scroll-speed", StringConverter::FloatToString(pTexture->ScrollSpeed));
                    }
                    else if (pTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_ANIMATION)
                    {
                        xmlWriter.WriteAttribute("animation-frames", StringConverter::UInt32ToString(pTexture->AnimationFrameCount));
                        xmlWriter.WriteAttribute("animation-speed", StringConverter::FloatToString(pTexture->AnimationSpeed));
                    }
                }
                xmlWriter.EndElement();
                xmlWriter.StartElement("filtering");
                xmlWriter.WriteAttribute("enabled", StringConverter::BoolToString(pTexture->AllowTextureFiltering));
                xmlWriter.EndElement();
            }
            xmlWriter.EndElement();

            if (xmlWriter.InErrorState() || pXMLStream->InErrorState())
            {
                xmlWriter.Close();
                pXMLStream->Release();
                return false;
            }
            xmlWriter.Close();
            pXMLStream->Release();
        }
        
        if (pTexture->pDiffuseMap != nullptr)
        {
            textureFileName.Format("textures/%s_diffuse.png", pTexture->Name.GetCharArray());
            if (!SaveTexture(pArchive, pTexture->pDiffuseMap, textureFileName))
                return false;
        }

        if (pTexture->pSpecularMap != nullptr)
        {
            textureFileName.Format("textures/%s_specular.png", pTexture->Name.GetCharArray());
            if (!SaveTexture(pArchive, pTexture->pSpecularMap, textureFileName))
                return false;
        }

        if (pTexture->pNormalMap != nullptr)
        {
            textureFileName.Format("textures/%s_normal.png", pTexture->Name.GetCharArray());
            if (!SaveTexture(pArchive, pTexture->pNormalMap, textureFileName))
                return false;
        }

        pProgressCallbacks->IncrementProgressValue();
    }

    return true;
}

bool BlockPaletteGenerator::SaveTexture(ZipArchive *pArchive, const Image *pImage, const char *filename) const
{
    // get codec
    ImageCodec *pImageCodec = ImageCodec::GetImageCodecForFileName(filename);
    if (pImageCodec == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::SaveTexture: Could not get codec for file name '%s'.", filename);
        return false;
    }

    // open the stream
    AutoReleasePtr<ByteStream> pTextureStream = pArchive->OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    if (pTextureStream == NULL)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::SaveTexture: Could not open file name '%s'.", filename);
        return false;
    }

    // create property list
    PropertyTable imageEncodePropertyList;
    imageEncodePropertyList.SetPropertyValueUInt32(ImageCodecEncoderOptions::PNG_COMPRESSION_LEVEL, BLOCK_MESH_BLOCK_LIST_TEXTURE_COMPRESSION_LEVEL);

    // write the image out
    if (!pImageCodec->EncodeImage(filename, pTextureStream, pImage, imageEncodePropertyList))
    {
        Log_ErrorPrintf("BlockPaletteGenerator::SaveTexture: Failed to encode image '%s'.", filename);
        return false;
    }

    return true;
}

bool BlockPaletteGenerator::SaveXML(ZipArchive *pArchive, ProgressCallbacks *pProgressCallbacks) const
{
    SmallString tempString;

    AutoReleasePtr<ByteStream> pBlockTypesStream = pArchive->OpenFile("palette.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    if (pBlockTypesStream == NULL)
        return false;

    // create xml writer
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pBlockTypesStream))
        return false;

    // write root element
    xmlWriter.StartElement("palette");
    {   
        xmlWriter.WriteAttribute("diffuse-map-size", StringConverter::UInt32ToString(m_diffuseMapSize));
        xmlWriter.WriteAttribute("specular-map-size", StringConverter::UInt32ToString(m_specularMapSize));
        xmlWriter.WriteAttribute("normal-map-size", StringConverter::UInt32ToString(m_normalMapSize));

        // for each block type
        for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
        {
            const BlockType *pBlockType = &m_blockTypes[i];
            if (!m_allocatedBlockTypes[i])
                continue;

            xmlWriter.StartElement("blocktype");
            {
                xmlWriter.WriteAttributef("index", "%u", i);
                xmlWriter.WriteAttribute("name", pBlockType->Name);

                // flags
                xmlWriter.StartElement("flags");
                {
                    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_SOLID)
                        xmlWriter.WriteEmptyElement("solid");

                    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS)
                        xmlWriter.WriteEmptyElement("cast-shadows");
                }
                xmlWriter.EndElement();

                // <visual>
                xmlWriter.StartElement("shape");
                {
                    static const char *shapeNames[BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_COUNT] = { "none", "cube", "slab", "stairs", "plane", "mesh" };
                    static const char *visualNames[BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COUNT] = { "color", "texture", "material" };

                    DebugAssert(pBlockType->ShapeType < BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_COUNT);
                    xmlWriter.WriteAttribute("type", shapeNames[pBlockType->ShapeType]);

                    if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE || 
                        pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB ||
                        pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
                    {
                        if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                            xmlWriter.WriteAttribute("height", StringConverter::FloatToString(pBlockType->SlabShapeSettings.Height));

                        xmlWriter.StartElement("flags");
                        {
                            if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME)
                                xmlWriter.WriteEmptyElement("volume");

                            if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_UNTILEABLE)
                                xmlWriter.WriteEmptyElement("untileable");
                        }
                        xmlWriter.EndElement();

                        xmlWriter.StartElement("faces");
                        {
                            for (uint32 j = 0; j < CUBE_FACE_COUNT; j++)
                            {
                                static const char *faceNames[CUBE_FACE_COUNT] = { "right", "left", "back", "front", "top", "bottom" };
                                xmlWriter.StartElement(faceNames[j]);
                                {
                                    const BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[j];
                                    DebugAssert(pFaceDef->Visual.Type < BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COUNT);
                                    xmlWriter.WriteAttribute("visual", visualNames[pFaceDef->Visual.Type]);
                                    xmlWriter.WriteAttribute("color", StringConverter::ColorToString(pFaceDef->Visual.Color));

                                    if (pFaceDef->Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE)
                                        xmlWriter.WriteAttribute("texture-name", m_textures[pFaceDef->Visual.TextureIndex]->Name);
                                    else if (pFaceDef->Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL)
                                        xmlWriter.WriteAttribute("material-name", pFaceDef->Visual.MaterialName);
                                }
                                xmlWriter.EndElement();
                            }
                        }
                        xmlWriter.EndElement();
                    }
                    else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                    {
                        xmlWriter.StartElement("settings");
                        {
                            DebugAssert(pBlockType->PlaneShapeSettings.Visual.Type < BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COUNT);
                            xmlWriter.WriteAttribute("visual", visualNames[pBlockType->PlaneShapeSettings.Visual.Type]);
                            xmlWriter.WriteAttribute("color", StringConverter::ColorToString(pBlockType->PlaneShapeSettings.Visual.Color));

                            if (pBlockType->PlaneShapeSettings.Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE)
                                xmlWriter.WriteAttribute("texture-name", m_textures[pBlockType->PlaneShapeSettings.Visual.TextureIndex]->Name);
                            else if (pBlockType->PlaneShapeSettings.Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL)
                                xmlWriter.WriteAttribute("material-name", pBlockType->PlaneShapeSettings.Visual.MaterialName);
                            
                            xmlWriter.WriteAttributef("offset-x", "%f", pBlockType->PlaneShapeSettings.OffsetX);
                            xmlWriter.WriteAttributef("offset-y", "%f", pBlockType->PlaneShapeSettings.OffsetY);
                            xmlWriter.WriteAttributef("width", "%f", pBlockType->PlaneShapeSettings.Width);
                            xmlWriter.WriteAttributef("height", "%f", pBlockType->PlaneShapeSettings.Height);
                            xmlWriter.WriteAttributef("base-rotation", "%f", pBlockType->PlaneShapeSettings.BaseRotation);
                            xmlWriter.WriteAttributef("repeat-count", "%u", pBlockType->PlaneShapeSettings.RepeatCount);
                            xmlWriter.WriteAttributef("repeat-rotation", "%f", pBlockType->PlaneShapeSettings.RepeatRotation);
                        }
                        xmlWriter.EndElement();
                    }
                    else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
                    {
                        xmlWriter.StartElement("settings");
                        {
                            xmlWriter.WriteAttribute("name", pBlockType->MeshShapeSettings.MeshName);
                            xmlWriter.WriteAttribute("scale", StringConverter::FloatToString(pBlockType->MeshShapeSettings.Scale));
                        }
                        xmlWriter.EndElement();
                    }
                }
                xmlWriter.EndElement(); // </shape>

                xmlWriter.StartElement("block-light-emitter");
                {
                    xmlWriter.WriteAttribute("enabled", StringConverter::BoolToString(pBlockType->BlockLightEmitterSettings.Enabled));
                    xmlWriter.WriteAttribute("radius", StringConverter::UInt32ToString(pBlockType->BlockLightEmitterSettings.Radius));
                }
                xmlWriter.EndElement();     // </block-light-emitter>

                xmlWriter.StartElement("point-light-emitter");
                {
                    xmlWriter.WriteAttribute("enabled", StringConverter::BoolToString(pBlockType->PointLightEmitterSettings.Enabled));
                    xmlWriter.WriteAttribute("offset", StringConverter::Float3ToString(pBlockType->PointLightEmitterSettings.Offset));
                    xmlWriter.WriteAttribute("color", StringConverter::ColorToString(pBlockType->PointLightEmitterSettings.Color));
                    xmlWriter.WriteAttribute("range", StringConverter::FloatToString(pBlockType->PointLightEmitterSettings.Range));
                    xmlWriter.WriteAttribute("brightness", StringConverter::FloatToString(pBlockType->PointLightEmitterSettings.Brightness));
                    xmlWriter.WriteAttribute("falloff", StringConverter::FloatToString(pBlockType->PointLightEmitterSettings.Falloff));
                }
                xmlWriter.EndElement();     // </point-light-emitter>
            } 
            xmlWriter.EndElement(); // </blocktype>
        }
    }
    xmlWriter.EndElement();

    return (!xmlWriter.InErrorState() && !pBlockTypesStream->InErrorState());
}

const BlockPaletteGenerator::BlockType *BlockPaletteGenerator::GetBlockTypeByName(const char *name) const
{
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        if (m_allocatedBlockTypes[i] && m_blockTypes[i].Name.Compare(name))
            return &m_blockTypes[i];
    }

    return nullptr;
}

BlockPaletteGenerator::BlockType *BlockPaletteGenerator::GetBlockTypeByName(const char *name)
{
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        if (m_allocatedBlockTypes[i] && m_blockTypes[i].Name.Compare(name))
            return &m_blockTypes[i];
    }

    return nullptr;
}

int32 BlockPaletteGenerator::FindBlockTypeByName(const char *name) const
{
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        if (m_allocatedBlockTypes[i] && m_blockTypes[i].Name.Compare(name))
            return (int32)i;
    }

    return -1;
}

BlockPaletteGenerator::BlockType *BlockPaletteGenerator::CreateBlockType(const char *blockName, BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType, uint32 flags)
{
    // ensure name is not used
    if (GetBlockTypeByName(blockName) != nullptr)
        return nullptr;

    // find a free slot
    for (uint32 slot = 1; slot < BLOCK_MESH_MAX_BLOCK_TYPES; slot++)
    {
        if (!m_allocatedBlockTypes[slot])
            return CreateBlockType(slot, blockName, shapeType, flags);
    }

    // no slots
    return nullptr;
}

BlockPaletteGenerator::BlockType *BlockPaletteGenerator::CreateBlockType(uint32 blockTypeIndex, const char *blockName, BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType, uint32 flags)
{
    uint32 i;
    DebugAssert(shapeType < BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_COUNT);
    if (blockTypeIndex == 0 || blockTypeIndex >= BLOCK_MESH_MAX_BLOCK_TYPES || m_allocatedBlockTypes[blockTypeIndex] || GetBlockTypeByName(blockName) != nullptr)
        return nullptr;

    BlockType *pBlockType = &m_blockTypes[blockTypeIndex];
    m_allocatedBlockTypes[blockTypeIndex] = true;

    pBlockType->Name = blockName;
    pBlockType->Flags = flags;
    pBlockType->ShapeType = shapeType;

    switch (pBlockType->ShapeType)
    {
    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE:
        {
            for (i = 0; i < CUBE_FACE_COUNT; i++)
            {
                pBlockType->CubeShapeFaces[i].Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
                pBlockType->CubeShapeFaces[i].Visual.Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
            }
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB:
        {
            pBlockType->SlabShapeSettings.Height = 1.0f;
            for (i = 0; i < CUBE_FACE_COUNT; i++)
            {
                pBlockType->CubeShapeFaces[i].Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
                pBlockType->CubeShapeFaces[i].Visual.Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
            }
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE:
        {
            pBlockType->PlaneShapeSettings.Visual.Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
            pBlockType->PlaneShapeSettings.Visual.Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
            pBlockType->PlaneShapeSettings.OffsetX = 0.0f;
            pBlockType->PlaneShapeSettings.OffsetY = 0.0f;
            pBlockType->PlaneShapeSettings.BaseRotation = 0.0f;
            pBlockType->PlaneShapeSettings.RepeatCount = 1;
            pBlockType->PlaneShapeSettings.RepeatRotation = 0.0f;
        }
        break;
    }

    return pBlockType;
}

void BlockPaletteGenerator::RemoveBlockType(uint32 blockTypeIndex)
{
    DebugAssert(blockTypeIndex < BLOCK_MESH_MAX_BLOCK_TYPES && m_allocatedBlockTypes[blockTypeIndex]);

    ResetBlockType(&m_blockTypes[blockTypeIndex]);
    m_allocatedBlockTypes[blockTypeIndex] = false;
}

const BlockPaletteGenerator::Texture *BlockPaletteGenerator::GetTextureByName(const char *name) const
{
    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->Name.CompareInsensitive(name))
            return m_textures[i];
    }

    return nullptr;
}

const BlockPaletteGenerator::Texture *BlockPaletteGenerator::CreateTexture(const char *textureName, BLOCK_MESH_TEXTURE_BLENDING blending, bool allowTextureFiltering)
{
    DebugAssert(blending < BLOCK_MESH_TEXTURE_BLENDING_COUNT);

    for (uint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->Name.CompareInsensitive(textureName))
            return nullptr;
    }

    Texture *pTexture = new Texture();
    pTexture->Index = m_textures.GetSize();
    pTexture->Name = textureName;
    pTexture->Blending = blending;
    pTexture->Effect = BLOCK_MESH_TEXTURE_EFFECT_NONE;
    pTexture->AllowTextureFiltering = allowTextureFiltering;
    Y_memzero(pTexture->ScrollDirection, sizeof(pTexture->ScrollDirection));
    pTexture->ScrollSpeed = 0.0f;
    pTexture->AnimationFrameCount = 0;
    pTexture->AnimationSpeed = 0.0f;
    pTexture->pDiffuseMap = nullptr;
    pTexture->pNormalMap = nullptr;
    pTexture->pSpecularMap = nullptr;
    m_textures.Add(pTexture);

    return pTexture;
}

bool BlockPaletteGenerator::SetTextureDiffuseMap(uint32 textureIndex, const Image *pImage)
{
    Texture *pTextureAtlas = m_textures[textureIndex];

    // set new
    if (pImage != NULL)
    {
        bool result = true;
        Image *pNewImage = new Image();
        if (pImage->GetPixelFormat() != BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT)
            result = pNewImage->CopyAndConvertPixelFormat(*pImage, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT);
        else
            pNewImage->Copy(*pImage);

        if (!result)
        {
            delete pNewImage;
            return false;
        }

        delete pTextureAtlas->pDiffuseMap;
        pTextureAtlas->pDiffuseMap = pNewImage;
        return true;
    }
    else
    {
        delete pTextureAtlas->pDiffuseMap;
        pTextureAtlas->pDiffuseMap = NULL;
        return true;
    }
}

bool BlockPaletteGenerator::SetTextureSpecularMap(uint32 textureIndex, const Image *pImage)
{
    Texture *pTextureAtlas = m_textures[textureIndex];

    // set new
    if (pImage != NULL)
    {
        bool result = true;
        Image *pNewImage = new Image();
        if (pImage->GetPixelFormat() != BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT)
            result = pNewImage->CopyAndConvertPixelFormat(*pImage, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT);
        else
            pNewImage->Copy(*pImage);

        if (!result)
        {
            delete pNewImage;
            return false;
        }

        delete pTextureAtlas->pSpecularMap;
        pTextureAtlas->pSpecularMap = pNewImage;
        return true;
    }
    else
    {
        delete pTextureAtlas->pSpecularMap;
        pTextureAtlas->pSpecularMap = NULL;
        return true;
    }
}

bool BlockPaletteGenerator::SetTextureNormalMap(uint32 textureIndex, const Image *pImage)
{
    Texture *pTexture = m_textures[textureIndex];

    // set new
    if (pImage != NULL)
    {
        bool result = true;
        Image *pNewImage = new Image();
        if (pImage->GetPixelFormat() != BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT)
            result = pNewImage->CopyAndConvertPixelFormat(*pImage, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT);
        else
            pNewImage->Copy(*pImage);

        if (!result)
        {
            delete pNewImage;
            return false;
        }

        delete pTexture->pNormalMap;
        pTexture->pNormalMap = pNewImage;
        return true;
    }
    else
    {
        delete pTexture->pNormalMap;
        pTexture->pNormalMap = NULL;
        return true;
    }
}

void BlockPaletteGenerator::SetTextureBlending(uint32 textureIndex, BLOCK_MESH_TEXTURE_BLENDING blending)
{
    Texture *pTexture = m_textures[textureIndex];

    pTexture->Blending = blending;
}

void BlockPaletteGenerator::SetTextureEffectNone(uint32 textureIndex)
{
    Texture *pTexture = m_textures[textureIndex];

    pTexture->Effect = BLOCK_MESH_TEXTURE_EFFECT_NONE;
    pTexture->ScrollDirection.SetZero();
    pTexture->ScrollSpeed = 0.0f;
    pTexture->AnimationFrameCount = 0;
    pTexture->AnimationSpeed = 0.0f;
}

void BlockPaletteGenerator::SetTextureEffectScrolled(uint32 textureIndex, const float2 &scrollDirection, float scrollSpeed)
{
    Texture *pTexture = m_textures[textureIndex];

    pTexture->Effect = BLOCK_MESH_TEXTURE_EFFECT_SCROLL;
    pTexture->ScrollDirection = scrollDirection.Normalize();
    pTexture->ScrollSpeed = scrollSpeed;
    pTexture->AnimationFrameCount = 0;
    pTexture->AnimationSpeed = 0.0f;
}

void BlockPaletteGenerator::SetTextureEffectAnimation(uint32 textureIndex, uint32 animationFrameCount, float animationSpeed)
{
    Texture *pTexture = m_textures[textureIndex];

    pTexture->Effect = BLOCK_MESH_TEXTURE_EFFECT_ANIMATION;
    pTexture->ScrollDirection.SetZero();
    pTexture->ScrollSpeed = 0.0f;
    pTexture->AnimationFrameCount = animationFrameCount;
    pTexture->AnimationSpeed = animationSpeed;
}

BlockPaletteGenerator &BlockPaletteGenerator::operator=(const BlockPaletteGenerator &copyFrom)
{
    Panic("Fixme");
    /*
    uint32 i;

    for (i = 0; i < m_Textures.GetSize(); i++)
    {
        Texture *t = m_Textures[i];
        if (t->DiffuseMapData != NULL)
            Y_free(t->DiffuseMapData);
        if (t->SpecularMapData != NULL)
            Y_free(t->SpecularMapData);
        if (t->NormalMapData != NULL)
            Y_free(t->NormalMapData);
        delete t;
    }
    m_Textures.Obliterate();

    for (i = 0; i < copyFrom.m_Textures.GetSize(); i++)
    {
        const Texture *pSourceTexture = copyFrom.m_Textures[i];
        Texture *pDestinationTexture = new Texture;

        pDestinationTexture->Index = pSourceTexture->Index;
        pDestinationTexture->Name = pSourceTexture->Name;

        if (pSourceTexture->DiffuseMapData != NULL)
        {
            pDestinationTexture->DiffuseMapData = (byte *)malloc(pSourceTexture->DiffuseMapDataSize);
            Y_memcpy(pDestinationTexture->DiffuseMapData, pSourceTexture->DiffuseMapData, pSourceTexture->DiffuseMapDataSize);
            pDestinationTexture->DiffuseMapDataSize = pSourceTexture->DiffuseMapDataSize;
            pDestinationTexture->DiffuseMapWidth = pSourceTexture->DiffuseMapWidth;
            pDestinationTexture->DiffuseMapHeight = pSourceTexture->DiffuseMapHeight;
        }
        else
        {
            pDestinationTexture->DiffuseMapData = NULL;
            pDestinationTexture->DiffuseMapDataSize = 0;
            pDestinationTexture->DiffuseMapWidth = 0;
            pDestinationTexture->DiffuseMapHeight = 0;
        }

        if (pSourceTexture->SpecularMapData != NULL)
        {
            pDestinationTexture->SpecularMapData = (byte *)malloc(pSourceTexture->SpecularMapDataSize);
            Y_memcpy(pDestinationTexture->SpecularMapData, pSourceTexture->SpecularMapData, pSourceTexture->SpecularMapDataSize);
            pDestinationTexture->SpecularMapDataSize = pSourceTexture->SpecularMapDataSize;
            pDestinationTexture->SpecularMapWidth = pSourceTexture->SpecularMapWidth;
            pDestinationTexture->SpecularMapHeight = pSourceTexture->SpecularMapHeight;
        }
        else
        {
            pDestinationTexture->SpecularMapData = NULL;
            pDestinationTexture->SpecularMapDataSize = 0;
            pDestinationTexture->SpecularMapWidth = 0;
            pDestinationTexture->SpecularMapHeight = 0;
        }

        if (pSourceTexture->NormalMapData != NULL)
        {
            pDestinationTexture->NormalMapData = (byte *)malloc(pSourceTexture->NormalMapDataSize);
            Y_memcpy(pDestinationTexture->NormalMapData, pSourceTexture->NormalMapData, pSourceTexture->NormalMapDataSize);
            pDestinationTexture->NormalMapDataSize = pSourceTexture->NormalMapDataSize;
            pDestinationTexture->NormalMapWidth = pSourceTexture->NormalMapWidth;
            pDestinationTexture->NormalMapHeight = pSourceTexture->NormalMapHeight;
        }
        else
        {
            pDestinationTexture->NormalMapData = NULL;
            pDestinationTexture->NormalMapDataSize = 0;
            pDestinationTexture->NormalMapWidth = 0;
            pDestinationTexture->NormalMapHeight = 0;
        }

        m_Textures.Add(pDestinationTexture);
    }

    const BlockType *pSourceBlockType = copyFrom.m_BlockTypes;
    BlockType *pDestinationBlockType = m_BlockTypes;
    for (i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++, pSourceBlockType++, pDestinationBlockType++)
    {
        ResetBlockType(pDestinationBlockType);
        m_bAllocatedBlockTypes[i] = copyFrom.m_bAllocatedBlockTypes[i];
        if (m_bAllocatedBlockTypes[i])
        {
            DebugAssert(pDestinationBlockType->BlockTypeIndex == pSourceBlockType->BlockTypeIndex);
            pDestinationBlockType->Name = pSourceBlockType->Name;
            pDestinationBlockType->Flags = pSourceBlockType->Flags;
            pDestinationBlockType->ShapeType = pSourceBlockType->ShapeType;
            
            if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
            {
                for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
                {
                    const BlockType::CubeShapeFace *pSourceFace = &pSourceBlockType->CubeShapeFaces[i];
                    BlockType::CubeShapeFace *pDestinationFace = &pDestinationBlockType->CubeShapeFaces[i];

                    pDestinationFace->VisualType = pSourceFace->VisualType;
                    pDestinationFace->Color = pSourceFace->Color;
                    pDestinationFace->TextureIndex = pSourceFace->TextureIndex;
                    pDestinationFace->TextureScrollSpeed = pSourceFace->TextureScrollSpeed;
                    pDestinationFace->ExternalMaterialName = pSourceFace->ExternalMaterialName;
                }
            }
            else if (pDestinationBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
            {
                pDestinationBlockType->PlaneShapeSettings.VisualType = pSourceBlockType->PlaneShapeSettings.VisualType;
                pDestinationBlockType->PlaneShapeSettings.Color = pSourceBlockType->PlaneShapeSettings.Color;
                pDestinationBlockType->PlaneShapeSettings.TextureIndex = pSourceBlockType->PlaneShapeSettings.TextureIndex;
                pDestinationBlockType->PlaneShapeSettings.TextureScrollSpeed = pSourceBlockType->PlaneShapeSettings.TextureScrollSpeed;
                pDestinationBlockType->PlaneShapeSettings.ExternalMaterialName = pSourceBlockType->PlaneShapeSettings.ExternalMaterialName;
                pDestinationBlockType->PlaneShapeSettings.OffsetX = pSourceBlockType->PlaneShapeSettings.OffsetX;
                pDestinationBlockType->PlaneShapeSettings.OffsetY = pSourceBlockType->PlaneShapeSettings.OffsetY;
                pDestinationBlockType->PlaneShapeSettings.BaseRotation = pSourceBlockType->PlaneShapeSettings.BaseRotation;
                pDestinationBlockType->PlaneShapeSettings.RepeatCount = pSourceBlockType->PlaneShapeSettings.RepeatCount;
                pDestinationBlockType->PlaneShapeSettings.RepeatRotation = pSourceBlockType->PlaneShapeSettings.RepeatRotation;
            }
        }
    }
    */
    return *this;
}

class BlockPaletteCompiler
{
public:
    BlockPaletteCompiler(const BlockPaletteGenerator *pGenerator);
    ~BlockPaletteCompiler();

    bool Compile(TEXTURE_PLATFORM texturePlatform, ByteStream *pOutputStream);

private:
    struct SourceTextureEntry
    {
        uint32 SourceIndex;
        uint32 MaterialIndex;
        uint32 TextureArrayIndex;
        float3 MinUV;
        float3 MaxUV;
    };

    struct MaterialEntry
    {
        DF_BLOCK_PALETTE_MATERIAL_TYPE Type;
        uint32 Flags;

        String ExternalMaterialName;

        BLOCK_MESH_TEXTURE_BLENDING TextureBlending;
        BLOCK_MESH_TEXTURE_EFFECT TextureEffect;

        bool HasNormalMap;
        bool HasSpecularMap;

        int32 DiffuseMapTextureIndex;
        int32 SpecularMapTextureIndex;
        int32 NormalMapTextureIndex;
        uint32 TextureArraySize;

        uint32 TextureAtlasTileWidth;
        uint32 TextureAtlasTileHeight;
        uint32 TextureAtlasHPadding;
        uint32 TextureAtlasVPadding;
        uint32 TextureAtlasColumns;
        uint32 TextureAtlasRows;

        float2 ScrollDirection;
        float ScrollSpeed;

        //uint32 AnimationFrameCount;
        //float AnimationSpeed;
    };

    struct BlockTypeEntry
    {
        uint32 BlockTypeIndex;
        String Name;
        uint32 Flags;
        BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE ShapeType;

        struct VisualParameters
        {
            BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE Type;
            uint32 MaterialIndex;
            uint32 Color;
            float3 MinUV;
            float3 MaxUV;
            float4 AtlasUVRange;
        };

        struct CubeShapeFace
        {
            VisualParameters Visual;
        };

        struct SlabShape
        {
            float Height;
        };

        struct PlaneShape
        {
            VisualParameters Visual;
            float OffsetX;
            float OffsetY;
            float Width;
            float Height;
            float BaseRotation;
            uint32 RepeatCount;
            float RepeatRotation;
        };

        struct MeshShape
        {
            uint32 MeshNameIndex;
            float Scale;
        };

        struct BlockLightEmitter
        {
            uint32 Radius;
        };

        struct PointLightEmitter
        {
            float3 Offset;
            uint32 Color;
            float Brightness;
            float Range;
            float Falloff;
        };

        CubeShapeFace CubeShapeFaces[CUBE_FACE_COUNT];
        SlabShape SlabSettings;
        PlaneShape PlaneSettings;
        MeshShape MeshSettings;
        BlockLightEmitter BlockLightEmitterSettings;
        PointLightEmitter PointLightEmitterSettings;
    };

    bool GenerateMaterials();

    bool GenerateTextureMipmaps();
    bool ConvertTextureArraysToTextureAtlas();

    bool CompileBlockType(const BlockPaletteGenerator::BlockType *pBlockType);
    bool CompileBlockTypeVisual(const BlockPaletteGenerator::BlockType::VisualParameters *pSourceVisual, BlockTypeEntry::VisualParameters *pDestinationVisual);

    bool WriteHeader(ByteStream *pStream);
    bool WriteBlockTypes(ByteStream *pStream, ChunkFileWriter &cfw);
    bool WriteTextures(ByteStream *pStream, ChunkFileWriter &cfw, TEXTURE_PLATFORM texturePlatform);
    bool WriteMaterials(ByteStream *pStream, ChunkFileWriter &cfw);
    bool WriteMeshes(ByteStream *pStream, ChunkFileWriter &cfw);

    const BlockPaletteGenerator *m_pGenerator;
    bool m_enableTextureCompression;
    bool m_enableMipGeneration;

    PODArray<SourceTextureEntry *> m_sourceTextures;
    PODArray<BlockTypeEntry *> m_blockTypes;
    PODArray<TextureGenerator *> m_textures;
    PODArray<MaterialEntry *> m_materials;
    PODArray<const String *> m_meshNames;
};

BlockPaletteCompiler::BlockPaletteCompiler(const BlockPaletteGenerator *pGenerator)
    : m_pGenerator(pGenerator),
      m_enableTextureCompression(true),
      m_enableMipGeneration(true)
{

}

BlockPaletteCompiler::~BlockPaletteCompiler()
{
    uint32 i;

    for (i = 0; i < m_materials.GetSize(); i++)
        delete m_materials[i];

    for (i = 0; i < m_textures.GetSize(); i++)
        delete m_textures[i];

    for (i = 0; i < m_blockTypes.GetSize(); i++)
        delete m_blockTypes[i];

    for (i = 0; i < m_sourceTextures.GetSize(); i++)
        delete m_sourceTextures[i];
}

bool BlockPaletteCompiler::Compile(TEXTURE_PLATFORM texturePlatform, ByteStream *pOutputStream)
{
    // settings
    //m_enableTextureCompression = true;
    //m_enableTextureCompression = false;
    //m_enableMipGeneration = true;
    //m_enableMipGeneration = false;

    // gen materials
    if (!GenerateMaterials())
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to generate materials.");
        return false;
    }

    // add block types
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        const BlockPaletteGenerator::BlockType *pBlockType = m_pGenerator->GetBlockType(i);
        if (pBlockType == NULL)
            continue;

        if (!CompileBlockType(pBlockType))
        {
            Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to compile block type %u (%s)", i, pBlockType->Name.GetCharArray());
            return false;
        }
    }

//     if (!ConvertTextureArraysToTextureAtlas())
//     {
//         Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to convert texture arrays to atlases.");
//         return false;
//     }

    if (!WriteHeader(pOutputStream))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to write header.");
        return false;
    }

    // initialize chunk file writer
    ChunkFileWriter cfw;
    if (!cfw.Initialize(pOutputStream, DF_BLOCK_PALETTE_CHUNK_COUNT))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to initialize chunk file writer.");
        return false;
    }

    if (!WriteBlockTypes(pOutputStream, cfw))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to write block types.");
        return false;
    }

    if (!WriteTextures(pOutputStream, cfw, texturePlatform))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to write textures.");
        return false;
    }

    if (!WriteMaterials(pOutputStream, cfw))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to write materials.");
        return false;
    }

    if (!WriteMeshes(pOutputStream, cfw))
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to write meshes.");
        return false;
    }

    // close chunk writer
    if (!cfw.Close() || pOutputStream->InErrorState())
    {
        Log_ErrorPrintf("BlockPaletteCompiler::Compile: Failed to close chunk file writer.");
        return false;
    }

    Log_DevPrintf("BlockPaletteCompiler::Compile: Block list compiled, size = %u bytes", (uint32)pOutputStream->GetSize());
    return true;
}

bool BlockPaletteCompiler::GenerateMaterials()
{
    // for each source texture...
    for (uint32 sourceTextureIndex = 0; sourceTextureIndex < m_pGenerator->GetTextureCount(); sourceTextureIndex++)
    {
        const BlockPaletteGenerator::Texture *pSourceTexture = m_pGenerator->GetTexture(sourceTextureIndex);

        // is it used anywhere?
        bool sourceTextureUsed = false;
        for (uint32 blockTypeIndex = 0; blockTypeIndex < BLOCK_MESH_MAX_BLOCK_TYPES; blockTypeIndex++)
        {
            const BlockPaletteGenerator::BlockType *pSourceBlockType = m_pGenerator->GetBlockType(blockTypeIndex);
            if (pSourceBlockType != NULL)
            {
                if (pSourceBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE || pSourceBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                {
                    for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
                    {
                        if (pSourceBlockType->CubeShapeFaces[i].Visual.TextureIndex == sourceTextureIndex)
                        {
                            sourceTextureUsed = true;
                            break;
                        }
                    }
                }
                else if (pSourceBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                {
                    if (pSourceBlockType->PlaneShapeSettings.Visual.TextureIndex == sourceTextureIndex)
                        sourceTextureUsed = true;
                }
            }

            if (sourceTextureUsed)
                break;
        }

        // if not, skip it
        if (!sourceTextureUsed)
            continue;

        // source texture has normal/specular map?
        bool sourceTextureHasNormalMap = (pSourceTexture->pNormalMap != nullptr);
        bool sourceTextureHasSpecularMap = (pSourceTexture->pSpecularMap != nullptr);

        // this texture is used somewhere, so it does have to be compiled
        // search for a material with these parameters already matching
        uint32 materialIndex;
        for (materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
        {
            MaterialEntry *pMaterialEntry = m_materials[materialIndex];
            if (pMaterialEntry->HasNormalMap == sourceTextureHasNormalMap &&
                pMaterialEntry->HasSpecularMap == sourceTextureHasSpecularMap &&
                pMaterialEntry->TextureBlending == pSourceTexture->Blending &&
                pMaterialEntry->TextureEffect == pSourceTexture->Effect)
            {
                // test for effect-specific parameters
                if (pMaterialEntry->TextureEffect == BLOCK_MESH_TEXTURE_EFFECT_SCROLL)
                {
                    if (pMaterialEntry->ScrollDirection != pSourceTexture->ScrollDirection ||
                        pMaterialEntry->ScrollSpeed != pSourceTexture->ScrollSpeed)
                    {
                        // mismatch
                        continue;
                    }
                }
                else if (pMaterialEntry->TextureEffect == BLOCK_MESH_TEXTURE_EFFECT_ANIMATION)
                {
                    //if (pMaterialEntry->)
                }

                // material matches
                break;
            }
        }

        // found one?
        MaterialEntry *pMaterialEntry;
        uint32 textureArrayIndex;
        if (materialIndex != m_materials.GetSize())
        {
            pMaterialEntry = m_materials[materialIndex];

            // if this material is not currently using a texture array, it will have to be now
            if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE ||
                pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_WITH_NORMAL_MAP ||
                pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE ||
                pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_WITH_NORMAL_MAP ||
                pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE ||
                pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_WITH_NORMAL_MAP)
            {
                // switch it to a texture array
                if (pMaterialEntry->DiffuseMapTextureIndex >= 0)
                {
                    DebugAssert(m_textures[pMaterialEntry->DiffuseMapTextureIndex]->GetTextureType() == TEXTURE_TYPE_2D);
                    m_textures[pMaterialEntry->DiffuseMapTextureIndex]->ConvertToTextureArray();
                }
                if (pMaterialEntry->SpecularMapTextureIndex >= 0)
                {
                    DebugAssert(m_textures[pMaterialEntry->SpecularMapTextureIndex]->GetTextureType() == TEXTURE_TYPE_2D);
                    m_textures[pMaterialEntry->SpecularMapTextureIndex]->ConvertToTextureArray();
                }
                if (pMaterialEntry->NormalMapTextureIndex >= 0)
                {
                    DebugAssert(m_textures[pMaterialEntry->NormalMapTextureIndex]->GetTextureType() == TEXTURE_TYPE_2D);
                    m_textures[pMaterialEntry->NormalMapTextureIndex]->ConvertToTextureArray();
                }

                // update the material type
                if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY;
                else if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_WITH_NORMAL_MAP)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ARRAY_WITH_NORMAL_MAP;
                else if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY;
                else if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_WITH_NORMAL_MAP)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ARRAY_WITH_NORMAL_MAP;
                else if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY;
                else if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_WITH_NORMAL_MAP)
                    pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ARRAY_WITH_NORMAL_MAP;
            }

            // append this texture's images to the array
            textureArrayIndex = pMaterialEntry->TextureArraySize++;

            // add diffuse map
            if (pSourceTexture->pDiffuseMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = m_textures[pMaterialEntry->DiffuseMapTextureIndex];
                if (pSourceTexture->pDiffuseMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pDiffuseMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pDiffuseMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->AddImage(&tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize diffuse map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->AddImage(pSourceTexture->pDiffuseMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to append diffuse map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }

            // add specular map
            if (pSourceTexture->pSpecularMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = m_textures[pMaterialEntry->SpecularMapTextureIndex];
                if (pSourceTexture->pSpecularMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pSpecularMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pSpecularMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->AddImage(&tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize specular map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->AddImage(pSourceTexture->pSpecularMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to append specular map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }

            // add normal map
            if (pSourceTexture->pNormalMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = m_textures[pMaterialEntry->NormalMapTextureIndex];
                if (pSourceTexture->pNormalMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pNormalMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pNormalMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->AddImage(&tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize normal map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->AddImage(pSourceTexture->pNormalMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to append normal map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }
        }
        else
        {
            // have to create a new material
            pMaterialEntry = new MaterialEntry;
            materialIndex = m_materials.GetSize();
            m_materials.Add(pMaterialEntry);

            // set the type of it
            if (pSourceTexture->Blending == BLOCK_MESH_TEXTURE_BLENDING_NONE)
                pMaterialEntry->Type = (sourceTextureHasNormalMap) ? DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_WITH_NORMAL_MAP : DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE;
            else if (pSourceTexture->Blending == BLOCK_MESH_TEXTURE_BLENDING_MASKED)
                pMaterialEntry->Type = (sourceTextureHasNormalMap) ? DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_WITH_NORMAL_MAP : DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE;
            else //if (pSourceTexture->Blending == BLOCK_MESH_TEXTURE_BLENDING_TRANSLUCENT)
                pMaterialEntry->Type = (sourceTextureHasNormalMap) ? DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_WITH_NORMAL_MAP : DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE;

            // set flags
            pMaterialEntry->Flags = 0;
            if (pSourceTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_SCROLL)
                pMaterialEntry->Flags |= DF_BLOCK_PALETTE_MATERIAL_FLAG_SCROLLED_TEXTURE;
            else if (pSourceTexture->Effect == BLOCK_MESH_TEXTURE_EFFECT_ANIMATION)
                pMaterialEntry->Flags |= DF_BLOCK_PALETTE_MATERIAL_FLAG_ANIMATED_TEXTURE;

            // fill in common fields
            pMaterialEntry->TextureBlending = pSourceTexture->Blending;
            pMaterialEntry->TextureEffect = pSourceTexture->Effect;
            pMaterialEntry->HasNormalMap = sourceTextureHasNormalMap;
            pMaterialEntry->HasSpecularMap = sourceTextureHasSpecularMap;
            pMaterialEntry->DiffuseMapTextureIndex = -1;
            pMaterialEntry->SpecularMapTextureIndex = -1;
            pMaterialEntry->NormalMapTextureIndex = -1;
            pMaterialEntry->TextureArraySize = 1;
            pMaterialEntry->TextureAtlasTileWidth = 0;
            pMaterialEntry->TextureAtlasTileHeight = 0;
            pMaterialEntry->TextureAtlasHPadding = 0;
            pMaterialEntry->TextureAtlasVPadding = 0;
            pMaterialEntry->TextureAtlasColumns = 0;
            pMaterialEntry->TextureAtlasRows = 0;
            pMaterialEntry->ScrollDirection = pSourceTexture->ScrollDirection;
            pMaterialEntry->ScrollSpeed = pSourceTexture->ScrollSpeed;

            // texture array index will be 0 since we are the only texture for now
            textureArrayIndex = 0;

            // create diffuse map texture
            if (pSourceTexture->pDiffuseMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = new TextureGenerator;
                pDestinationTexture->Create(TEXTURE_TYPE_2D, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, m_pGenerator->GetDiffuseMapSize(), m_pGenerator->GetDiffuseMapSize(), 1, 1);
                pDestinationTexture->SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureFilter((pSourceTexture->AllowTextureFiltering) ? TEXTURE_FILTER_ANISOTROPIC : TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR);
                pDestinationTexture->SetTextureUsage(TEXTURE_USAGE_COLOR_MAP);
                pDestinationTexture->SetSourcePremultipliedAlpha(false);
                pDestinationTexture->SetEnablePremultipliedAlpha((pSourceTexture->Blending == BLOCK_MESH_TEXTURE_BLENDING_TRANSLUCENT));
                pDestinationTexture->SetEnableTextureCompression(m_enableTextureCompression);
                pDestinationTexture->SetGenerateMipmaps(m_enableMipGeneration);
                pDestinationTexture->SetMipMapResizeFilter(IMAGE_RESIZE_FILTER_LANCZOS3);
                pDestinationTexture->SetSourceSRGB(true);
                pDestinationTexture->SetEnableSRGB(true);
                pMaterialEntry->DiffuseMapTextureIndex = (uint32)m_textures.GetSize();
                m_textures.Add(pDestinationTexture);

                if (pSourceTexture->pDiffuseMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pDiffuseMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pDiffuseMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->SetImage(0, &tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize diffuse map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->SetImage(0, pSourceTexture->pDiffuseMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to set diffuse map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }

            // create specular map texture
            if (pSourceTexture->pSpecularMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = new TextureGenerator;
                pDestinationTexture->Create(TEXTURE_TYPE_2D, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, m_pGenerator->GetSpecularMapSize(), m_pGenerator->GetSpecularMapSize(), 1, 1);
                pDestinationTexture->SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureFilter((pSourceTexture->AllowTextureFiltering) ? TEXTURE_FILTER_ANISOTROPIC : TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR);
                pDestinationTexture->SetTextureUsage(TEXTURE_USAGE_GLOSS_MAP);
                pDestinationTexture->SetSourcePremultipliedAlpha(false);
                pDestinationTexture->SetEnablePremultipliedAlpha(false);
                pDestinationTexture->SetEnableTextureCompression(m_enableTextureCompression);
                pDestinationTexture->SetGenerateMipmaps(m_enableMipGeneration);
                pDestinationTexture->SetMipMapResizeFilter(IMAGE_RESIZE_FILTER_LANCZOS3);
                pDestinationTexture->SetSourceSRGB(false);
                pDestinationTexture->SetEnableSRGB(false);
                pMaterialEntry->SpecularMapTextureIndex = (uint32)m_textures.GetSize();
                m_textures.Add(pDestinationTexture);

                if (pSourceTexture->pSpecularMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pSpecularMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pSpecularMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->SetImage(0, &tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize specular map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->SetImage(0, pSourceTexture->pSpecularMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to set specular map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }

            // create normal map texture
            if (pSourceTexture->pNormalMap != nullptr)
            {
                TextureGenerator *pDestinationTexture = new TextureGenerator;
                pDestinationTexture->Create(TEXTURE_TYPE_2D, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, m_pGenerator->GetNormalMapSize(), m_pGenerator->GetNormalMapSize(), 1, 1);
                pDestinationTexture->SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_WRAP);
                pDestinationTexture->SetTextureFilter((pSourceTexture->AllowTextureFiltering) ? TEXTURE_FILTER_ANISOTROPIC : TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR);
                pDestinationTexture->SetTextureUsage(TEXTURE_USAGE_NORMAL_MAP);
                pDestinationTexture->SetSourcePremultipliedAlpha(false);
                pDestinationTexture->SetEnablePremultipliedAlpha(false);
                pDestinationTexture->SetEnableTextureCompression(m_enableTextureCompression);
                pDestinationTexture->SetGenerateMipmaps(m_enableMipGeneration);
                pDestinationTexture->SetMipMapResizeFilter(IMAGE_RESIZE_FILTER_LANCZOS3);
                pDestinationTexture->SetSourceSRGB(false);
                pDestinationTexture->SetEnableSRGB(false);
                pMaterialEntry->NormalMapTextureIndex = (uint32)m_textures.GetSize();
                m_textures.Add(pDestinationTexture);

                if (pSourceTexture->pNormalMap->GetWidth() != pDestinationTexture->GetWidth() ||
                    pSourceTexture->pNormalMap->GetHeight() != pDestinationTexture->GetHeight())
                {
                    // copy and resize it
                    Image tempImage;
                    if (!tempImage.CopyAndResize(*pSourceTexture->pNormalMap, IMAGE_RESIZE_FILTER_LANCZOS3, pDestinationTexture->GetWidth(), pDestinationTexture->GetHeight(), 1) ||
                        !pDestinationTexture->SetImage(0, &tempImage))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to resize normal map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
                else
                {
                    // add as normal
                    if (!pDestinationTexture->SetImage(0, pSourceTexture->pNormalMap))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::GenerateMaterials: Failed to set normal map for source texture '%s'", pSourceTexture->Name.GetCharArray());
                        return false;
                    }
                }
            }
        }

        // now we have a material and texture array index, we can create the source texture info
        SourceTextureEntry *pSourceTextureEntry = new SourceTextureEntry;
        pSourceTextureEntry->SourceIndex = sourceTextureIndex;
        pSourceTextureEntry->MaterialIndex = materialIndex;
        pSourceTextureEntry->TextureArrayIndex = textureArrayIndex;
        pSourceTextureEntry->MinUV.Set(0.0f, 0.0f, (float)textureArrayIndex);
        pSourceTextureEntry->MaxUV.Set(1.0f, 1.0f, (float)textureArrayIndex);
        m_sourceTextures.Add(pSourceTextureEntry);
    }

    Log_DevPrintf("BlockPaletteCompiler::GenerateMaterials: Generated %u materials, %u textures", m_materials.GetSize(), m_textures.GetSize());
    return true;
}

bool BlockPaletteCompiler::CompileBlockTypeVisual(const BlockPaletteGenerator::BlockType::VisualParameters *pSourceVisual, BlockTypeEntry::VisualParameters *pDestinationVisual)
{
    if (pSourceVisual->Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL)
    {
        // find a matching external material
        uint32 materialIndex;
        for (materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
        {
            if (m_materials[materialIndex]->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL && m_materials[materialIndex]->ExternalMaterialName == pSourceVisual->MaterialName)
                break;
        }

        // not found?
        if (materialIndex == m_materials.GetSize())
        {
            // create it
            MaterialEntry *pMaterialEntry = new MaterialEntry;
            pMaterialEntry->Type = DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL;
            pMaterialEntry->Flags = 0;
            pMaterialEntry->ExternalMaterialName = pSourceVisual->MaterialName;
            pMaterialEntry->TextureBlending = BLOCK_MESH_TEXTURE_BLENDING_COUNT;
            pMaterialEntry->TextureEffect = BLOCK_MESH_TEXTURE_EFFECT_NONE;
            pMaterialEntry->DiffuseMapTextureIndex = -1;
            pMaterialEntry->SpecularMapTextureIndex = -1;
            pMaterialEntry->NormalMapTextureIndex = -1;
            pMaterialEntry->TextureArraySize = 0;
            pMaterialEntry->TextureAtlasTileWidth = 0;
            pMaterialEntry->TextureAtlasTileHeight = 0;
            pMaterialEntry->TextureAtlasHPadding = 0;
            pMaterialEntry->TextureAtlasVPadding = 0;
            pMaterialEntry->TextureAtlasColumns = 0;
            pMaterialEntry->TextureAtlasRows = 0;
            pMaterialEntry->ScrollDirection.SetZero();
            pMaterialEntry->ScrollSpeed = 0.0f;
            m_materials.Add(pMaterialEntry);
        }

        // return everything
        pDestinationVisual->Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL;
        pDestinationVisual->MaterialIndex = materialIndex;
        pDestinationVisual->Color = PixelFormatHelpers::ConvertSRGBToLinear(pSourceVisual->Color);
        pDestinationVisual->MinUV.Set(0.0f, 0.0f, 0.0f);
        pDestinationVisual->MaxUV.Set(1.0f, 1.0f, 0.0f);
        pDestinationVisual->AtlasUVRange.Set(0.0f, 0.0f, 0.0f, 0.0f);
        return true;
    }
    else if (pSourceVisual->Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR)
    {
        // is the colour translucent or not?
        DF_BLOCK_PALETTE_MATERIAL_TYPE searchMaterialType = ((pSourceVisual->Color >> 24) != 0xFF) ? DF_BLOCK_PALETTE_MATERIAL_TYPE_COLOR : DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_COLOR;

        // find matching material
        uint32 materialIndex;
        for (materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
        {
            if (m_materials[materialIndex]->Type == searchMaterialType)
                break;
        }
        if (materialIndex == m_materials.GetSize())
        {
            // create it
            MaterialEntry *pMaterialEntry = new MaterialEntry;
            pMaterialEntry->Type = searchMaterialType;
            pMaterialEntry->Flags = 0;
            pMaterialEntry->TextureBlending = BLOCK_MESH_TEXTURE_BLENDING_COUNT;
            pMaterialEntry->TextureEffect = BLOCK_MESH_TEXTURE_EFFECT_NONE;
            pMaterialEntry->DiffuseMapTextureIndex = -1;
            pMaterialEntry->SpecularMapTextureIndex = -1;
            pMaterialEntry->NormalMapTextureIndex = -1;
            pMaterialEntry->TextureArraySize = 0;
            pMaterialEntry->TextureAtlasTileWidth = 0;
            pMaterialEntry->TextureAtlasTileHeight = 0;
            pMaterialEntry->TextureAtlasHPadding = 0;
            pMaterialEntry->TextureAtlasVPadding = 0;
            pMaterialEntry->TextureAtlasColumns = 0;
            pMaterialEntry->TextureAtlasRows = 0;
            pMaterialEntry->ScrollDirection.SetZero();
            pMaterialEntry->ScrollSpeed = 0.0f;
            m_materials.Add(pMaterialEntry);
        }

        // return colour
        pDestinationVisual->Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_COLOR;
        pDestinationVisual->MaterialIndex = materialIndex;
        pDestinationVisual->Color = PixelFormatHelpers::ConvertSRGBToLinear(pSourceVisual->Color);
        pDestinationVisual->MinUV.Set(0.0f, 0.0f, 0.0f);
        pDestinationVisual->MaxUV.Set(1.0f, 1.0f, 0.0f);
        pDestinationVisual->AtlasUVRange.Set(0.0f, 0.0f, 0.0f, 0.0f);
        return true;
    }
    else if (pSourceVisual->Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE)
    {
        // find the matching source texture
        for (uint32 i = 0; i < m_sourceTextures.GetSize(); i++)
        {
            SourceTextureEntry *pSourceTextureEntry = m_sourceTextures[i];
            if (pSourceTextureEntry->SourceIndex == pSourceVisual->TextureIndex)
            {
                // found it, copy through
                pDestinationVisual->Type = BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE;
                pDestinationVisual->MaterialIndex = pSourceTextureEntry->MaterialIndex;
                pDestinationVisual->Color = PixelFormatHelpers::ConvertSRGBToLinear(pSourceVisual->Color);

                // is this using a texture atlas?
                const MaterialEntry *pMaterialEntry = m_materials[pSourceTextureEntry->MaterialIndex];
                if (pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS ||
                    pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TEXTURE_ATLAS_WITH_NORMAL_MAP ||
                    pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS ||
                    pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_MASKED_TEXTURE_ATLAS_WITH_NORMAL_MAP ||
                    pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS ||
                    pMaterialEntry->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_TEXTURE_ATLAS_WITH_NORMAL_MAP)
                {
                    // fix up uvs
                    pDestinationVisual->MinUV.Set(0.0f, 0.0f, 0.0f);
                    pDestinationVisual->MaxUV.Set(1.0f, 1.0f, 0.0f);
                    pDestinationVisual->AtlasUVRange.Set(pSourceTextureEntry->MinUV.x, pSourceTextureEntry->MinUV.y, pSourceTextureEntry->MaxUV.x, pSourceTextureEntry->MaxUV.y);
                }
                else
                {
                    // copy uvs
                    pDestinationVisual->MinUV = pSourceTextureEntry->MinUV;
                    pDestinationVisual->MaxUV = pSourceTextureEntry->MaxUV;
                    pDestinationVisual->AtlasUVRange.Set(0.0f, 0.0f, 0.0f, 0.0f);
                }

                return true;
            }
        }
    }

    // shouldn't be reached
    UnreachableCode();
    return false;
}

bool BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas()
{
    return false;
    /*
    uint32 i;
    uint32 textureIndex, arrayIndex, mipIndex, materialIndex;
    TextureEntry *pTextureEntry;

    for (textureIndex = 0; textureIndex < m_textures.GetSize(); textureIndex++)
    {
        pTextureEntry = m_textures[textureIndex];
        if (pTextureEntry->GeneratedTexture.GetArraySize() > 1)
        {
            // found a texture array.
            TextureGenerator &textureGenerator = pTextureEntry->GeneratedTexture;
            uint32 tileWidth = textureGenerator.GetWidth();
            uint32 tileHeight = textureGenerator.GetHeight();
            uint32 tileCount = textureGenerator.GetArraySize();
            Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Tile width: %u", tileWidth);
            Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Tile height: %u", tileHeight);
            Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Tile count: %u", tileCount);

            // padding size
            //uint32 paddingSize = 4;
            const uint32 paddingSize = 0;
            uint32 doublePaddingSize = paddingSize * 2;
            Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Padding: %u", paddingSize);

            // figure out how many columns and rows we'll need
            uint32 textureWidth, textureHeight;
            uint32 textureTileColumns, textureTileRows;

            // find the smallest texture size required to represent all of these columns and rows, with padding
            uint32 currentTextureSize = 1;
            for (;;)
            {
                if (currentTextureSize > 16384)
                {
                    Log_WarningPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Skipping converting texture %u to atlas, size would exceed 16384", textureIndex);
                    goto NEXTTEXTURE;
                }

                // enough to fit padding?
                if (currentTextureSize > paddingSize)
                {
                    // using this texture size, how many tiles would we fit in each row/column
                    uint32 fitTileCountX = (currentTextureSize - paddingSize) / (tileWidth + doublePaddingSize);
                    uint32 fitTileCountY = (currentTextureSize - paddingSize) / (tileHeight + doublePaddingSize);

                    // get total number of tiles
                    uint32 nTotalTiles = fitTileCountX * fitTileCountY;
                    if (nTotalTiles >= tileCount)
                    {
                        textureWidth = currentTextureSize;
                        textureHeight = currentTextureSize;
                        textureTileColumns = fitTileCountX;
                        textureTileRows = fitTileCountY;
                        Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Required rows/columns: %u x %u on a %u x %u texture", fitTileCountX, fitTileCountY, currentTextureSize, currentTextureSize);
                        
                        // reduce the height of the texture if it's not needed
                        currentTextureSize = textureHeight;
                        while (currentTextureSize > 1)
                        {
                            currentTextureSize /= 2;
                            fitTileCountY = (currentTextureSize - paddingSize) / (tileHeight + doublePaddingSize);
                            nTotalTiles = fitTileCountX * fitTileCountY;
                            if (nTotalTiles >= tileCount)
                            {
                                textureHeight = currentTextureSize;
                                textureTileRows = fitTileCountY;
                            }
                            else
                                break;
                        }

                        if (textureWidth != textureHeight)
                            Log_DevPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Reduced height of the texture to %u", textureHeight);

                        // break out of upper loop
                        break;
                    }
                }

                // go to next pow2
                currentTextureSize = Y_nextpow2(currentTextureSize + 1);
            }

            // calculate how many mip levels we can have
            uint32 nMipLevels = 0;
            uint32 currentWidth = textureWidth;
            uint32 currentHeight = textureHeight;
            uint32 currentTileWidth = tileWidth;
            uint32 currentTileHeight = tileHeight;
            uint32 currentPaddingSize = paddingSize;
            uint32 currentDoublePaddingSize = doublePaddingSize;
            for (; ;)
            {
                if ((currentPaddingSize + textureTileColumns * (currentTileWidth + currentDoublePaddingSize)) > currentWidth ||
                    (currentPaddingSize + textureTileRows * (currentTileHeight + currentDoublePaddingSize)) > currentHeight)
                {
                    break;
                }

                if (currentWidth > 1)
                    currentWidth /= 2;
                else
                    break;

                if (currentHeight > 1)
                    currentHeight /= 2;
                else
                    break;

                if (currentTileWidth > 1)
                    currentTileWidth /= 2;
                else
                    break;

                if (currentTileHeight > 1)
                    currentTileHeight /= 2;
                else
                    break;

                if (currentPaddingSize > 1)
                {
                    currentDoublePaddingSize = currentPaddingSize;
                    currentPaddingSize /= 2;
                }
                else if (currentPaddingSize != 0)
                {
                    break;
                }
                nMipLevels++;
            }
            DebugAssert(nMipLevels > 0);

            // create image array
            Image *pAtlasImages = new Image[nMipLevels];

            // allocate images
            currentWidth = textureWidth;
            currentHeight = textureHeight;
            for (mipIndex = 0; mipIndex < nMipLevels; mipIndex++)
            {
                pAtlasImages[mipIndex].Create(BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, currentWidth, currentHeight, 1);

                // fill with white pixels (this will fill in the padding)
                Y_memset(pAtlasImages[mipIndex].GetData(), 0xFF, pAtlasImages[mipIndex].GetDataSize());
                //Y_memset(pAtlasImages[mipIndex].GetData(), 0x00, pAtlasImages[mipIndex].GetDataSize());

                if (currentWidth > 1)
                    currentWidth /= 2;
                if (currentHeight > 1)
                    currentHeight /= 2;
            }

            // copy in each image
            for (arrayIndex = 0; arrayIndex < textureGenerator.GetArraySize(); arrayIndex++)
            {
                // work out the row/column we're writing to
                uint32 tileRow = arrayIndex / textureTileColumns;
                uint32 tileColumn = arrayIndex % textureTileColumns;

                // get source image
                const Image *pOriginalSourceImage = textureGenerator.GetImage(arrayIndex, 0);

                // add each mip level
                currentWidth = textureWidth;
                currentHeight = textureHeight;
                currentTileWidth = tileWidth;
                currentTileHeight = tileHeight;
                currentPaddingSize = paddingSize;
                currentDoublePaddingSize = doublePaddingSize;
                for (mipIndex = 0; mipIndex < nMipLevels; mipIndex++)
                {
                    // copy, and resize the image if necessary
                    Image sourceImage;
                    const Image *pMipImage = (mipIndex < textureGenerator.GetMipLevels()) ? textureGenerator.GetImage(arrayIndex, mipIndex) : NULL;
                    if (pMipImage != NULL && pMipImage->GetWidth() == currentTileWidth && pMipImage->GetHeight() == currentTileHeight)
                    {
                        sourceImage.Copy(*pMipImage);
                    }
                    else
                    {
                        if (!sourceImage.CopyAndResize(*pOriginalSourceImage, IMAGE_RESIZE_FILTER_LANCZOS3, currentTileWidth, currentTileHeight, 1))
                        {
                            Log_ErrorPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Failed to resize miplevel %u", mipIndex);
                            delete[] pAtlasImages;
                            return false;
                        }
                    }

                    // get destination image
                    Image &destinationImage = pAtlasImages[mipIndex];

                    // get destination pointer
                    uint32 startX = currentPaddingSize + ((currentTileWidth + currentDoublePaddingSize) * tileColumn);
                    uint32 startY = currentPaddingSize + ((currentTileHeight + currentDoublePaddingSize) * tileRow);

                    // blit the pixels
                    if (!destinationImage.Blit(startX, startY, sourceImage, 0, 0, currentTileWidth, currentTileHeight))
                    {
                        Log_ErrorPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Failed to blit miplevel %u", mipIndex);
                        delete[] pAtlasImages;
                        return false;
                    }

                    // this is a bit of a hack, but due to texture filtering we're gonna repeat the first and last columns of the texture into the padding space.
                    uint32 borderStartX, borderStartY;

                    // left border
                    borderStartX = startX - currentPaddingSize;
                    borderStartY = startY;
                    for (i = 0; i < currentPaddingSize; i++)
                        destinationImage.Blit(borderStartX + i, borderStartY, sourceImage, 0, 0, 1, currentTileHeight);

                    // right border
                    borderStartX = startX + currentTileWidth;
                    borderStartY = startY;
                    for (i = 0; i < currentPaddingSize; i++)
                        destinationImage.Blit(borderStartX + i, borderStartY, sourceImage, currentTileWidth - 1, 0, 1, currentTileHeight);

                    // top border
                    borderStartX = startX;
                    borderStartY = startY - currentPaddingSize;
                    for (i = 0; i < currentPaddingSize; i++)
                        destinationImage.Blit(borderStartX, borderStartY + i, sourceImage, 0, 0, currentTileWidth, 1);

                    // bottom border
                    borderStartX = startX;
                    borderStartY = startY + currentTileHeight;
                    for (i = 0; i < currentPaddingSize; i++)
                        destinationImage.Blit(borderStartX, borderStartY + i, sourceImage, 0, currentTileHeight - 1, currentTileWidth, 1);

                    // halve everything for next miplevel
                    if (currentWidth > 1)
                        currentWidth /= 2;
                    if (currentHeight > 1)
                        currentHeight /= 2;
                    if (currentTileWidth > 1)
                        currentTileWidth /= 2;
                    if (currentTileHeight > 1)
                        currentTileHeight /= 2;
                    if (currentPaddingSize > 1)
                    {
                        currentDoublePaddingSize = currentPaddingSize;
                        currentPaddingSize /= 2;
                    }
                }
            }

            // create the new atlas texture
            TextureEntry *pNewTextureEntry = new TextureEntry;
            pNewTextureEntry->TextureName = pTextureEntry->TextureName;
            if (!pNewTextureEntry->GeneratedTexture.Create(TEXTURE_TYPE_2D, BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, textureWidth, textureHeight, 1, nMipLevels, 1))
            {
                Log_ErrorPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Failed to create texture generator.");
                delete[] pAtlasImages;
                delete pNewTextureEntry;
                return false;
            }

            // allocate and set miplevels
            TextureGenerator &generatedTexture = pNewTextureEntry->GeneratedTexture;
            for (mipIndex = 0; mipIndex < nMipLevels; mipIndex++)
            {
                if (!generatedTexture.SetImage(0, mipIndex, &pAtlasImages[mipIndex]))
                {
                    Log_ErrorPrintf("BlockPaletteCompiler::ConvertTextureArraysToTextureAtlas: Failed to set miplevel %u", mipIndex);
                    delete[] pAtlasImages;
                    delete pNewTextureEntry;
                    return false;
                }
            }

            // set max lod, and addressing modes
            generatedTexture.SetTextureFilter(TEXTURE_FILTER_MIN_MAG_MIP_POINT);
            generatedTexture.SetTextureAddressModeU(TEXTURE_ADDRESS_MODE_CLAMP);
            generatedTexture.SetTextureAddressModeV(TEXTURE_ADDRESS_MODE_CLAMP);

            // delete atlas images, no longer needed
            delete[] pAtlasImages;

            // set remaining fields
            pNewTextureEntry->IsTextureAtlas = true;
            pNewTextureEntry->TextureAtlasTileWidth = tileWidth;
            pNewTextureEntry->TextureAtlasTileHeight = tileHeight;
            pNewTextureEntry->TextureAtlasHPadding = paddingSize;
            pNewTextureEntry->TextureAtlasVPadding = paddingSize;
            pNewTextureEntry->TextureAtlasColumns = textureTileColumns;
            pNewTextureEntry->TextureAtlasRows = textureTileRows;

            // delete and swap
            delete pTextureEntry;
            m_textures[textureIndex] = pNewTextureEntry;

            // change any materials referencing this texture to be an atlas texture
            for (materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
            {
                MaterialEntry *pMaterialEntry = m_materials[materialIndex];
                if (pMaterialEntry->AutoGenDiffuseMapTextureIndex == textureIndex ||
                    pMaterialEntry->AutoGenNormalMapTextureIndex == textureIndex ||
                    pMaterialEntry->AutoGenSpecularMapTextureIndex == textureIndex)
                {
                    if (pMaterialEntry->MaterialType == DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_STATIC_TEXTURE_ARRAY)
                        pMaterialEntry->MaterialType = DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_STATIC_TEXTURE_ATLAS;
                    else if (pMaterialEntry->MaterialType == DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_SCROLLED_TEXTURE_ARRAY)
                        pMaterialEntry->MaterialType = DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_SCROLLED_TEXTURE_ATLAS;
                    else if (pMaterialEntry->MaterialType == DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_TEXTURE_ARRAY)
                        pMaterialEntry->MaterialType = DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_STATIC_TEXTURE_ATLAS;
                    else if (pMaterialEntry->MaterialType == DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SCROLLED_TEXTURE_ARRAY)
                        pMaterialEntry->MaterialType = DF_BLOCK_PALETTE_MATERIAL_TYPE_AUTOGEN_TRANSPARENT_SCROLLED_TEXTURE_ATLAS;
                }
            }
        }
NEXTTEXTURE:
        ;
    }

    //////////////////////////////////////////////////////////////////////////

                            // find a texture that's used
                        const TextureEntry *pTextureEntry = NULL;
                        if (pMaterialEntry->AutoGenDiffuseMapTextureIndex != 0xFFFFFFFF)
                            pTextureEntry = m_textures[pMaterialEntry->AutoGenDiffuseMapTextureIndex];
                        else if (pMaterialEntry->AutoGenSpecularMapTextureIndex != 0xFFFFFFFF)
                            pTextureEntry = m_textures[pMaterialEntry->AutoGenSpecularMapTextureIndex];
                        else if (pMaterialEntry->AutoGenNormalMapTextureIndex != 0xFFFFFFFF)
                            pTextureEntry = m_textures[pMaterialEntry->AutoGenNormalMapTextureIndex];

                        // pull the image width/height, tile width/height
                        uint32 tileIndex = inCubeFaceDef->TextureArrayIndex;
                        uint32 imageWidth = pTextureEntry->GeneratedTexture.GetWidth();
                        uint32 imageHeight = pTextureEntry->GeneratedTexture.GetHeight();
                        uint32 tileWidth = pTextureEntry->TextureAtlasTileWidth;
                        uint32 tileHeight = pTextureEntry->TextureAtlasTileHeight;
                        uint32 paddingX = pTextureEntry->TextureAtlasHPadding;
                        uint32 paddingY = pTextureEntry->TextureAtlasVPadding;
                        uint32 columns = pTextureEntry->TextureAtlasColumns;
                        uint32 rows = pTextureEntry->TextureAtlasRows;
                        float inverseImageWidth = 1.0f / (float)imageWidth;
                        float inverseImageHeight = 1.0f / (float)imageHeight;
                        uint32 doublePaddingX = paddingX * 2;
                        uint32 doublePaddingY = paddingY * 2;

                        // work out tile column, and row
                        DebugAssert(columns > 0 && rows > 0);
                        uint32 tileColumn = tileIndex % columns;
                        uint32 tileRow = tileIndex / columns;

                        // calculate the rect of the atlas image
                        uint32 startX = paddingX + ((tileWidth + doublePaddingX) * tileColumn);
                        uint32 startY = paddingY + ((tileHeight + doublePaddingY) * tileRow);
                        uint32 endX = startX + tileWidth;
                        uint32 endY = startY + tileHeight;

                        // work out the offset of the uvs
                        //float uvOffsetX = 0.5f / (float)imageWidth;
                        //float uvOffsetY = 0.5f / (float)imageHeight;
                        const float uvOffsetX = 0.0f;
                        const float uvOffsetY = 0.0f;

                        // work out the uvs of it
                        outCubeFaceDef->MinUV[0] = 0.0f;
                        outCubeFaceDef->MinUV[1] = 0.0f;
                        outCubeFaceDef->MaxUV[0] = 1.0f;
                        outCubeFaceDef->MaxUV[1] = 1.0f;
                        outCubeFaceDef->AtlasUVRange[0] = (float)startX * inverseImageWidth + uvOffsetX;
                        outCubeFaceDef->AtlasUVRange[1] = (float)startY * inverseImageHeight + uvOffsetY;
                        outCubeFaceDef->AtlasUVRange[2] = (float)(endX - startX) * inverseImageWidth - uvOffsetX;
                        outCubeFaceDef->AtlasUVRange[3] = (float)(endY - startY) * inverseImageHeight - uvOffsetY;
                    }

    //////////////////////////////////////////////////////////////////////////

    return true;*/
}


bool BlockPaletteCompiler::CompileBlockType(const BlockPaletteGenerator::BlockType *pBlockType)
{
    // generate the main part of the block type entry
    BlockTypeEntry *pBTEntry = new BlockTypeEntry();
    pBTEntry->BlockTypeIndex = pBlockType->BlockTypeIndex;
    pBTEntry->Name = pBlockType->Name;
    pBTEntry->Flags = 0;
    pBTEntry->ShapeType = pBlockType->ShapeType;

    // set flags
    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_SOLID)
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_SOLID | BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE;
    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS)
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS;
    if (pBlockType->BlockLightEmitterSettings.Enabled)
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCK_LIGHT_EMITTER;
    if (pBlockType->PointLightEmitterSettings.Enabled)
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_POINT_LIGHT_EMITTER;
    
    // set defaults on remaining fields
    Y_memzero(pBTEntry->CubeShapeFaces, sizeof(pBTEntry->CubeShapeFaces));
    Y_memzero(&pBTEntry->SlabSettings, sizeof(pBTEntry->SlabSettings));
    Y_memzero(&pBTEntry->PlaneSettings, sizeof(pBTEntry->PlaneSettings));
    Y_memzero(&pBTEntry->MeshSettings, sizeof(pBTEntry->MeshSettings));
    Y_memzero(&pBTEntry->BlockLightEmitterSettings, sizeof(pBTEntry->BlockLightEmitterSettings));
    Y_memzero(&pBTEntry->PointLightEmitterSettings, sizeof(pBTEntry->PointLightEmitterSettings));

    if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE || 
        pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB ||
        pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
    {
        // update flags
        // transparent + volume

        if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        {
            // cubes block visibility
            pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE | BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY;
        }
        else if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
        {
            // slabs may not block visibility
            pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE | BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY;
            pBTEntry->SlabSettings.Height = pBlockType->SlabShapeSettings.Height;
        }
        else if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
        {
            // stairs don't block visibility
            pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE;
        }

        if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME)
            pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME;
        if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_UNTILEABLE)
            pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_UNTILEABLE;

        // do faces
        for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
        {
            const BlockPaletteGenerator::BlockType::CubeShapeFace *pSourceFace = &pBlockType->CubeShapeFaces[i];
            BlockTypeEntry::CubeShapeFace *pDestinationFace = &pBTEntry->CubeShapeFaces[i];

            if (pSourceFace->Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE &&
                (m_pGenerator->GetTexture(pSourceFace->Visual.TextureIndex)->Blending == BLOCK_MESH_TEXTURE_BLENDING_MASKED ||
                 m_pGenerator->GetTexture(pSourceFace->Visual.TextureIndex)->Blending == BLOCK_MESH_TEXTURE_BLENDING_TRANSLUCENT))
            {
                pBTEntry->Flags &= ~(BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY);
            }
            else if (pSourceFace->Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_MATERIAL)
            {
                // assume materials don't block visibility
                pBTEntry->Flags &= ~(BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY);
            }

            // build visual
            if (!CompileBlockTypeVisual(&pSourceFace->Visual, &pDestinationFace->Visual))
                return false;
        }
    }
    else if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
    {
        // build visual
        if (!CompileBlockTypeVisual(&pBlockType->PlaneShapeSettings.Visual, &pBTEntry->PlaneSettings.Visual))
            return false;

        // is a visible block
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE;

        //if (pBlockType->PlaneShapeSettings.Visual.Type == BLOCK_MESH_BLOCK_TYPE_VISUAL_TYPE_TEXTURE && m_pGenerator->GetTexture(pBlockType->PlaneShapeSettings.Visual.TextureIndex)->Blending == BLOCK_MESH_TEXTURE_BLENDING_TRANSLUCENT)
            //pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY;

        pBTEntry->PlaneSettings.OffsetX = pBlockType->PlaneShapeSettings.OffsetX;
        pBTEntry->PlaneSettings.OffsetY = pBlockType->PlaneShapeSettings.OffsetY;
        pBTEntry->PlaneSettings.Width = pBlockType->PlaneShapeSettings.Width;
        pBTEntry->PlaneSettings.Height = pBlockType->PlaneShapeSettings.Height;
        pBTEntry->PlaneSettings.BaseRotation = pBlockType->PlaneShapeSettings.BaseRotation;
        pBTEntry->PlaneSettings.RepeatCount = pBlockType->PlaneShapeSettings.RepeatCount;
        pBTEntry->PlaneSettings.RepeatRotation = pBlockType->PlaneShapeSettings.RepeatRotation;
    }
    else if (pBTEntry->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
    {
        // look for the mesh in the list of names
        uint32 meshNameIndex = 0;
        while (meshNameIndex < m_meshNames.GetSize())
        {
            if (m_meshNames[meshNameIndex]->CompareInsensitive(pBlockType->MeshShapeSettings.MeshName))
                break;

            meshNameIndex++;
        }

        // add if not found
        if (meshNameIndex == m_meshNames.GetSize())
            m_meshNames.Add(&pBlockType->MeshShapeSettings.MeshName);

        // set data
        pBTEntry->Flags |= BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE;
        pBTEntry->MeshSettings.MeshNameIndex = meshNameIndex;
        pBTEntry->MeshSettings.Scale = pBlockType->MeshShapeSettings.Scale;
    }

    // block light emitter
    if (pBlockType->BlockLightEmitterSettings.Enabled)
        pBTEntry->BlockLightEmitterSettings.Radius = pBlockType->BlockLightEmitterSettings.Radius;

    // point light emitter
    if (pBlockType->PointLightEmitterSettings.Enabled)
    {
        pBTEntry->PointLightEmitterSettings.Offset = pBlockType->PointLightEmitterSettings.Offset;
        pBTEntry->PointLightEmitterSettings.Color = PixelFormatHelpers::ConvertSRGBToLinear(pBlockType->PointLightEmitterSettings.Color);
        pBTEntry->PointLightEmitterSettings.Brightness = pBlockType->PointLightEmitterSettings.Brightness;
        pBTEntry->PointLightEmitterSettings.Range = pBlockType->PointLightEmitterSettings.Range;
        pBTEntry->PointLightEmitterSettings.Falloff = pBlockType->PointLightEmitterSettings.Falloff;
    }

//     Log_DevPrintf("BlockType '%s': [ %i, %i, %i, %i, %i, %i ],  [ %i, %i, %i, %i, %i, %i ]",
//                   pBTEntry->Name.GetCharArray(),
//                   (int32)pBTEntry->FaceMaterialIndices[0], (int32)pBTEntry->FaceMaterialIndices[1], (int32)pBTEntry->FaceMaterialIndices[2], 
//                   (int32)pBTEntry->FaceMaterialIndices[3], (int32)pBTEntry->FaceMaterialIndices[4], (int32)pBTEntry->FaceMaterialIndices[5],
//                   (int32)pBTEntry->FaceTextureArrayIndices[0], (int32)pBTEntry->FaceTextureArrayIndices[1], (int32)pBTEntry->FaceTextureArrayIndices[2],
//                   (int32)pBTEntry->FaceTextureArrayIndices[3], (int32)pBTEntry->FaceTextureArrayIndices[4], (int32)pBTEntry->FaceTextureArrayIndices[5]
//                   );

    m_blockTypes.Add(pBTEntry);
    return true;
}

bool BlockPaletteCompiler::WriteHeader(ByteStream *pStream)
{
    // write it out to the file
    DF_BLOCK_PALETTE_LIST_HEADER blockListHeader;
    blockListHeader.Magic = DF_BLOCK_PALETTE_HEADER_MAGIC;
    blockListHeader.HeaderSize = sizeof(blockListHeader);
    blockListHeader.BlockTypeCount = m_blockTypes.GetSize();
    blockListHeader.TextureCount = m_textures.GetSize();
    blockListHeader.MaterialCount = m_materials.GetSize();
    blockListHeader.MeshCount = m_meshNames.GetSize();
    if (!pStream->Write2(&blockListHeader, sizeof(blockListHeader)))
        return false;

    return true;
}

bool BlockPaletteCompiler::WriteBlockTypes(ByteStream *pStream, ChunkFileWriter &cfw)
{
    // for block types
    cfw.BeginChunk(DF_BLOCK_PALETTE_CHUNK_BLOCK_TYPES);
    for (uint32 i = 0; i < m_blockTypes.GetSize(); i++)
    {
        const BlockTypeEntry *inBlockType = m_blockTypes[i];
        DF_BLOCK_PALETTE_BLOCK_TYPE outBlockType;

        outBlockType.BlockTypeIndex = inBlockType->BlockTypeIndex;
        outBlockType.NameStringIndex = cfw.AddString(inBlockType->Name);
        outBlockType.Flags = inBlockType->Flags;
        outBlockType.ShapeType = inBlockType->ShapeType;

        // initialize defaults
        Y_memzero(outBlockType.CubeShapeFaces, sizeof(outBlockType.CubeShapeFaces));

        // fill in data
        if (inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE ||
            inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB ||
            inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
        {
            if (inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                outBlockType.SlabSettings.Height = inBlockType->SlabSettings.Height;

            for (uint32 j = 0; j < CUBE_FACE_COUNT; j++)
            {
                const BlockTypeEntry::CubeShapeFace *inCubeFaceDef = &inBlockType->CubeShapeFaces[j];
                DF_BLOCK_PALETTE_BLOCK_TYPE::CubeShapeFace *outCubeFaceDef = &outBlockType.CubeShapeFaces[j];

                outCubeFaceDef->Visual.Type = inCubeFaceDef->Visual.Type;
                outCubeFaceDef->Visual.MaterialIndex = inCubeFaceDef->Visual.MaterialIndex;
                outCubeFaceDef->Visual.Color = inCubeFaceDef->Visual.Color;
                inCubeFaceDef->Visual.MinUV.Store(outCubeFaceDef->Visual.MinUV);
                inCubeFaceDef->Visual.MaxUV.Store(outCubeFaceDef->Visual.MaxUV);
                inCubeFaceDef->Visual.AtlasUVRange.Store(outCubeFaceDef->Visual.AtlasUVRange);
            }
        }
        else if (inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
        {
            const BlockTypeEntry::PlaneShape *inPlaneSettings = &inBlockType->PlaneSettings;
            DF_BLOCK_PALETTE_BLOCK_TYPE::PlaneShape *outPlaneSettings = &outBlockType.PlaneSettings;

            outPlaneSettings->Visual.Type = inPlaneSettings->Visual.Type;
            outPlaneSettings->Visual.MaterialIndex = inPlaneSettings->Visual.MaterialIndex;
            outPlaneSettings->Visual.Color = inPlaneSettings->Visual.Color;
            inPlaneSettings->Visual.MinUV.Store(outPlaneSettings->Visual.MinUV);
            inPlaneSettings->Visual.MaxUV.Store(outPlaneSettings->Visual.MaxUV);
            inPlaneSettings->Visual.AtlasUVRange.Store(outPlaneSettings->Visual.AtlasUVRange);

            outPlaneSettings->OffsetX = inPlaneSettings->OffsetX;
            outPlaneSettings->OffsetY = inPlaneSettings->OffsetY;
            outPlaneSettings->Width = inPlaneSettings->Width;
            outPlaneSettings->Height = inPlaneSettings->Height;
            outPlaneSettings->BaseRotation = inPlaneSettings->BaseRotation;
            outPlaneSettings->RepeatCount = inPlaneSettings->RepeatCount;
            outPlaneSettings->RepeatRotation = inPlaneSettings->RepeatRotation;
        }
        else if (inBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
        {
            const BlockTypeEntry::MeshShape *inMeshSettings = &inBlockType->MeshSettings;
            DF_BLOCK_PALETTE_BLOCK_TYPE::MeshShape *outMeshSettings = &outBlockType.MeshSettings;

            outMeshSettings->MeshIndex = inMeshSettings->MeshNameIndex;
            outMeshSettings->Scale = inMeshSettings->Scale;
        }

        // block light emitter
        outBlockType.BlockLightEmitterSettings.Radius = inBlockType->BlockLightEmitterSettings.Radius;

        // point light emitter
        inBlockType->PointLightEmitterSettings.Offset.Store(outBlockType.PointLightEmitterSettings.Offset);
        outBlockType.PointLightEmitterSettings.Color = inBlockType->PointLightEmitterSettings.Color;
        outBlockType.PointLightEmitterSettings.Brightness = inBlockType->PointLightEmitterSettings.Brightness;
        outBlockType.PointLightEmitterSettings.Range = inBlockType->PointLightEmitterSettings.Range;
        outBlockType.PointLightEmitterSettings.Falloff = inBlockType->PointLightEmitterSettings.Falloff;

        cfw.WriteChunkData(&outBlockType, sizeof(outBlockType));
    }
    cfw.EndChunk();

    return true;
}

bool BlockPaletteCompiler::WriteTextures(ByteStream *pStream, ChunkFileWriter &cfw, TEXTURE_PLATFORM texturePlatform)
{
    // strip alpha channels from non-blended diffuse textures, specular textures, and normal maps
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
    {
        if (m_materials[i]->TextureBlending == BLOCK_MESH_TEXTURE_BLENDING_NONE && m_materials[i]->DiffuseMapTextureIndex >= 0 && !m_textures[m_materials[i]->DiffuseMapTextureIndex]->RemoveAlphaChannel())
        {
            Log_ErrorPrintf("BlockPaletteCompiler::WriteTextures: Could not remove alpha channel from diffuse texture %u", i);
            return false;
        }

        if (m_materials[i]->SpecularMapTextureIndex >= 0 && !m_textures[m_materials[i]->SpecularMapTextureIndex]->RemoveAlphaChannel())
        {
            Log_ErrorPrintf("BlockPaletteCompiler::WriteTextures: Could not remove alpha channel from specular texture %u", i);
            return false;
        }

        if (m_materials[i]->NormalMapTextureIndex >= 0 && !m_textures[m_materials[i]->NormalMapTextureIndex]->RemoveAlphaChannel())
        {
            Log_ErrorPrintf("BlockPaletteCompiler::WriteTextures: Could not remove alpha channel from normal texture %u", i);
            return false;
        }
    }

    // for textures
    cfw.BeginChunk(DF_BLOCK_PALETTE_CHUNK_TEXTURES);
    if (m_textures.GetSize() > 0)
    {
        ByteStream **ppGeneratedTextureStreams = new ByteStream *[m_textures.GetSize()];
        uint32 nGeneratedTextureStreams = 0;
        uint32 nUsedTextureStreams = 0;

        for (uint32 i = 0; i < m_textures.GetSize(); i++)
        {
            TextureGenerator *pTextureGenerator = m_textures[i];
            Log_DevPrintf("BlockPaletteCompiler::WriteTextures: Compiling texture %u (%ux%u %u subtextures)", i, pTextureGenerator->GetWidth(), pTextureGenerator->GetHeight(), pTextureGenerator->GetArraySize());

            ppGeneratedTextureStreams[nGeneratedTextureStreams] = ByteStream_CreateGrowableMemoryStream(NULL, 0);
            
            if (!pTextureGenerator->Compile(ppGeneratedTextureStreams[nGeneratedTextureStreams], texturePlatform))
            {
                Log_ErrorPrintf("BlockPaletteCompiler::WriteTextures: Failed to compile texture %u", i);

                for (uint32 j = 0; j < i; j++)
                    ppGeneratedTextureStreams[j]->Release();

                delete[] ppGeneratedTextureStreams;
                return false;
            }

            nGeneratedTextureStreams++;
        }

        uint32 currentTextureOffset = sizeof(DF_BLOCK_PALETTE_TEXTURE) * m_textures.GetSize();
        for (uint32 i = 0; i < m_textures.GetSize(); i++)
        {
            DF_BLOCK_PALETTE_TEXTURE outTexture;

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

bool BlockPaletteCompiler::WriteMaterials(ByteStream *pStream, ChunkFileWriter &cfw)
{

    // for materials
    cfw.BeginChunk(DF_BLOCK_PALETTE_CHUNK_MATERIALS);
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
    {
        const MaterialEntry *inMaterial = m_materials[i];
        DF_BLOCK_PALETTE_MATERIAL outMaterial;

        outMaterial.MaterialType = inMaterial->Type;
        outMaterial.MaterialFlags = inMaterial->Flags;

        if (inMaterial->Type != DF_BLOCK_PALETTE_MATERIAL_TYPE_EXTERNAL)
        {
            SmallString materialName;
            materialName.AppendString("autogen:");

            // build material name
            if (inMaterial->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_COLOR)
                materialName.AppendString("color");
            else if (inMaterial->Type == DF_BLOCK_PALETTE_MATERIAL_TYPE_TRANSLUCENT_COLOR)
                materialName.AppendString("color:translucent");
            else
                materialName.AppendFormattedString("texture:%s:%s", NameTable_GetNameString(NameTables::BlockMeshTextureBlending, inMaterial->TextureBlending), NameTable_GetNameString(NameTables::BlockMeshTextureEffect, inMaterial->TextureEffect));

            outMaterial.MaterialNameStringIndex = cfw.AddString(materialName);
        }
        else
        {
            // use external material name
            outMaterial.MaterialNameStringIndex = cfw.AddString(inMaterial->ExternalMaterialName);
        }

        outMaterial.DiffuseMapTextureIndex = inMaterial->DiffuseMapTextureIndex;
        outMaterial.SpecularMapTextureIndex = inMaterial->SpecularMapTextureIndex;
        outMaterial.NormalMapTextureIndex = inMaterial->NormalMapTextureIndex;
        
        float2 textureScrollVector(inMaterial->ScrollDirection * inMaterial->ScrollSpeed);
        textureScrollVector.Store(outMaterial.TextureScrollVector);

        cfw.WriteChunkData(&outMaterial, sizeof(outMaterial));
    }
    cfw.EndChunk();

    return true;
}

bool BlockPaletteCompiler::WriteMeshes(ByteStream *pStream, ChunkFileWriter &cfw)
{
    cfw.BeginChunk(DF_BLOCK_PALETTE_CHUNK_MESHES);
    for (uint32 i = 0; i < m_meshNames.GetSize(); i++)
    {
        DF_BLOCK_PALETTE_MESH outMesh;
        outMesh.MeshNameStringIndex = cfw.AddString(*m_meshNames[i]);
        cfw.WriteChunkData(&outMesh, sizeof(outMesh));
    }
    cfw.EndChunk();

    return true;
}

bool BlockPaletteGenerator::Compile(TEXTURE_PLATFORM texturePlatform, ByteStream *pOutputStream) const
{
    BlockPaletteCompiler compiler(this);
    return compiler.Compile(texturePlatform, pOutputStream);
}

static bool LoadImportedAtlasTexture(Image *pDecodedImage, const char *textureFileName)
{
    Log_DevPrintf("Loading texture atlas '%s'", textureFileName);

    AutoReleasePtr<ByteStream> pInputTextureStream = FileSystem::OpenFile(textureFileName, BYTESTREAM_OPEN_READ);
    if (pInputTextureStream == NULL)
    {
        Log_ErrorPrintf("Failed to open '%s'", textureFileName);
        return false;
    }

    ImageCodec *pImageCodec = ImageCodec::GetImageCodecForStream(textureFileName, pInputTextureStream);
    if (pImageCodec == NULL)
        return false;

    if (!pImageCodec->DecodeImage(pDecodedImage, textureFileName, pInputTextureStream))
        return false;

    // convert to something usable
    if (pDecodedImage->GetPixelFormat() != BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT && !pDecodedImage->ConvertPixelFormat(BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT))
        return false;

    return true;
}

static bool SplitImportedTextureAtlas(const Image *pAtlasImage, uint32 textureTileWidth, uint32 textureTileHeight, uint32 textureTileIndex, Image *pSplitImage)
{
    Log_DevPrintf("Splitting texture atlas at tile %u", textureTileIndex);
    DebugAssert(pAtlasImage->GetPixelFormat() == BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT);

    // make sure the tile index is valid
    uint32 decodedImageTilesPerRow = pAtlasImage->GetWidth() / textureTileWidth;
    uint32 decodedImageRows = pAtlasImage->GetHeight() / textureTileHeight;
    uint32 tileRow = textureTileIndex / decodedImageTilesPerRow;
    uint32 tileCol = textureTileIndex % decodedImageTilesPerRow;
    if (tileRow >= decodedImageRows)
        return false;

    // create a new image using the texture tile width + height
    pSplitImage->Create(BLOCK_MESH_BLOCK_LIST_TEXTURE_INTERNAL_FORMAT, textureTileWidth, textureTileHeight, 1);
    
    // get start point in source image
    uint32 decodedImageRowPitch = pAtlasImage->GetDataRowPitch();
    const byte *pDecodedImagePointer = pAtlasImage->GetData() + (tileRow * textureTileHeight * decodedImageRowPitch) + (4 * tileCol * textureTileWidth);

    // get start point in output image
    DebugAssert(pSplitImage->GetDataRowPitch() == 4 * textureTileWidth);
    uint32 splitImageRowPitch = pSplitImage->GetDataRowPitch();
    byte *pSplitImagePointer = pSplitImage->GetData();

    // blit the pixels
    for (uint32 i = 0; i < textureTileHeight; i++)
    {
        Y_memcpy(pSplitImagePointer, pDecodedImagePointer, 4 * textureTileWidth);
        pDecodedImagePointer += decodedImageRowPitch;
        pSplitImagePointer += splitImageRowPitch;
    }

    // all done
    return true;
}

bool BlockPaletteGenerator::ImportTextureAtlas(const char *namePrefix, const char *diffuseMapFileName, const char *specularMapFileName, const char *normalMapFileName, uint32 tilesWide, uint32 tilesHigh, const bool *pImportTileIndices, uint32 maxImportTileIndices, String *pOutTextureNames, uint32 maxOutTextureNames)
{
    uint32 i;
    SmallString textureName;

    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;

    int32 minImageWidth, minImageHeight;
    int32 maxImageWidth, maxImageHeight;

    Image diffuseMapImage;
    Image specularMapImage;
    Image normalMapImage;
    Image splitImage;

    // load the textures
    minImageWidth = minImageHeight = INT_MAX;
    maxImageWidth = maxImageHeight = -INT_MAX;

    // diffuse map
    hasDiffuseMap = (diffuseMapFileName != NULL);
    if (hasDiffuseMap)
    {
        if (!LoadImportedAtlasTexture(&diffuseMapImage, diffuseMapFileName))
        {
            Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to import diffuse map texture '%s'", diffuseMapFileName);
            return false;
        }

        minImageWidth = Min(minImageWidth, (int32)diffuseMapImage.GetWidth());
        minImageHeight = Min(minImageHeight, (int32)diffuseMapImage.GetHeight());
        maxImageWidth = Max(maxImageWidth, (int32)diffuseMapImage.GetWidth());
        maxImageHeight = Max(maxImageWidth, (int32)diffuseMapImage.GetHeight());
    }

    // specular map
    hasSpecularMap = (specularMapFileName != NULL);
    if (hasSpecularMap)
    {
        if (!LoadImportedAtlasTexture(&specularMapImage, specularMapFileName))
        {
            Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to import specular map texture '%s'", specularMapFileName);
            return false;
        }

        minImageWidth = Min(minImageWidth, (int32)specularMapImage.GetWidth());
        minImageHeight = Min(minImageHeight, (int32)specularMapImage.GetHeight());
        maxImageWidth = Max(maxImageWidth, (int32)specularMapImage.GetWidth());
        maxImageHeight = Max(maxImageWidth, (int32)specularMapImage.GetHeight());
    }

    // normal map
    hasNormalMap = (normalMapFileName != NULL);
    if (hasNormalMap)
    {
        if (!LoadImportedAtlasTexture(&normalMapImage, normalMapFileName))
        {
            Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to import normal map texture '%s'", normalMapFileName);
            return false;
        }

        minImageWidth = Min(minImageWidth, (int32)normalMapImage.GetWidth());
        minImageHeight = Min(minImageHeight, (int32)normalMapImage.GetHeight());
        maxImageWidth = Max(maxImageWidth, (int32)normalMapImage.GetWidth());
        maxImageHeight = Max(maxImageWidth, (int32)normalMapImage.GetHeight());
    }

    // no images?
    if (!hasDiffuseMap && !hasSpecularMap && !hasNormalMap)
        return false;

    // images should all be of the same dimension
    if (minImageWidth != maxImageWidth || minImageHeight != maxImageHeight)
    {
        Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: One or more textures are of differing dimensions: min (%i,%i) vs max (%i,%i)", minImageWidth, minImageHeight, maxImageWidth, maxImageHeight);
        return false;
    }

    // determine the number of tiles in the image
    uint32 imageWidth = (uint32)maxImageWidth;
    uint32 imageHeight = (uint32)maxImageHeight;
    uint32 tileWidth = imageWidth / tilesWide;
    uint32 tileHeight = imageHeight / tilesHigh;

    // determine total number of images
    uint32 nTotalTiles = tilesWide * tilesHigh;
    if (nTotalTiles < 1)
        return false;

    // import each image
    for (i = 0; i < nTotalTiles; i++)
    {
        if (pImportTileIndices != NULL && (i >= maxImportTileIndices || !pImportTileIndices[i]))
            continue;

        // create texture
        textureName.Format("%s_tile_%u", namePrefix, i);
        const Texture *pTexture = CreateTexture(textureName, BLOCK_MESH_TEXTURE_BLENDING_NONE, true);
        if (pTexture == nullptr)
            return false;

        // add maps to texture
        if (hasDiffuseMap)
        {
            if (!SplitImportedTextureAtlas(&diffuseMapImage, tileWidth, tileHeight, i, &splitImage) ||
                !SetTextureDiffuseMap(pTexture->Index, &splitImage))
            {
                Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to split or set diffuse map tile %u", i);
                return false;
            }
        }
        if (hasSpecularMap)
        {
            if (!SplitImportedTextureAtlas(&specularMapImage, tileWidth, tileHeight, i, &splitImage) ||
                !SetTextureDiffuseMap(pTexture->Index, &splitImage))
            {
                Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to split or set specular map tile %u", i);
                return false;
            }
        }   
        if (hasNormalMap)
        {
            if (!SplitImportedTextureAtlas(&normalMapImage, tileWidth, tileHeight, i, &splitImage) ||
                !SetTextureDiffuseMap(pTexture->Index, &splitImage))
            {
                Log_ErrorPrintf("BlockPaletteGenerator::ImportTextureAtlas: Failed to split or set normal map tile %u", i);
                return false;
            }
        }

        // set imported index
        if (pOutTextureNames != NULL && i < maxOutTextureNames)
            pOutTextureNames[i] = m_textures[pTexture->Index]->Name;
    }

    // done
    return true;
}

// Interface
BinaryBlob *ResourceCompiler::CompileBlockPalette(ResourceCompilerCallbacks *pCallbacks, TEXTURE_PLATFORM platform, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.blp.zip", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileBlockPalette: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    BlockPaletteGenerator *pGenerator = new BlockPaletteGenerator();
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
    if (!pGenerator->Compile(platform, pOutputStream))
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
