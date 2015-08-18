#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/CollisionShapeGenerator.h"
#include "YBaseLib/BinaryWriteBuffer.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(CollisionShapeGenerator);

// Import bullet headers
#include "Engine/Physics/BulletHeaders.h"
//#include "hacdCircularList.h"
//#include "hacdVector.h"
//#include "hacdICHull.h"
//#include "hacdGraph.h"
//#include "hacdHACD.h"

namespace Physics {

CollisionShapeGenerator::CollisionShapeGenerator(COLLISION_SHAPE_TYPE type /* = COLLISION_SHAPE_TYPE_TRIANGLE_MESH */)
    : m_type(type),
      m_boxCenter(float3::Zero),
      m_boxHalfExtents(float3::Zero),
      m_sphereCenter(float3::Zero),
      m_sphereRadius(0.0f)
{

}

CollisionShapeGenerator::~CollisionShapeGenerator()
{

}

void CollisionShapeGenerator::SetType(COLLISION_SHAPE_TYPE type)
{
    m_type = type;
    m_boxCenter = float3::Zero;
    m_boxHalfExtents = float3::Zero;
    m_sphereCenter = float3::Zero;
    m_sphereRadius = 0.0f;
    m_triangleMeshTriangles.Clear();
    m_convexHullVertices.Clear();
}

void CollisionShapeGenerator::AddTriangle(const float3 &v0, const float3 &v1, const float3 &v2)
{
    DebugAssert(m_type == COLLISION_SHAPE_TYPE_TRIANGLE_MESH);
    
    TriangleMeshTriangle triangle;
    triangle.VertexPositions[0] = v0;
    triangle.VertexPositions[1] = v1;
    triangle.VertexPositions[2] = v2;
    m_triangleMeshTriangles.Add(triangle);
}

void CollisionShapeGenerator::AddConvexHullVertex(const float3 &v)
{
    DebugAssert(m_type == COLLISION_SHAPE_TYPE_CONVEX_HULL);
    m_convexHullVertices.Add(v);
}

void CollisionShapeGenerator::ConvertToBox()
{
    Assert(m_type == COLLISION_SHAPE_TYPE_TRIANGLE_MESH);

    m_type = COLLISION_SHAPE_TYPE_BOX;
    if (m_triangleMeshTriangles.GetSize() == 0)
    {
        m_boxHalfExtents.SetZero();
    }
    else
    {
        // take the bounding box of all the vertices in the triangle mesh
        AABox boundingBox(m_triangleMeshTriangles[0].VertexPositions[0]);
        for (uint32 i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                boundingBox.Merge(m_triangleMeshTriangles[i].VertexPositions[j]);
            }
        }
        m_triangleMeshTriangles.Obliterate();

        // get center and extents
        m_boxHalfExtents = boundingBox.GetExtents() * 0.5f;
    }
}

void CollisionShapeGenerator::ConvertToSphere()
{
    Assert(m_type == COLLISION_SHAPE_TYPE_TRIANGLE_MESH);

    m_type = COLLISION_SHAPE_TYPE_SPHERE;
    if (m_triangleMeshTriangles.GetSize() == 0)
    {
        m_sphereRadius = Math::Epsilon<float>();
    }
    else
    {
        // take the bounding sphere of all the vertices in the triangle mesh
        AABox boundingBox(m_triangleMeshTriangles[0].VertexPositions[0]);
        for (uint32 i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                boundingBox.Merge(m_triangleMeshTriangles[i].VertexPositions[j]);
            }
        }

        Sphere boundingSphere(boundingBox.GetCenter(), 0.0f);
        for (uint32 i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
        {
            for (uint32 j = 0; j < 3; j++)
            {
                boundingSphere.Merge(m_triangleMeshTriangles[i].VertexPositions[j]);
            }
        }
        m_triangleMeshTriangles.Obliterate();

        // get center and extents
        m_sphereRadius = boundingSphere.GetRadius();
    }
}

void CollisionShapeGenerator::ConvertToConvexHull()
{
    Assert(m_type == COLLISION_SHAPE_TYPE_TRIANGLE_MESH);
    Panic("unimplemented");
}

