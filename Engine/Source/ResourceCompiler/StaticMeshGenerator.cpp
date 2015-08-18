#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "ResourceCompiler/CollisionShapeGenerator.h"
#include "Engine/DataFormats.h"
#include "Core/ChunkFileReader.h"
#include "Core/ChunkFileWriter.h"
#include "Core/MeshUtilties.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(StaticMeshGenerator);

StaticMeshGenerator::StaticMeshGenerator()
    : m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_pCollisionShapeGenerator(nullptr)
{

}

StaticMeshGenerator::~StaticMeshGenerator()
{
    delete m_pCollisionShapeGenerator;

    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        LOD *lod = m_lods[lodIndex];
        for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
            delete lod->Batches[batchIndex];

        delete lod;
    }
}

bool StaticMeshGenerator::GetVertexTextureCoordinatesEnabled() const
{
    return m_properties.GetPropertyValueDefaultBool("EnableVertexTextureCoordinates", false);
}

void StaticMeshGenerator::SetVertexTextureCoordinatesEnabled(bool enabled)
{
    m_properties.SetPropertyValueBool("EnableVertexTextureCoordinates", enabled);
}

bool StaticMeshGenerator::GetVertexColorsEnabled() const
{
    return m_properties.GetPropertyValueDefaultBool("EnableVertexColors", false);
}

void StaticMeshGenerator::SetVertexColorsEnabled(bool enabled)
{
    m_properties.SetPropertyValueBool("EnableVertexColors", enabled);
}

uint32 StaticMeshGenerator::GetVertexTextureCoordinateComponentCount() const
{
    return m_properties.GetPropertyValueDefaultUInt32("VertexTextureCoordinateComponents", 2);
}

void StaticMeshGenerator::SetVertexTextureCoordinateComponentCount(uint32 components)
{
    m_properties.SetPropertyValueUInt32("VertexTextureCoordinateComponents", components);
}

bool StaticMeshGenerator::Create(bool enableVertexTextureCoordinates /* = true */, bool enableVertexColors /* = false */, uint32 textureCoordinateComponents /* = 2 */)
{
    DebugAssert(m_pCollisionShapeGenerator == nullptr);
    DebugAssert(m_lods.GetSize() == 0);

    // set initial properties
    SetVertexTextureCoordinatesEnabled(enableVertexTextureCoordinates);
    SetVertexColorsEnabled(enableVertexColors);
    SetVertexTextureCoordinateComponentCount(textureCoordinateComponents);

    // set bounds to zero
    m_boundingBox = AABox::Zero;
    m_boundingSphere = Sphere::Zero;
    return true;
}

