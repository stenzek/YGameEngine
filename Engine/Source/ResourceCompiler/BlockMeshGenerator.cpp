#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/BlockMeshGenerator.h"
#include "ResourceCompiler/BlockPaletteGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "ResourceCompiler/CollisionShapeGenerator.h"
#include "Engine/BlockPalette.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/DataFormats.h"
#include "Engine/ResourceManager.h"
#include "Engine/BlockMesh.h"
#include "Core/ChunkFileWriter.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(BlockMeshGenerator);

BlockMeshGenerator::BlockMeshGenerator()
    : m_changed(false),
      m_pPalette(NULL),
      m_scale(1.0f),
      m_useAmbientOcclusion(false),
      m_collisionShapeType(Physics::COLLISION_SHAPE_TYPE_NONE),
      m_pLODLevels(NULL),
      m_nLODLevels(0)
{

}

BlockMeshGenerator::~BlockMeshGenerator()
{
    delete[] m_pLODLevels;

    if (m_pPalette != NULL)
        m_pPalette->Release();
}

void BlockMeshGenerator::SetScale(float scale)
{
    float currentScale = scale;
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        m_pLODLevels[i].SetScale(currentScale);
        currentScale *= 2.0f;
    }
}

bool BlockMeshGenerator::Create(const BlockPalette *pPalette, float scale /* = 1.0f */, Physics::COLLISION_SHAPE_TYPE collisionShapeType /* = COLLISION_SHAPE_TYPE_TRIANGLE_MESH */)
{
    m_pPalette = pPalette;
    m_pPalette->AddRef();
    m_scale = scale;
    m_collisionShapeType = collisionShapeType;

    m_nLODLevels = 1;
    m_pLODLevels = new BlockMeshVolume[1];
    m_pLODLevels[0].SetPalette(pPalette);
    m_pLODLevels[0].SetScale(scale);
    m_pLODLevels[0].Resize(int3::Zero, int3::Zero);
    m_pLODLevels[0].Clear();

    m_changed = true;
    return true;
}