bool CollisionShapeGenerator::LoadFromXML(XMLReader &xmlReader)
{
    // process xml nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 shapeSelection = xmlReader.Select("box|sphere|triangle-mesh|convex-hull");
            if (shapeSelection < 0)
                return false;

            switch (shapeSelection)
            {
                // box
            case 0:
                {
                    SetType(COLLISION_SHAPE_TYPE_BOX);
                    m_boxCenter = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("center", "0 0 0"));
                    m_boxHalfExtents = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("half-extents", "0 0 0"));

                    if (!xmlReader.SkipCurrentElement())
                        return false;
                }
                break;

                // sphere
            case 1:
                {
                    SetType(COLLISION_SHAPE_TYPE_SPHERE);
                    m_sphereCenter = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("center", "0 0 0"));
                    m_sphereRadius = StringConverter::StringToFloat(xmlReader.FetchAttributeDefault("radius", "0"));

                    if (!xmlReader.SkipCurrentElement())
                        return false;
                }
                break;

                // triangle-mesh
            case 2:
                {
                    SetType(COLLISION_SHAPE_TYPE_TRIANGLE_MESH);

                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                return false;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 triangleMeshSelection = xmlReader.Select("triangles");
                                if (triangleMeshSelection < 0)
                                    return false;

                                switch (triangleMeshSelection)
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

                                                    switch (triangleMeshSelection)
                                                    {
                                                        // triangle
                                                    case 0:
                                                        {
                                                            TriangleMeshTriangle triangle;
                                                            triangle.VertexPositions[0] = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("vertex1", "0 0 0"));
                                                            triangle.VertexPositions[1] = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("vertex2", "0 0 0"));
                                                            triangle.VertexPositions[2] = StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("vertex3", "0 0 0"));
                                                            m_triangleMeshTriangles.Add(triangle);

                                                            if (!xmlReader.SkipCurrentElement())
                                                                return false;
                                                        }
                                                        break;
                                                    }
                                                }
                                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                {
                                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "triangles") == 0);
                                                    break;
                                                }
                                                else
                                                {
                                                    xmlReader.PrintError("parse error");
                                                    return false;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "triangle-mesh") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                return false;
                            }
                        }
                    }
                }
                break;

                // convex-hull
            case 3:
                {
                    SetType(COLLISION_SHAPE_TYPE_CONVEX_HULL);

                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                return false;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 convexHullSelection = xmlReader.Select("vertices");
                                if (convexHullSelection < 0)
                                    return false;

                                switch (convexHullSelection)
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
                                                    int32 verticesSelection = xmlReader.Select("vertex");
                                                    if (verticesSelection < 0)
                                                        return false;

                                                    switch (convexHullSelection)
                                                    {
                                                        // triangle
                                                    case 0:
                                                        {
                                                            m_convexHullVertices.Add(StringConverter::StringToFloat3(xmlReader.FetchAttributeDefault("position", "0 0 0")));

                                                            if (!xmlReader.SkipCurrentElement())
                                                                return false;
                                                        }
                                                        break;
                                                    }
                                                }
                                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                {
                                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "vertices") == 0);
                                                    break;
                                                }
                                                else
                                                {
                                                    xmlReader.PrintError("parse error");
                                                    return false;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "convex-hull") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                return false;
                            }
                        }
                    }
                }
                break;
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            break;
        }
        else
        {
            xmlReader.PrintError("parse error");
            return false;
        }
    }

    return true;
}