bool StaticMeshGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
        return false;

    if (!xmlReader.SkipToElement("static-mesh"))
    {
        xmlReader.PrintError("could not skip to static-mesh element.");
        return false;
    }

    // process xml nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 staticMeshSelection = xmlReader.Select("properties|bounds|collision-shape|lods");
            if (staticMeshSelection < 0)
                return false;

            switch (staticMeshSelection)
            {
                // properties
            case 0:
                {
                    if (!xmlReader.IsEmptyElement() && !m_properties.LoadFromXML(xmlReader))
                        return false;
                }
                break;

                // bounds
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
                                int32 boundsSelection = xmlReader.Select("bounding-box|bounding-sphere");
                                if (boundsSelection < 0)
                                    return false;

                                switch (boundsSelection)
                                {
                                    // bounding-box
                                case 0:
                                    {
                                        float3 minBounds = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("min-bounds", "0 0 0"));
                                        float3 maxBounds = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("max-bounds", "0 0 0"));
                                        m_boundingBox.SetBounds(minBounds, maxBounds);
                                        
                                        if (!xmlReader.SkipCurrentElement())
                                            return false;
                                    }
                                    break;

                                    // bounding-sphere
                                case 1:
                                    {
                                        float3 sphereCenter = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("center", "0 0 0"));
                                        float sphereRadius = StringConverter::StringToFloat(xmlReader.FetchAttributeDefault("radius", "0"));
                                        m_boundingSphere.SetCenter(sphereCenter);
                                        m_boundingSphere.SetRadius(sphereRadius);

                                        if (!xmlReader.SkipCurrentElement())
                                            return false;
                                    }
                                    break;
                                }
                            }
                            else
                            {
                                if (!xmlReader.ExpectEndOfElementName("bounds"))
                                    return false;

                                break;
                            }
                        }
                    }
                }
                break;

                // collision-shape
            case 2:
                {
                    if (m_pCollisionShapeGenerator != nullptr)
                    {
                        xmlReader.PrintError("collision shape already defined");
                        return false;
                    }

                    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator();
                    if (!m_pCollisionShapeGenerator->LoadFromXML(xmlReader))
                        return false;
                }
                break;

                // lods
            case 3:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                return false;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 lodsSelection = xmlReader.Select("lod");
                                if (lodsSelection < 0)
                                    return false;

                                switch (lodsSelection)
                                {
                                    // lod
                                case 0:
                                    {
                                        LOD *lod = new LOD();
                                        m_lods.Add(lod);

                                        if (!xmlReader.IsEmptyElement())
                                        {
                                            for (;;)
                                            {
                                                if (!xmlReader.NextToken())
                                                    return false;

                                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                {
                                                    int32 lodSelection = xmlReader.Select("vertices|batches");
                                                    if (lodSelection < 0)
                                                        return false;

                                                    switch (lodSelection)
                                                    {
                                                        // vertices
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
                                                                        int32 verticesSelection = xmlReader.Select("vertex");
                                                                        if (verticesSelection < 0)
                                                                            return false;

                                                                        // init vertex
                                                                        Vertex vertex;
                                                                        vertex.Position = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("position", "0 0 0"));
                                                                        vertex.TexCoord = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("texcoord", "0 0 0"));
                                                                        vertex.Tangent = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("tangent", "1 0 0"));
                                                                        vertex.Binormal = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("binormal", "0 1 0"));
                                                                        vertex.Normal = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("normal", "0 0 1"));
                                                                        vertex.Color = StringConverter::StringToColor(xmlReader.FetchAttributeDefault("color", "#00000000"));
                                                                        if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                                                            return false;

                                                                        // store it
                                                                        lod->Vertices.Add(vertex);
                                                                    }
                                                                    else
                                                                    {
                                                                        if (!xmlReader.ExpectEndOfElementName("vertices"))
                                                                            return false;

                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        break;

                                                        // batches
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
                                                                        int32 batchesSelection = xmlReader.Select("batch");
                                                                        if (batchesSelection < 0)
                                                                            return false;

                                                                        switch (batchesSelection)
                                                                        {
                                                                            // batch
                                                                        case 0:
                                                                            {
                                                                                const char *materialNameStr = xmlReader.FetchAttribute("material");
                                                                                if (materialNameStr == nullptr)
                                                                                {
                                                                                    xmlReader.PrintError("missing material name for batch");
                                                                                    return false;
                                                                                }

                                                                                Batch *batch = new Batch();
                                                                                batch->MaterialName = materialNameStr;
                                                                                lod->Batches.Add(batch);

                                                                                if (!xmlReader.IsEmptyElement())
                                                                                {
                                                                                    for (;;)
                                                                                    {
                                                                                        if (!xmlReader.NextToken())
                                                                                            return false;

                                                                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                                                        {
                                                                                            int32 batchSelection = xmlReader.Select("triangles");
                                                                                            if (batchSelection < 0)
                                                                                                return false;

                                                                                            switch (batchSelection)
                                                                                            {
                                                                                                // triangles
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
                                                                                                                int32 trianglesSelection = xmlReader.Select("triangle");
                                                                                                                if (trianglesSelection < 0)
                                                                                                                    return false;

                                                                                                                Triangle triangle;
                                                                                                                triangle.Indices[0] = StringConverter::StringToUInt32(xmlReader.FetchAttributeDefault("vertex1", "0"));
                                                                                                                triangle.Indices[1] = StringConverter::StringToUInt32(xmlReader.FetchAttributeDefault("vertex2", "0"));
                                                                                                                triangle.Indices[2] = StringConverter::StringToUInt32(xmlReader.FetchAttributeDefault("vertex3", "0"));

                                                                                                                // validate indices
                                                                                                                if (triangle.Indices[0] >= lod->Vertices.GetSize() ||
                                                                                                                    triangle.Indices[1] >= lod->Vertices.GetSize() ||
                                                                                                                    triangle.Indices[2] >= lod->Vertices.GetSize())
                                                                                                                {
                                                                                                                    xmlReader.PrintError("triangle contains out of range vertex indices");
                                                                                                                    return false;
                                                                                                                }

                                                                                                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                                                                                                    return false;

                                                                                                                // store it
                                                                                                                batch->Triangles.Add(triangle);
                                                                                                            }
                                                                                                            else
                                                                                                            {
                                                                                                                if (!xmlReader.ExpectEndOfElementName("triangles"))
                                                                                                                    return false;

                                                                                                                break;
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                                break;
                                                                                            }
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            if (!xmlReader.ExpectEndOfElementName("batch"))
                                                                                                return false;

                                                                                            break;
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                            break;
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        if (!xmlReader.ExpectEndOfElementName("batches"))
                                                                            return false;

                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        break;
                                                    }
                                                }
                                                else
                                                {
                                                    if (!xmlReader.ExpectEndOfElementName("lod"))
                                                        return false;

                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            else
                            {
                                if (!xmlReader.ExpectEndOfElementName("lods"))
                                    return false;

                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        else
        {
            if (!xmlReader.ExpectEndOfElementName("static-mesh"))
                return false;

            break;
        }
    }

    return true;
}

bool StaticMeshGenerator::IsCompleteMesh() const
{
    if (m_lods.GetSize() == 0)
        return false;

    for (uint32 i = 0; i < m_lods.GetSize(); i++)
    {
        if (m_lods[i]->Batches.GetSize() == 0)
            return false;

        for (uint32 j = 0; j < m_lods[i]->Batches.GetSize(); j++)
        {
            if (m_lods[i]->Batches[j]->Triangles.GetSize() == 0)
                return false;
        }
    }

    return true;
}

bool StaticMeshGenerator::SaveToXML(ByteStream *pStream) const
{
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    SmallString tempString;
    xmlWriter.StartElement("static-mesh");

    xmlWriter.StartElement("properties");
    m_properties.SaveToXML(xmlWriter);
    xmlWriter.EndElement();     // </properties>

    // write bounds
    xmlWriter.StartElement("bounds");
    {
        // bounding box
        xmlWriter.StartElement("bounding-box");
        {
            StringConverter::Float3ToString(tempString, m_boundingBox.GetMinBounds());
            xmlWriter.WriteAttribute("min-bounds", tempString);
            StringConverter::Float3ToString(tempString, m_boundingBox.GetMaxBounds());
            xmlWriter.WriteAttribute("max-bounds", tempString);
        }
        xmlWriter.EndElement();

        // bounding sphere
        xmlWriter.StartElement("bounding-sphere");
        {
            StringConverter::Float3ToString(tempString, m_boundingSphere.GetCenter());
            xmlWriter.WriteAttribute("center", tempString);
            StringConverter::FloatToString(tempString, m_boundingSphere.GetRadius());
            xmlWriter.WriteAttribute("radius", tempString);
        }
        xmlWriter.EndElement();
    }
    xmlWriter.EndElement();

    // write collision shape
    if (m_pCollisionShapeGenerator != NULL)
    {
        xmlWriter.StartElement("collision-shape");
        if (!m_pCollisionShapeGenerator->SaveToXML(xmlWriter))
            return false;

        xmlWriter.EndElement();
    }

    // write lods
    xmlWriter.StartElement("lods");
    {
        for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
        {
            const LOD *lod = m_lods[lodIndex];

            xmlWriter.StartElement("lod");
            {
                xmlWriter.StartElement("vertices");
                {
                    for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
                    {
                        const Vertex &vertex = lod->Vertices[vertexIndex];

                        xmlWriter.StartElement("vertex");
                        {
                            StringConverter::Float3ToString(tempString, vertex.Position);
                            xmlWriter.WriteAttribute("position", tempString);

                            StringConverter::Float3ToString(tempString, vertex.TexCoord);
                            xmlWriter.WriteAttribute("texcoord", tempString);

                            StringConverter::Float3ToString(tempString, vertex.Tangent);
                            xmlWriter.WriteAttribute("tangent", tempString);

                            StringConverter::Float3ToString(tempString, vertex.Binormal);
                            xmlWriter.WriteAttribute("binormal", tempString);

                            StringConverter::Float3ToString(tempString, vertex.Normal);
                            xmlWriter.WriteAttribute("normal", tempString);

                            StringConverter::ColorToString(tempString, vertex.Color);
                            xmlWriter.WriteAttribute("color", tempString);
                        }
                        xmlWriter.EndElement(); // </vertex>
                    }
                }
                xmlWriter.EndElement();     // </vertices>

                xmlWriter.StartElement("batches");
                {
                    for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
                    {
                        const Batch *batch = lod->Batches[batchIndex];

                        xmlWriter.StartElement("batch");
                        {
                            xmlWriter.WriteAttribute("material", batch->MaterialName);
                            xmlWriter.StartElement("triangles");
                            {
                                for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
                                {
                                    const Triangle &triangle = batch->Triangles[triangleIndex];
                                    xmlWriter.StartElement("triangle");
                                    {
                                        StringConverter::UInt32ToString(tempString, triangle.Indices[0]);
                                        xmlWriter.WriteAttribute("vertex1", tempString);

                                        StringConverter::UInt32ToString(tempString, triangle.Indices[1]);
                                        xmlWriter.WriteAttribute("vertex2", tempString);

                                        StringConverter::UInt32ToString(tempString, triangle.Indices[2]);
                                        xmlWriter.WriteAttribute("vertex3", tempString);
                                    }
                                    xmlWriter.EndElement();     // </triangle>
                                }
                            }
                            xmlWriter.EndElement();     // </triangles>
                        }
                        xmlWriter.EndElement();     // </batch>
                    }
                }
                xmlWriter.EndElement();     // </batches>
            }
            xmlWriter.EndElement();     // </lod>
        }
    }
    xmlWriter.EndElement();     // </lods>
    
    // end static mesh
    xmlWriter.EndElement();

    // test state
    bool result = (!xmlWriter.InErrorState());
    xmlWriter.Close();
    return result;
}

bool StaticMeshGenerator::Compile(ByteStream *pStream) const
{
    // valid mesh?
    if (!IsCompleteMesh())
        return false;

    // calculate size of a vertex
    uint32 vertexFlags = 0;
    if (GetVertexTextureCoordinatesEnabled())
    {
        if (GetVertexTextureCoordinateComponentCount() == 3)
            vertexFlags |= DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT3;
        else
            vertexFlags |= DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT2;
    }

    if (GetVertexColorsEnabled())
        vertexFlags |= DF_STATICMESH_VERTEX_FLAG_COLOR;

    // compile a list of materials
    PODArray<const String *> uniqueMaterials;
    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        const LOD *lod = m_lods[lodIndex];
        for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
        {
            const String &material = lod->Batches[batchIndex]->MaterialName;

            uint32 materialIndex;
            for (materialIndex = 0; materialIndex < uniqueMaterials.GetSize(); materialIndex++)
            {
                if (uniqueMaterials[materialIndex]->CompareInsensitive(material))
                    break;
            }

            if (materialIndex == uniqueMaterials.GetSize())
                uniqueMaterials.Add(&material);
        }
    }
    if (uniqueMaterials.GetSize() == 0)
        return false;

    // create writer
    BinaryWriter binaryWriter(pStream);

    // fill header
    DF_STATICMESH_HEADER meshHeader;
    meshHeader.Magic = DF_STATICMESH_HEADER_MAGIC;
    meshHeader.Size = sizeof(DF_STATICMESH_HEADER);
    meshHeader.CreationTime = static_cast<uint64>(Timestamp::Now().AsUnixTimestamp());
    meshHeader.BoundingBoxMin = m_boundingBox.GetMinBounds();
    meshHeader.BoundingBoxMax = m_boundingBox.GetMaxBounds();
    meshHeader.BoundingSphereCenter = m_boundingSphere.GetCenter();
    meshHeader.BoundingSphereRadius = m_boundingSphere.GetRadius();
    meshHeader.CollisionShapeType = (m_pCollisionShapeGenerator != NULL) ? (uint32)m_pCollisionShapeGenerator->GetType() : (uint32)Physics::COLLISION_SHAPE_TYPE_NONE;
    meshHeader.CollisionShapeSize = 0;
    meshHeader.CollisionShapeOffset = 0;
    meshHeader.VertexFlags = vertexFlags;
    meshHeader.MaterialCount = 0;
    meshHeader.MaterialNamesOffset = 0;
    meshHeader.LODCount = 0;
    meshHeader.LODOffsetsOffset = 0;
    
    // write header
    uint64 headerOffset = pStream->GetPosition();
    if (!pStream->Write2(&meshHeader, sizeof(meshHeader)))
        return false;

    // write collision shape
    if (m_pCollisionShapeGenerator != nullptr)
    {
        BinaryBlob *pCollisionDataBlob;
        if (!m_pCollisionShapeGenerator->Compile(&pCollisionDataBlob))
            return false;

        meshHeader.CollisionShapeOffset = (uint32)(pStream->GetPosition() - headerOffset);
        meshHeader.CollisionShapeSize = pCollisionDataBlob->GetDataSize();
        if (!pStream->Write2(pCollisionDataBlob->GetDataPointer(), meshHeader.CollisionShapeSize))
        {
            pCollisionDataBlob->Release();
            return false;
        }

        pCollisionDataBlob->Release();
    }

    // write materials
    {
        meshHeader.MaterialCount = uniqueMaterials.GetSize();
        meshHeader.MaterialNamesOffset = (uint32)(pStream->GetPosition() - headerOffset);

        for (uint32 materialIndex = 0; materialIndex < uniqueMaterials.GetSize(); materialIndex++)
        {
            //if (!binaryWriter.WriteCString(*uniqueMaterials[materialIndex]))
                //return false;

            binaryWriter.WriteCString(*uniqueMaterials[materialIndex]);
        }
    }

    // write lods
    {
        meshHeader.LODCount = m_lods.GetSize();
        meshHeader.LODOffsetsOffset = (uint32)(pStream->GetPosition() - headerOffset);

        // write an empty offset for each lod, which gets filled in later
        uint64 lodOffsetsOffset = pStream->GetPosition();
        uint32 *lodOffsets = (uint32 *)alloca(sizeof(uint32) * m_lods.GetSize());
        Y_memzero(lodOffsets, sizeof(uint32) * m_lods.GetSize());
        if (!pStream->Write2(lodOffsets, sizeof(uint32) * m_lods.GetSize()))
            return false;

        // write each lod
        for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
        {
            const LOD *lod = m_lods[lodIndex];
            
            // calculate offset to lod
            uint64 lodHeaderOffset = pStream->GetPosition();
            lodOffsets[lodIndex] = (uint32)(lodHeaderOffset - headerOffset);

            // write lod header
            DF_STATICMESH_LOD_HEADER lodHeader;
            lodHeader.VertexCount = 0;
            lodHeader.VerticesOffset = 0;
            lodHeader.IndexCount = 0;
            lodHeader.IndexFormat = 0;
            lodHeader.IndicesOffset = 0;
            lodHeader.BatchCount = 0;
            lodHeader.BatchesOffset = 0;
            if (!pStream->Write2(&lodHeader, sizeof(lodHeader)))
                return false;

            // write vertices
            {
                lodHeader.VertexCount = lod->Vertices.GetSize();
                lodHeader.VerticesOffset = (uint32)(pStream->GetPosition() - headerOffset);

                for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
                {
                    const Vertex &sourceVertex = lod->Vertices[vertexIndex];
                    DF_STATICMESH_VERTEX fileVertex;
                    sourceVertex.Position.Store(fileVertex.Position);
                    sourceVertex.Tangent.Store(fileVertex.Tangent);
                    sourceVertex.Binormal.Store(fileVertex.Binormal);
                    sourceVertex.Normal.Store(fileVertex.Normal);
                    sourceVertex.TexCoord.Store(fileVertex.TexCoord);
                    fileVertex.Color = sourceVertex.Color;

                    if (!pStream->Write2(&fileVertex, sizeof(fileVertex)))
                        return false;
                }
            }

            // write indices
            // assume that batches are stored sequentially after one another
            {
                // find total number of indices
                uint32 totalIndices = 0;
                for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
                    totalIndices += lod->Batches[batchIndex]->Triangles.GetSize() * 3;

                // should have indices...
                DebugAssert(totalIndices > 0);

                // can use 16-bit indices?
                if (lod->Vertices.GetSize() <= 0xFFFF)
                {
                    uint16 *pOutIndices = new uint16[totalIndices];
                    uint16 *pOutIndexPointer = pOutIndices;

                    // add indices
                    for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
                    {
                        const Batch *batch = lod->Batches[batchIndex];
                        for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
                        {
                            const Triangle *pTriangle = &batch->Triangles[triangleIndex];
                            *(pOutIndexPointer++) = (uint16)pTriangle->Indices[0];
                            *(pOutIndexPointer++) = (uint16)pTriangle->Indices[1];
                            *(pOutIndexPointer++) = (uint16)pTriangle->Indices[2];
                        }
                    }

                    // write indices
                    lodHeader.IndexCount = totalIndices;
                    lodHeader.IndexFormat = GPU_INDEX_FORMAT_UINT16;
                    lodHeader.IndicesOffset = (uint32)(pStream->GetPosition() - headerOffset);
                    if (!pStream->Write2(pOutIndices, sizeof(uint16) * totalIndices))
                    {
                        delete[] pOutIndices;
                        return false;
                    }

                    delete[] pOutIndices;
                }
                else
                {
                    uint32 *pOutIndices = new uint32[totalIndices];
                    uint32 *pOutIndexPointer = pOutIndices;

                    // add indices
                    for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
                    {
                        const Batch *batch = lod->Batches[batchIndex];
                        for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
                        {
                            const Triangle *pTriangle = &batch->Triangles[triangleIndex];
                            *(pOutIndexPointer++) = pTriangle->Indices[0];
                            *(pOutIndexPointer++) = pTriangle->Indices[1];
                            *(pOutIndexPointer++) = pTriangle->Indices[2];
                        }
                    }

                    // write indices
                    lodHeader.IndexCount = totalIndices;
                    lodHeader.IndexFormat = GPU_INDEX_FORMAT_UINT32;
                    lodHeader.IndicesOffset = (uint32)(pStream->GetPosition() - headerOffset);
                    if (!pStream->Write2(pOutIndices, sizeof(uint32) * totalIndices))
                    {
                        delete[] pOutIndices;
                        return false;
                    }

                    delete[] pOutIndices;
                }
            }

            // write batches
            // the triangle starting index is easy to calculate now
            {
                uint32 startingIndex = 0;

                lodHeader.BatchCount = lod->Batches.GetSize();
                lodHeader.BatchesOffset = (uint32)(pStream->GetPosition() - headerOffset);

                for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
                {
                    const Batch *batch = lod->Batches[batchIndex];
                    uint32 batchIndices = batch->Triangles.GetSize() * 3;

                    // find the material index
                    uint32 batchMaterial = 0;
                    for (uint32 materialIndex = 0; materialIndex < uniqueMaterials.GetSize(); materialIndex++)
                    {
                        if (uniqueMaterials[materialIndex]->Compare(batch->MaterialName))
                        {
                            batchMaterial = materialIndex;
                            break;
                        }
                    }

                    // write data
                    DF_STATICMESH_BATCH fileBatch;
                    fileBatch.MaterialIndex = batchMaterial;
                    fileBatch.StartIndex = startingIndex;
                    fileBatch.NumIndices = batchIndices;
                    if (!pStream->Write2(&fileBatch, sizeof(fileBatch)))
                        return false;

                    // increment starting triangle
                    startingIndex += fileBatch.NumIndices;
                }
            }

            // rewrite the lod header
            uint64 seekOffset = pStream->GetPosition();
            if (!pStream->SeekAbsolute(lodHeaderOffset) || !pStream->Write2(&lodHeader, sizeof(lodHeader)) || !pStream->SeekAbsolute(seekOffset))
                return false;
        }

        // rewrite lod offset table
        uint64 seekOffset = pStream->GetPosition();
        if (!pStream->SeekAbsolute(lodOffsetsOffset) || !pStream->Write2(lodOffsets, sizeof(uint32) * m_lods.GetSize()) || !pStream->SeekAbsolute(seekOffset))
            return false;
    }

    // rewrite header
    uint64 seekOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(headerOffset) || !pStream->Write2(&meshHeader, sizeof(meshHeader)) || !pStream->SeekAbsolute(seekOffset))
        return false;

    // done
    return (!pStream->InErrorState());
}

void StaticMeshGenerator::Copy(const StaticMeshGenerator *pGenerator)
{
    // trash everything first
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = nullptr;
    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        LOD *lod = m_lods[lodIndex];
        for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
            delete lod->Batches[batchIndex];

        delete lod;
    }
    m_lods.Obliterate();
    m_properties.Clear();

    // copy everything in
    m_boundingBox = pGenerator->m_boundingBox;
    m_boundingSphere = pGenerator->m_boundingSphere;
    m_properties.CopyProperties(&pGenerator->m_properties);

    // copy the collision shape
    if (pGenerator->m_pCollisionShapeGenerator != nullptr)
    {
        m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator();
        m_pCollisionShapeGenerator->Copy(pGenerator->m_pCollisionShapeGenerator);
    }

    // copy lods
    for (uint32 lodIndex = 0; lodIndex < pGenerator->m_lods.GetSize(); lodIndex++)
    {
        const LOD *srcLOD = pGenerator->m_lods[lodIndex];

        LOD *destLOD = new LOD;
        destLOD->Vertices.Assign(srcLOD->Vertices);

        for (uint32 batchIndex = 0; batchIndex < srcLOD->Batches.GetSize(); batchIndex++)
        {
            const Batch *srcBatch = srcLOD->Batches[batchIndex];

            Batch *destBatch = new Batch;
            destBatch->MaterialName = srcBatch->MaterialName;
            destBatch->Triangles.Assign(srcBatch->Triangles);
            destLOD->Batches.Add(destBatch);
        }

        m_lods.Add(destLOD);
    }
}

uint32 StaticMeshGenerator::AddLOD()
{
    LOD *lod = new LOD;
    m_lods.Add(lod);
    return m_lods.GetSize() - 1;
}

uint32 StaticMeshGenerator::AddVertex(uint32 lod, const float3 &position, const float3 &tangent /* = float3::UnitX */, const float3 &binormal /* = float3::UnitY */, const float3 &normal /* = float3::UnitZ */, const float3 &texCoord /* = float3::Zero */, const uint32 color /* = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255)*/)
{
    Vertex v;
    v.Position = position;
    v.Tangent = tangent;
    v.Binormal = binormal;
    v.Normal = normal;
    v.TexCoord = texCoord;
    v.Color = color;

    DebugAssert(lod < m_lods.GetSize());
    m_lods[lod]->Vertices.Add(v);
    return m_lods[lod]->Vertices.GetSize() - 1;
}

uint32 StaticMeshGenerator::AddBatch(uint32 lod, const char *materialName)
{
    Batch *batch = new Batch;
    batch->MaterialName = materialName;
    
    DebugAssert(lod < m_lods.GetSize());
    m_lods[lod]->Batches.Add(batch);
    return m_lods[lod]->Batches.GetSize() - 1;
}

uint32 StaticMeshGenerator::AddTriangle(uint32 lod, uint32 batchIndex, const uint32 i0, const uint32 i1, const uint32 i2)
{
    Triangle t;
    t.Indices[0] = i0;
    t.Indices[1] = i1;
    t.Indices[2] = i2;

    DebugAssert(lod < m_lods.GetSize() && batchIndex < m_lods[lod]->Batches.GetSize());
    m_lods[lod]->Batches[batchIndex]->Triangles.Add(t);
    return m_lods[lod]->Batches[batchIndex]->Triangles.GetSize() - 1;
}

void StaticMeshGenerator::CalculateBounds()
{
    // get bounding box
    {
        bool first = true;
        AABox boundingBox(AABox::Zero);

        for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
        {
            const LOD *lod = m_lods[lodIndex];
            for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
            {
                if (first)
                {
                    boundingBox.SetBounds(lod->Vertices[vertexIndex].Position);
                    first = false;
                }
                else
                {
                    boundingBox.Merge(lod->Vertices[vertexIndex].Position);
                }
            }
        }

        m_boundingBox = boundingBox;
    }

    // get bounding sphere
    {
        // set the sphere center to the box center
        m_boundingSphere.SetCenter(m_boundingBox.GetCenter());
        m_boundingSphere.SetRadius(0.0f);

        // insert points
        for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
        {
            const LOD *lod = m_lods[lodIndex];
            for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
                m_boundingSphere.Merge(lod->Vertices[vertexIndex].Position);
        }
    }
}

void StaticMeshGenerator::GenerateTangents(uint32 LODIndex)
{
    DebugAssert(LODIndex < m_lods.GetSize());
    LOD *lod = m_lods[LODIndex];

    // zero all tangents
    for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
    {
        Vertex *vertex = &lod->Vertices[vertexIndex];
        vertex->Tangent.SetZero();
        vertex->Binormal.SetZero();
        vertex->Normal.SetZero();
    }

    // calculate tangents for each vertex
    for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
    {
        const Batch *batch = lod->Batches[batchIndex];

        for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
        {
            const Triangle *triangle = &batch->Triangles[triangleIndex];

            Vertex *v0 = &lod->Vertices[triangle->Indices[0]];
            Vertex *v1 = &lod->Vertices[triangle->Indices[1]];
            Vertex *v2 = &lod->Vertices[triangle->Indices[2]];

            // use texcoords to generate tangent vectors
            float3 tx, ty, tz;
            MeshUtilites::CalculateTangentSpaceVectors(v0->Position, v1->Position, v2->Position, v0->TexCoord.xy(), v1->TexCoord.xy(), v2->TexCoord.xy(), tx, ty, tz);

            // add to vertices
            v0->Tangent += tx;
            v0->Binormal += ty;
            v0->Normal += tz;
            v1->Tangent += tx;
            v1->Binormal += ty;
            v1->Normal += tz;
            v2->Tangent += tx;
            v2->Binormal += ty;
            v2->Normal += tz;
        }
    }
   
    // normalize each tangent
    for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
    {
        Vertex *vertex = &lod->Vertices[vertexIndex];
        if (vertex->Tangent.SquaredLength() > 0.0f)
            vertex->Tangent.NormalizeInPlace();
        if (vertex->Binormal.SquaredLength() > 0.0f)
            vertex->Binormal.NormalizeInPlace();
        if (vertex->Normal.SquaredLength() > 0.0f)
            vertex->Normal.NormalizeInPlace();
    }
}

void StaticMeshGenerator::JoinBatches()
{
    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        LOD *lod = m_lods[lodIndex];

        for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); )
        {
            Batch *batch = lod->Batches[batchIndex];
            bool wasMerged = false;

            for (uint32 mergeBatchIndex = batchIndex + 1; mergeBatchIndex < lod->Batches.GetSize(); mergeBatchIndex++)
            {
                Batch *mergeBatch = lod->Batches[mergeBatchIndex];
                if (mergeBatch->MaterialName == batch->MaterialName)
                {
                    DebugAssert(mergeBatch != batch);

                    Log_DevPrintf("StaticMeshGenerator::CollapseBatches: Merge LOD %u batch %u -> %u", lodIndex, mergeBatchIndex, batchIndex);
                    
                    wasMerged = true;
                    batch->Triangles.AddArray(mergeBatch->Triangles);
                    lod->Batches.OrderedRemove(mergeBatchIndex);
                    delete mergeBatch;
                    break;
                }
            }

            if (!wasMerged)
                batchIndex++;
        }
    }
}