bool BlockMeshGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        xmlReader.PrintError("failed to create XML reader");
        return false;
    }

    // skip to correct node
    if (!xmlReader.SkipToElement("blockmesh"))
    {
        xmlReader.PrintError("failed to skip to blockmesh element");
        return false;
    }

    // read attributes
    const char *nameStr = xmlReader.FetchAttribute("palette");
    const char *scaleStr = xmlReader.FetchAttribute("scale");
    const char *lodCountStr = xmlReader.FetchAttribute("lod-count");
    const char *ambientOcclusionEnabledStr = xmlReader.FetchAttribute("ambient-occlusion");
    const char *collisionShapeTypeStr = xmlReader.FetchAttribute("collision-shape-type");

    // check
    if (nameStr == NULL || scaleStr == NULL || lodCountStr == NULL || ambientOcclusionEnabledStr == NULL || collisionShapeTypeStr == NULL)
    {
        xmlReader.PrintError("incomplete mesh definition");
        return false;
    }

    // load palette
    m_pPalette = g_pResourceManager->GetBlockPalette(nameStr);
    if (m_pPalette == NULL)
    {
        xmlReader.PrintError("failed to load palette '%s'", nameStr);
        return false;
    }

    // load settings
    m_scale = StringConverter::StringToFloat(scaleStr);
    m_nLODLevels = StringConverter::StringToUInt32(lodCountStr);
    m_useAmbientOcclusion = StringConverter::StringToBool(ambientOcclusionEnabledStr);
    if (!NameTable_TranslateType(Physics::NameTables::CollisionShapeType, collisionShapeTypeStr, &m_collisionShapeType, true))
    {
        xmlReader.PrintError("unknown collision shape type '%s'", collisionShapeTypeStr);
        return false;
    }

    // validate
    if (m_scale <= 0.0f ||
        m_nLODLevels == 0 || m_nLODLevels > BlockMesh::MAX_LOD_LEVELS)
    {
        xmlReader.PrintError("invalid scale: %f, or lod level count: %u", m_scale, m_nLODLevels);
        return false;
    }

    // alloc lods
    m_pLODLevels = new BlockMeshVolume[m_nLODLevels];
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        m_pLODLevels[i].SetPalette(m_pPalette);
        m_pLODLevels[i].SetScale(m_scale * (float)(1 << i));
    }

    // skip to data
    if (xmlReader.IsEmptyElement() || !xmlReader.MoveToElement())
    {
        xmlReader.PrintError("blockmesh element should have content");
        return false;
    }

    // start parsing xml
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            if (xmlReader.Select("lod") < 0)
                return false;

            const char *lodLevelStr = xmlReader.FetchAttribute("level");
            uint32 lodLevelIndex;
            if (lodLevelStr == NULL ||
                (lodLevelIndex = StringConverter::StringToUInt32(lodLevelStr)) >= m_nLODLevels ||
                m_pLODLevels[lodLevelIndex].GetWidth() > 0)
            {
                xmlReader.PrintError("invalid lod definition");
                return false;
            }

            // get range
            const char *minXStr = xmlReader.FetchAttribute("minx");
            const char *minYStr = xmlReader.FetchAttribute("miny");
            const char *minZStr = xmlReader.FetchAttribute("minz");
            const char *maxXStr = xmlReader.FetchAttribute("maxx");
            const char *maxYStr = xmlReader.FetchAttribute("maxy");
            const char *maxZStr = xmlReader.FetchAttribute("maxz");
            if (minXStr == NULL || minYStr == NULL || minZStr == NULL ||
                maxXStr == NULL || maxYStr == NULL || maxZStr == NULL)
            {
                xmlReader.PrintError("incomplete mesh declaration");
                return false;
            }

            int3 minCoordinates(StringConverter::StringToInt32(minXStr), StringConverter::StringToInt32(minYStr), StringConverter::StringToInt32(minZStr));
            int3 maxCoordinates(StringConverter::StringToInt32(maxXStr), StringConverter::StringToInt32(maxYStr), StringConverter::StringToInt32(maxZStr));
            if (minCoordinates.AnyGreater(maxCoordinates))
            {
                xmlReader.PrintError("invalid mesh size");
                return false;
            }

            // init volume
            BlockMeshVolume &meshVolume = m_pLODLevels[lodLevelIndex];
            meshVolume.Resize(minCoordinates, maxCoordinates);
            meshVolume.Clear();

            // read block info
            if (!xmlReader.IsEmptyElement())
            {
                for (;;)
                {
                    if (!xmlReader.NextToken())
                        return false;

                    if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                    {
                        if (xmlReader.Select("block") < 0)
                            return false;

                        const char *xStr = xmlReader.FetchAttribute("x");
                        const char *yStr = xmlReader.FetchAttribute("y");
                        const char *zStr = xmlReader.FetchAttribute("z");
                        const char *typeStr = xmlReader.FetchAttribute("type");
                        if (xStr == NULL || yStr == NULL || zStr == NULL || typeStr == NULL)
                        {
                            xmlReader.PrintError("incomplete block declaration");
                            return false;
                        }

                        int3 at(StringConverter::StringToInt32(xStr), StringConverter::StringToInt32(yStr), StringConverter::StringToInt32(zStr));
                        uint32 t = StringConverter::StringToUInt32(typeStr);

                        if (at.AnyLess(minCoordinates) || at.AnyGreater(maxCoordinates) || t > BLOCK_MESH_MAX_BLOCK_TYPES || !m_pPalette->GetBlockType(t)->IsAllocated)
                        {
                            xmlReader.PrintError("invalid block declaration.");
                            return false;
                        }

                        meshVolume.SetBlock(at, (uint8)t);
                    }
                    else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                    {
                        DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "lod") == 0);
                        break;
                    }
                    else
                    {
                        UnreachableCode();
                    }
                }
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "blockmesh") == 0);
            break;
        }
    }

    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        if (m_pLODLevels[i].GetWidth() == 0)
        {
            xmlReader.PrintError("lod level %u not defined", i);
            return false;
        }
    }

    m_changed = false;
    return true;
}