bool CollisionShapeGenerator::SaveToXML(XMLWriter &xmlWriter) const
{
    uint32 i;
    SmallString tempString;

    switch (m_type)
    {
    case COLLISION_SHAPE_TYPE_BOX:
        {
            xmlWriter.StartElement("box");
            {
                StringConverter::Float3ToString(tempString, m_boxCenter);
                xmlWriter.WriteAttribute("center", tempString);

                StringConverter::Float3ToString(tempString, m_boxHalfExtents);
                xmlWriter.WriteAttribute("half-extents", tempString);
            }
            xmlWriter.EndElement();
        }
        break;

    case COLLISION_SHAPE_TYPE_SPHERE:
        {
            xmlWriter.StartElement("sphere");
            {
                StringConverter::Float3ToString(tempString, m_sphereCenter);
                xmlWriter.WriteAttribute("center", tempString);

                StringConverter::FloatToString(tempString, m_sphereRadius);
                xmlWriter.WriteAttribute("radius", tempString);
            }
            xmlWriter.EndElement();
        }
        break;

    case COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
        {
            xmlWriter.StartElement("triangle-mesh");
            {
                xmlWriter.StartElement("triangles");
                {
                    for (i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
                    {
                        xmlWriter.StartElement("triangle");
                        {
                            const TriangleMeshTriangle &triangle = m_triangleMeshTriangles[i];

                            StringConverter::Float3ToString(tempString, triangle.VertexPositions[0]);
                            xmlWriter.WriteAttribute("vertex1", tempString);

                            StringConverter::Float3ToString(tempString, triangle.VertexPositions[1]);
                            xmlWriter.WriteAttribute("vertex2", tempString);

                            StringConverter::Float3ToString(tempString, triangle.VertexPositions[2]);
                            xmlWriter.WriteAttribute("vertex3", tempString);
                        }
                        xmlWriter.EndElement();
                    }
                }
                xmlWriter.EndElement();
            }
            xmlWriter.EndElement();
        }
        break;

    case COLLISION_SHAPE_TYPE_CONVEX_HULL:
        {
            xmlWriter.StartElement("convex-hull");
            {
                xmlWriter.StartElement("vertices");
                {
                    for (i = 0; i < m_convexHullVertices.GetSize(); i++)
                    {
                        xmlWriter.StartElement("vertex");
                        {
                            StringConverter::Float3ToString(tempString, m_convexHullVertices[i]);
                            xmlWriter.WriteAttribute("position", tempString);
                        }
                        xmlWriter.EndElement();
                    }
                }
                xmlWriter.EndElement();
            }
            xmlWriter.EndElement();
        }
        break;

    default:
        UnreachableCode();
        break;
    }
    
    return (!xmlWriter.InErrorState());
}

bool CollisionShapeGenerator::Compile(BinaryBlob **ppOutputBlob) const
{
    switch (m_type)
    {
    case COLLISION_SHAPE_TYPE_BOX:
        {
            BinaryWriteBuffer binaryWriter;            
            binaryWriter << uint8(COLLISION_SHAPE_TYPE_BOX);
            binaryWriter << m_boxCenter;
            binaryWriter << m_boxHalfExtents;
            *ppOutputBlob = BinaryBlob::CreateFromPointer(binaryWriter.GetBufferPointer(), binaryWriter.GetBufferSize());
            return true;
        }
        break;

    case COLLISION_SHAPE_TYPE_SPHERE:
        {
            BinaryWriteBuffer binaryWriter;
            binaryWriter << uint8(COLLISION_SHAPE_TYPE_SPHERE);
            binaryWriter << m_sphereCenter;
            binaryWriter << m_sphereRadius;
            *ppOutputBlob = BinaryBlob::CreateFromPointer(binaryWriter.GetBufferPointer(), binaryWriter.GetBufferSize());
            return true;
        }
        break;
    
    case COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
        {
            // create triangle mesh
            btTriangleMesh *pBulletTriangleMesh = new btTriangleMesh();


#if 0
            // pull in the triangles
            for (uint32 i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
            {
                const TriangleMeshTriangle &triangle = m_triangleMeshTriangles[i];
                pBulletTriangleMesh->addTriangle(BulletHelpers::Float3ToBulletVector(triangle.VertexPositions[0]),
                                                 BulletHelpers::Float3ToBulletVector(triangle.VertexPositions[1]),
                                                 BulletHelpers::Float3ToBulletVector(triangle.VertexPositions[2]),
                                                 true);
            }

#elif 1

            // extract vertices
            float3 *pVertices = new float3[m_triangleMeshTriangles.GetSize() * 3];
            HashTable<float3, uint32> *pVerticesHashTable = new HashTable<float3, uint32>();
            uint32 *pIndices = new uint32[m_triangleMeshTriangles.GetSize() * 3];
            uint32 nVertices = 0;
            uint32 nIndices = 0;
            for (uint32 i = 0; i < m_triangleMeshTriangles.GetSize(); i++)
            {
                const TriangleMeshTriangle &triangle = m_triangleMeshTriangles[i];
                for (uint32 j = 0; j < 3; j++)
                {
                    const float3 &vertex = triangle.VertexPositions[j];
                    HashTable<float3, uint32>::Member *pMember = pVerticesHashTable->Find(vertex);
                    if (pMember != NULL)
                    {
                        DebugAssert(pMember->Key == vertex && pVertices[pMember->Value] == vertex);
                    }
                    else
                    {
                        pVertices[nVertices] = vertex;
                        pMember = pVerticesHashTable->Insert(vertex, nVertices);
                        nVertices++;
                    }

                    pIndices[nIndices++] = pMember->Value;
                }
            }

            // add to bullet shape
            for (uint32 i = 0; i < nVertices; i++)
                pBulletTriangleMesh->findOrAddVertex(Float3ToBulletVector(pVertices[i]), false);
            for (uint32 i = 0; i < nIndices; i++)
                pBulletTriangleMesh->addIndex(pIndices[i]);
            pBulletTriangleMesh->getIndexedMeshArray()[0].m_numTriangles = nIndices / 3;

            // clean temp memory
            delete[] pIndices;
            delete pVerticesHashTable;
            delete[] pVertices;

#endif

            // create the shape
            btBvhTriangleMeshShape *pShape = new btBvhTriangleMeshShape(pBulletTriangleMesh, true, true);
        
            // serialize it
            btDefaultSerializer serializer;
            serializer.startSerialization();
            pShape->serializeSingleShape(&serializer);
            serializer.finishSerialization();

            // create the blob
            BinaryBlob *pOutputBlob = BinaryBlob::Allocate(sizeof(uint8) + serializer.getCurrentBufferSize());
            *(uint8 *)(pOutputBlob->GetDataPointer()) = COLLISION_SHAPE_TYPE_TRIANGLE_MESH;
            Y_memcpy(reinterpret_cast<byte *>(pOutputBlob->GetDataPointer()) + sizeof(uint8), serializer.getBufferPointer(), serializer.getCurrentBufferSize());
            *ppOutputBlob = pOutputBlob;
        
            // clean up
            delete pShape;
            delete pBulletTriangleMesh;
            return true;
        }
        break;

    case COLLISION_SHAPE_TYPE_CONVEX_HULL:
        {
            //*ppOutputBlob = BinaryBlob::CreateFromPointer()
            *ppOutputBlob = NULL;
            //return true;
            return false;
        }
        break;

    default:
        UnreachableCode();
        return false;
    }
}

void CollisionShapeGenerator::Copy(const CollisionShapeGenerator *pGenerator)
{
    // just copy all vars, meh.
    m_type = pGenerator->m_type;
    m_boxCenter = pGenerator->m_boxCenter;
    m_boxHalfExtents = pGenerator->m_boxHalfExtents;
    m_sphereCenter = pGenerator->m_sphereCenter;
    m_sphereRadius = pGenerator->m_sphereRadius;
    m_triangleMeshTriangles.Assign(pGenerator->m_triangleMeshTriangles);
    m_convexHullVertices.Assign(pGenerator->m_convexHullVertices);
}

bool CollisionShapeGenerator::ApplyTransform(const Transform &transform)
{
    switch (m_type)
    {
    case COLLISION_SHAPE_TYPE_BOX:
        {
            AABox tempBox(m_boxCenter - m_boxHalfExtents, m_boxCenter + m_boxHalfExtents);
            AABox transformedBox(transform.TransformBoundingBox(tempBox));
            m_boxCenter = transformedBox.GetCenter();
            m_boxHalfExtents = transformedBox.GetExtents() / 2.0f;
            return true;
        }

    case COLLISION_SHAPE_TYPE_SPHERE:
        {
            Sphere tempSphere(m_sphereCenter, m_sphereRadius);
            Sphere transformedSphere(transform.TransformBoundingSphere(tempSphere));
            m_sphereCenter = transformedSphere.GetCenter();
            m_sphereRadius = transformedSphere.GetRadius();
            return true;
        }
    
    case COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
        {
            for (TriangleMeshTriangle &tri : m_triangleMeshTriangles)
            {
                tri.VertexPositions[0] = transform.TransformPoint(tri.VertexPositions[0]);
                tri.VertexPositions[1] = transform.TransformPoint(tri.VertexPositions[1]);
                tri.VertexPositions[2] = transform.TransformPoint(tri.VertexPositions[2]);
            }
            return true;
        }
        
    case COLLISION_SHAPE_TYPE_CONVEX_HULL:
        {
            return false;
        }

    default:
        UnreachableCode();
        return false;
    }
}

bool CollisionShapeGenerator::ApplyTransform(const float4x4 &transformMatrix)
{
    switch (m_type)
    {
    case COLLISION_SHAPE_TYPE_BOX:
        {
            AABox tempBox(m_boxCenter - m_boxHalfExtents, m_boxCenter + m_boxHalfExtents);
            AABox transformedBox(tempBox.GetTransformed(transformMatrix));
            m_boxCenter = transformedBox.GetCenter();
            m_boxHalfExtents = transformedBox.GetExtents() / 2.0f;
            return true;
        }

    case COLLISION_SHAPE_TYPE_SPHERE:
        {
            Sphere tempSphere(m_sphereCenter, m_sphereRadius);
            Sphere transformedSphere(tempSphere.GetTransformed(transformMatrix));
            m_sphereCenter = transformedSphere.GetCenter();
            m_sphereRadius = transformedSphere.GetRadius();
            return true;
        }

    case COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
        {
            for (TriangleMeshTriangle &tri : m_triangleMeshTriangles)
            {
                tri.VertexPositions[0] = transformMatrix.TransformPoint(tri.VertexPositions[0]);
                tri.VertexPositions[1] = transformMatrix.TransformPoint(tri.VertexPositions[1]);
                tri.VertexPositions[2] = transformMatrix.TransformPoint(tri.VertexPositions[2]);
            }
            return true;
        }

    case COLLISION_SHAPE_TYPE_CONVEX_HULL:
        {
            return false;
        }

    default:
        UnreachableCode();
        return false;
    }
}

}       // namespace Physics