void StaticMeshGenerator::CenterMesh(CenterOrigin origin /* = CenterOrigin_Center */, float3 *pOffset /* = nullptr */)
{
    // ensure bounding box is up to date
    CalculateBounds();

    // some useful vars for the next part
    float3 boundingBoxMin(m_boundingBox.GetMinBounds());
    float3 boundingBoxCenter(m_boundingBox.GetCenter());
    float3 boundingBoxHalfExtents(m_boundingBox.GetExtents() * 0.5f);

    // work out the offset
    float3 moveOffset(float3::Zero);
    switch (origin)
    {
    case CenterOrigin_Center:
        moveOffset = -boundingBoxCenter;
        break;

    case CenterOrigin_CenterBottom:
        moveOffset = float3(-boundingBoxCenter.x, -boundingBoxCenter.y, -boundingBoxMin.z);
        break;
    }

    // apply the offset
    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        LOD *lod = m_lods[lodIndex];
        for (uint32 vertexIndex = 0; vertexIndex < lod->Vertices.GetSize(); vertexIndex++)
            lod->Vertices[vertexIndex].Position += moveOffset;
    }

    // recalculate bounds
    CalculateBounds();

    // update offset
    if (pOffset != nullptr)
        *pOffset = moveOffset;
}

bool StaticMeshGenerator::BuildCollisionShape(CollisionShapeType type)
{
    switch (type)
    {
    case CollisionShapeType_None:
        RemoveCollisionShape();
        return true;

    case CollisionShapeType_Box:
        return BuildBoxCollisionShape();

    case CollisionShapeType_Sphere:
        return BuildSphereCollisionShape();

    case CollisionShapeType_TriangleMesh:
        return BuildTriangleMeshCollisionShape(0);

    case CollisionShapeType_ConvexHull:
        return BuildConvexHullCollisionShape(0);
    }

    return false;
}