bool BlockMeshGenerator::SaveToXML(ByteStream *pStream)
{
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("blockmesh");
    {
        xmlWriter.WriteAttribute("palette", m_pPalette->GetName());
        xmlWriter.WriteAttributef("scale", "%f", m_scale);
        xmlWriter.WriteAttributef("lod-count", "%u", m_nLODLevels);
        xmlWriter.WriteAttribute("ambient-occlusion", StringConverter::BoolToString(m_useAmbientOcclusion));
        xmlWriter.WriteAttribute("collision-shape-type", NameTable_GetNameString(Physics::NameTables::CollisionShapeType, m_collisionShapeType));

        for (uint32 i = 0; i < m_nLODLevels; i++)
        {
            const BlockMeshVolume &meshVolume = m_pLODLevels[i];

            xmlWriter.StartElement("lod");
            {
                int32 minX = meshVolume.GetMinCoordinates().x;
                int32 minY = meshVolume.GetMinCoordinates().y;
                int32 minZ = meshVolume.GetMinCoordinates().z;
                int32 maxX = meshVolume.GetMaxCoordinates().x;
                int32 maxY = meshVolume.GetMaxCoordinates().y;
                int32 maxZ = meshVolume.GetMaxCoordinates().z;

                xmlWriter.WriteAttributef("level", "%u", i);
                xmlWriter.WriteAttributef("minx", "%i", minX);
                xmlWriter.WriteAttributef("miny", "%i", minY);
                xmlWriter.WriteAttributef("minz", "%i", minZ);
                xmlWriter.WriteAttributef("maxx", "%i", maxX);
                xmlWriter.WriteAttributef("maxy", "%i", maxY);
                xmlWriter.WriteAttributef("maxz", "%i", maxZ);

                for (int32 z = minZ; z <= maxZ; z++)
                {
                    for (int32 y = minY; y <= maxY; y++)
                    {
                        for (int32 x = minX; x <= maxX; x++)
                        {
                            uint8 blockValue = GetBlock(i, x, y, z);
                            if (blockValue == 0)
                                continue;

                            xmlWriter.StartElement("block");
                            {
                                xmlWriter.WriteAttributef("x", "%i", x);
                                xmlWriter.WriteAttributef("y", "%i", y);
                                xmlWriter.WriteAttributef("z", "%i", z);
                                xmlWriter.WriteAttributef("type", "%u", (uint32)blockValue);
                            }
                            xmlWriter.EndElement();
                        }
                    }
                }
            }
            xmlWriter.EndElement();
        }
    }
    xmlWriter.EndElement();

    m_changed = false;
    return (!xmlWriter.InErrorState() && !pStream->InErrorState());
}

void BlockMeshGenerator::Clear()
{
    for (uint32 i = 0; i < m_nLODLevels; i++)
        Clear(i);
}

void BlockMeshGenerator::Clear(uint32 LOD)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.Clear();
    m_changed = true;
}

void BlockMeshGenerator::Resize(uint32 LOD, const int3 &newMinCoordinates, const int3 &newMaxCoordinates)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.Resize(newMinCoordinates, newMaxCoordinates);
    m_changed = true;
}

bool BlockMeshGenerator::GetActiveCoordinatesRange(uint32 LOD, int3 *pMinActiveCoordinates, int3 *pMaxActiveCoordinates)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    return meshVolume.GetActiveCoordinatesRange(pMinActiveCoordinates, pMaxActiveCoordinates);
}

void BlockMeshGenerator::Center()
{
    for (uint32 i = 0; i < m_nLODLevels; i++)
        Center(i);
}

void BlockMeshGenerator::Center(uint32 LOD)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.Center();
    m_changed = true;
}

void BlockMeshGenerator::Shrink()
{
    for (uint32 i = 0; i < m_nLODLevels; i++)
        Shrink(i);
}

void BlockMeshGenerator::Shrink(uint32 LOD)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.Shrink();
    m_changed = true;
}

void BlockMeshGenerator::MoveBlock(uint32 LOD, const int3 &blockCoordinates, const int3 &moveDelta)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.MoveBlock(blockCoordinates, moveDelta);
    m_changed = true;
}