void StaticMeshGenerator::InternalBuildTriangleMeshCollisionShape(uint32 buildFromLOD)
{
    DebugAssert(buildFromLOD < m_lods.GetSize());

    // nuke the old one
    delete m_pCollisionShapeGenerator;

    // build new
    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH);

    // use lod
    const LOD *lod = m_lods[buildFromLOD];
    for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
    {
        const Batch *batch = lod->Batches[batchIndex];

        for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
        {
            const Triangle *triangle = &batch->Triangles[triangleIndex];
            m_pCollisionShapeGenerator->AddTriangle(lod->Vertices[triangle->Indices[0]].Position,
                                                    lod->Vertices[triangle->Indices[1]].Position,
                                                    lod->Vertices[triangle->Indices[2]].Position);
        }
    }
}

bool StaticMeshGenerator::BuildBoxCollisionShape()
{
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_BOX);
    m_pCollisionShapeGenerator->SetBoxCenter(m_boundingBox.GetCenter());
    m_pCollisionShapeGenerator->SetBoxHalfExtents(m_boundingBox.GetExtents() / 2.0f);
    return true;
}

bool StaticMeshGenerator::BuildSphereCollisionShape()
{
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_SPHERE);
    m_pCollisionShapeGenerator->SetSphereCenter(m_boundingSphere.GetCenter());
    m_pCollisionShapeGenerator->SetSphereRadius(m_boundingSphere.GetRadius());
    return true;
}

bool StaticMeshGenerator::BuildTriangleMeshCollisionShape(uint32 buildFromLOD /* = 0 */)
{
    InternalBuildTriangleMeshCollisionShape(buildFromLOD);
    return true;
}

bool StaticMeshGenerator::BuildConvexHullCollisionShape(uint32 buildFromLOD /* = 0 */)
{
    //InternalBuildTriangleMeshCollisionShape(buildFromLOD);
    //m_pCollisionShapeGenerator->ConvertToConvexHull();
    return false;
}

void StaticMeshGenerator::SetCollisionShape(Physics::CollisionShapeGenerator *pGenerator)
{
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = pGenerator;
}

void StaticMeshGenerator::RemoveCollisionShape()
{
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = NULL;
}

void StaticMeshGenerator::FlipTriangleWinding()
{
    for (uint32 lodIndex = 0; lodIndex < m_lods.GetSize(); lodIndex++)
    {
        LOD *lod = m_lods[lodIndex];
        for (uint32 batchIndex = 0; batchIndex < lod->Batches.GetSize(); batchIndex++)
        {
            Batch *batch = lod->Batches[batchIndex];

            for (uint32 triangleIndex = 0; triangleIndex < batch->Triangles.GetSize(); triangleIndex++)
            {
                Triangle *triangle = &batch->Triangles[triangleIndex];
                Swap(triangle->Indices[1], triangle->Indices[2]);
            }
        }
    }
}

// StaticMeshGenerator &StaticMeshGenerator::operator=(const StaticMeshGenerator &copyFrom)
// {
//     m_boundingBox = copyFrom.m_boundingBox;
//     m_bVerticesHaveColors = copyFrom.m_bVerticesHaveColors;
//     m_bVerticesHaveTexCoords = copyFrom.m_bVerticesHaveTexCoords;
// 
//     m_Materials = copyFrom.m_Materials;
//     m_Vertices = copyFrom.m_Vertices;
//     m_Triangles = copyFrom.m_Triangles;
//     m_Batches = copyFrom.m_Batches;
//     m_LODs = copyFrom.m_LODs;
// 
//     return *this;
// }

// Interface
BinaryBlob *ResourceCompiler::CompileStaticMesh(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.staticmesh.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileStaticMesh: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    StaticMeshGenerator *pGenerator = new StaticMeshGenerator();
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