void BlockMeshGenerator::MoveBlocks(uint32 LOD, const int3 &selectionMin, const int3 &selectionMax, const int3 &moveDelta)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    meshVolume.MoveBlocks(selectionMin, selectionMax, moveDelta);
    m_changed = true;
}

uint8 BlockMeshGenerator::GetBlock(uint32 LOD, const int3 &coords) const
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    if (coords.AnyLess(meshVolume.GetMinCoordinates()) || coords.AnyGreater(meshVolume.GetMaxCoordinates()))
    {
        Log_WarningPrintf("BlockMeshGenerator::GetBlock: Reading out-of-range block (%i,%i,%i)", coords.x, coords.y, coords.z);
        return 0;
    }

    return meshVolume.GetBlock(coords);
}

void BlockMeshGenerator::SetBlock(uint32 LOD, const int3 &coords, BlockVolumeBlockType v)
{
    BlockMeshVolume &meshVolume = m_pLODLevels[LOD];
    DebugAssert(LOD < m_nLODLevels);

    if (coords.AnyLess(meshVolume.GetMinCoordinates()) || coords.AnyGreater(meshVolume.GetMaxCoordinates()))
    {
        Log_WarningPrintf("BlockMeshGenerator::SetBlock: Setting out-of-range block (%i,%i,%i). Resizing mesh.", coords.x, coords.y, coords.z);
        
        int3 newMinCoordinates = coords.Min(meshVolume.GetMinCoordinates());
        int3 newMaxCoordinates = coords.Max(meshVolume.GetMaxCoordinates());
        Resize(LOD, newMinCoordinates, newMaxCoordinates);
    }

    meshVolume.SetBlock(coords, v);
    m_changed = true;
}

void BlockMeshGenerator::SetPalette(const BlockPalette *pPalette)
{
    if (m_pPalette == pPalette)
        return;

    m_pPalette->Release();
    m_pPalette = pPalette;
    m_pPalette->AddRef();

    // check blocks
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        BlockMeshVolume &meshVolume = m_pLODLevels[i];
        meshVolume.SetPalette(pPalette);
    
        int32 minX = meshVolume.GetMinCoordinates().x;
        int32 minY = meshVolume.GetMinCoordinates().y;
        int32 minZ = meshVolume.GetMinCoordinates().z;
        int32 maxX = meshVolume.GetMaxCoordinates().x;
        int32 maxY = meshVolume.GetMaxCoordinates().y;
        int32 maxZ = meshVolume.GetMaxCoordinates().z;

        int32 x, y, z;
        for (z = minZ; z <= maxZ; z++)
        {
            for (y = minY; y <= maxY; y++)
            {
                for (x = minX; x <= maxX; x++)
                {
                    BlockVolumeBlockType blockValue = GetBlock(i, x, y, z);
                    if (blockValue != 0 && !pPalette->GetBlockType(blockValue)->IsAllocated)
                        SetBlock(i, x, y, z, 0);
                }
            }
        }
    }

    m_changed = true;
}

bool BlockMeshGenerator::Compile(ByteStream *pOutputStream) const
{
    // calculate bounding box
    AABox boundingBox;
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        // update bounds
        AABox levelBoundingBox(m_pLODLevels[i].CalculateActiveBoundingBox());        
        if (i == 0)
            boundingBox = levelBoundingBox;
        else
            boundingBox.Merge(levelBoundingBox);
    }

    // from this calculate bounding sphere
    Sphere boundingSphere(Sphere::FromAABox(boundingBox));

    // write to file
    BinaryWriter binaryWriter(pOutputStream);

    // magic
    binaryWriter.WriteUInt32(DF_BLOCK_MESH_HEADER_MAGIC);

    // blocklist name
    binaryWriter.WriteSizePrefixedString(m_pPalette->GetName());

    // settings
    binaryWriter.WriteFloat(m_scale);
    binaryWriter.WriteBool(m_useAmbientOcclusion);
    binaryWriter.WriteUInt32(m_nLODLevels);

    // bounding box
    binaryWriter << (boundingBox.GetMinBounds());
    binaryWriter << (boundingBox.GetMaxBounds());

    // bounding sphere
    binaryWriter << (boundingSphere.GetCenter());
    binaryWriter << (boundingSphere.GetRadius());

    // write lod levels
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        const BlockMeshVolume &meshVolume = m_pLODLevels[i];
        AABox levelBoundingBox(meshVolume.CalculateActiveBoundingBox());
        Sphere levelBoundingSphere(Sphere::FromAABox(levelBoundingBox));

        binaryWriter << (meshVolume.GetScale());
        binaryWriter << (levelBoundingBox.GetMinBounds());
        binaryWriter << (levelBoundingBox.GetMaxBounds());
        binaryWriter << (levelBoundingSphere.GetCenter());
        binaryWriter << (levelBoundingSphere.GetRadius());

        binaryWriter << (meshVolume.GetMinCoordinates());
        binaryWriter << (meshVolume.GetMaxCoordinates());
        binaryWriter << (meshVolume.GetWidth());
        binaryWriter << (meshVolume.GetLength());
        binaryWriter << (meshVolume.GetHeight());
            
        uint32 nBlocks = meshVolume.GetWidth() * meshVolume.GetLength() * meshVolume.GetHeight();
        binaryWriter.WriteBytes(meshVolume.GetData(), sizeof(BlockVolumeBlockType) * nBlocks);
    }

    binaryWriter.WriteUInt32(m_collisionShapeType);
    if (m_collisionShapeType != Physics::COLLISION_SHAPE_TYPE_NONE && m_collisionShapeType != Physics::COLLISION_SHAPE_TYPE_BLOCK_MESH)
    {
        // build triangle mesh
        BlockMeshBuilder builder;
        builder.SetFromVolume(&m_pLODLevels[0]);
        builder.SetAmbientOcclusionEnabled(false);
        builder.GenerateCollisionMesh();

        // get builder verts/tris
        const BlockMeshBuilder::VertexArray &builderVertices = builder.GetOutputVertices();
        const BlockMeshBuilder::TriangleArray &builderTriangles = builder.GetOutputTriangles();

        // send to collision shape generator
        Physics::CollisionShapeGenerator collisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH);
        for (uint32 i = 0; i < builderTriangles.GetSize(); i++)
        {
            const uint32 *indices = builderTriangles[i].Indices;
            collisionShapeGenerator.AddTriangle(builderVertices[indices[0]].Position,
                                                builderVertices[indices[1]].Position,
                                                builderVertices[indices[2]].Position);
        }

        // convert to the appropriate type
        if (m_collisionShapeType == Physics::COLLISION_SHAPE_TYPE_BOX)
        {
            collisionShapeGenerator.ConvertToBox();
        }
        else if (m_collisionShapeType == Physics::COLLISION_SHAPE_TYPE_SPHERE)
        {
            collisionShapeGenerator.ConvertToSphere();
        }
        else if (m_collisionShapeType == Physics::COLLISION_SHAPE_TYPE_CONVEX_HULL)
        {
            collisionShapeGenerator.ConvertToConvexHull();
        }
        else if (m_collisionShapeType != Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH)
        {
            Log_ErrorPrintf("BlockMeshGenerator::Compile: Unhandled collision shape type '%s'", NameTable_GetNameString(Physics::NameTables::CollisionShapeType, m_collisionShapeType));
            return false;
        }

        // write it out
        BinaryBlob *pBlob;
        if (!collisionShapeGenerator.Compile(&pBlob))
        {
            Log_ErrorPrintf("BlockMeshGenerator::Compile: Collision shape compilation failed.");
            return false;
        }

        // copy to stream
        binaryWriter.WriteUInt32(pBlob->GetDataSize());
        binaryWriter.WriteBytes(pBlob->GetDataPointer(), pBlob->GetDataSize());

        pBlob->Release();
    }

    return !binaryWriter.InErrorState();
}

// Interface
BinaryBlob *ResourceCompiler::CompileBlockMesh(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.blm.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileBlockMesh: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    BlockMeshGenerator *pGenerator = new BlockMeshGenerator();
    if (!pGenerator->LoadFromXML(sourceFileName, pStream))
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
