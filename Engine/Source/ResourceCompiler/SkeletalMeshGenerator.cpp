#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/SkeletalMeshGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "ResourceCompiler/CollisionShapeGenerator.h"
#include "Engine/DataFormats.h"
#include "Core/ChunkFileWriter.h"
#include "Core/MeshUtilties.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(SkeletalMeshGenerator);

static const uint32 MAX_BONES_PER_BATCH = 64;
static const uint32 MAX_VERTICES_PER_BATCH = 0xFFFE;

SkeletalMeshGenerator::SkeletalMeshGenerator()
    : m_provideVertexTextureCoordinates(false),
      m_provideVertexColors(false),
      m_pCollisionShapeGenerator(nullptr)
{

}

SkeletalMeshGenerator::~SkeletalMeshGenerator()
{
    delete m_pCollisionShapeGenerator;
}

const int32 SkeletalMeshGenerator::FindBoneIndexByName(const char *boneName) const
{
    for (uint32 i = 0; i < m_bones.GetSize(); i++)
    {
        if (m_bones[i].Name.Compare(boneName))
            return (int32)i;
    }

    return -1;
}

SkeletalMeshGenerator::Bone *SkeletalMeshGenerator::AddBone(const char *boneName, const Transform &localToBoneTransform)
{
    Assert(FindBoneIndexByName(boneName) == -1);

    uint32 boneIndex = m_bones.GetSize();
    m_bones.Add(Bone(boneIndex, boneName, localToBoneTransform));
    return &m_bones[boneIndex];
}

const int32 SkeletalMeshGenerator::FindMaterialNameIndex(const char *materialName) const
{
    for (uint32 i = 0; i < m_materialNames.GetSize(); i++)
    {
        if (m_materialNames[i].Compare(materialName))
            return (int32)i;
    }

    return -1;
}

const uint32 SkeletalMeshGenerator::AddMaterialName(const char *materialName)
{
    for (uint32 i = 0; i < m_materialNames.GetSize(); i++)
    {
        if (m_materialNames[i].Compare(materialName))
            return i;
    }

    uint32 materialNameIndex = m_materialNames.GetSize();
    m_materialNames.Add(String(materialName));
    return materialNameIndex;
}

uint32 SkeletalMeshGenerator::AddVertex(const Vertex &vertex)
{
    for (uint32 i = 0; i < SKELETAL_MESH_MAX_BONES_PER_VERTEX; i++)
    {
        DebugAssert(vertex.BoneIndices[i] < 0 || (uint32)vertex.BoneIndices[i] < m_bones.GetSize());
    }

    uint32 index = m_vertices.GetSize();
    m_vertices.Add(vertex);
    return index;
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates)
{
    return AddVertex(position, textureCoordinates, float3::UnitX, float3::UnitY, float3::UnitZ);
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &normal)
{
    return AddVertex(position, textureCoordinates, float3::UnitX, float3::UnitY, normal);
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &tangent, const float3 &binormal, const float3 &normal)
{
    Vertex vert;
    vert.Position = position;
    vert.TextureCoordinates = textureCoordinates;
    vert.Tangent = tangent;
    vert.Binormal = binormal;
    vert.Normal = normal;

    for (uint32 i = 0; i < SKELETAL_MESH_MAX_BONES_PER_VERTEX; i++)
    {
        vert.BoneIndices[i] = -1;
        vert.BoneWeights[i] = 0.0f;
    }

    uint32 index = m_vertices.GetSize();
    m_vertices.Add(vert);
    return index;
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates, const int32 *pBoneIndices, const float *pBoneWeights)
{
    return AddVertex(position, textureCoordinates, float3::UnitX, float3::UnitY, float3::UnitZ, pBoneIndices, pBoneWeights);
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &normal, const int32 *pBoneIndices, const float *pBoneWeights)
{
    return AddVertex(position, textureCoordinates, float3::UnitX, float3::UnitY, normal, pBoneIndices, pBoneWeights);
}

uint32 SkeletalMeshGenerator::AddVertex(const float3 &position, const float2 &textureCoordinates, const float3 &tangent, const float3 &binormal, const float3 &normal, const int32 *pBoneIndices, const float *pBoneWeights)
{
    Vertex vert;
    vert.Position = position;
    vert.TextureCoordinates = textureCoordinates;
    vert.Tangent = tangent;
    vert.Binormal = binormal;
    vert.Normal = normal;
    Y_memcpy(vert.BoneIndices, pBoneIndices, sizeof(vert.BoneIndices));
    Y_memcpy(vert.BoneWeights, pBoneWeights, sizeof(vert.BoneWeights));

    for (uint32 i = 0; i < SKELETAL_MESH_MAX_BONES_PER_VERTEX; i++)
    {
        DebugAssert(vert.BoneIndices[i] < 0 || (uint32)vert.BoneIndices[i] < m_bones.GetSize());
    }

    uint32 index = m_vertices.GetSize();
    m_vertices.Add(vert);
    return index;
}

uint32 SkeletalMeshGenerator::AddTriangle(uint32 materialNameIndex, uint64 smoothingGroups, uint32 v0, uint32 v1, uint32 v2)
{
    DebugAssert(materialNameIndex < m_materialNames.GetSize());

    Triangle tri;
    tri.MaterialIndex = materialNameIndex;
    tri.SmoothingGroups = smoothingGroups;
    tri.VertexIndices[0] = v0;
    tri.VertexIndices[1] = v1;
    tri.VertexIndices[2] = v2;
    
    uint32 index = m_triangles.GetSize();
    m_triangles.Add(tri);
    return index;
}

uint32 SkeletalMeshGenerator::AddTriangle(uint32 materialNameIndex, uint64 smoothingGroups, const uint32 *pIndices)
{
    DebugAssert(materialNameIndex < m_materialNames.GetSize());

    Triangle tri;
    tri.MaterialIndex = materialNameIndex;
    tri.SmoothingGroups = smoothingGroups;
    Y_memcpy(tri.VertexIndices, pIndices, sizeof(uint32) * 3);

    uint32 index = m_triangles.GetSize();
    m_triangles.Add(tri);
    return index;
}

void SkeletalMeshGenerator::CalculateTangentVectors()
{
    // zero everything
    for (uint32 i = 0; i < m_vertices.GetSize(); i++)
    {
        Vertex &vertex = m_vertices[i];
        vertex.Tangent.SetZero();
        vertex.Binormal.SetZero();
    }

    // build each triangle's tangent space
    // todo: smoothing groups
    for (uint32 i = 0; i < m_triangles.GetSize(); i++)
    {
        Triangle &tri = m_triangles[i];
        Vertex &v0 = m_vertices[tri.VertexIndices[0]];
        Vertex &v1 = m_vertices[tri.VertexIndices[1]];
        Vertex &v2 = m_vertices[tri.VertexIndices[2]];

        float3 tangent, binormal, normal;
        MeshUtilites::CalculateTangentSpaceVectors(v0.Position, v1.Position, v2.Position, v0.TextureCoordinates, v1.TextureCoordinates, v2.TextureCoordinates, tangent, binormal, normal);

        v0.Tangent += tangent;
        v0.Binormal += binormal;
        v1.Tangent += tangent;
        v1.Binormal += binormal;
        v2.Tangent += tangent;
        v2.Binormal += binormal;
    }

    // normalize everything
    for (uint32 i = 0; i < m_vertices.GetSize(); i++)
    {
        Vertex &vertex = m_vertices[i];
        if (vertex.Tangent.SquaredLength() > 0.0f)
            vertex.Tangent.NormalizeInPlace();
        if (vertex.Binormal.SquaredLength() > 0.0f)
            vertex.Binormal.NormalizeInPlace();
    }
}

void SkeletalMeshGenerator::CalculateTangentVectorsAndNormals()
{
    // zero everything
    for (uint32 i = 0; i < m_vertices.GetSize(); i++)
    {
        Vertex &vertex = m_vertices[i];
        vertex.Tangent.SetZero();
        vertex.Binormal.SetZero();
        vertex.Normal.SetZero();
    }

    // build each triangle's tangent space
    // todo: smoothing groups
    for (uint32 i = 0; i < m_triangles.GetSize(); i++)
    {
        Triangle &tri = m_triangles[i];
        Vertex &v0 = m_vertices[tri.VertexIndices[0]];
        Vertex &v1 = m_vertices[tri.VertexIndices[1]];
        Vertex &v2 = m_vertices[tri.VertexIndices[2]];

        float3 tangent, binormal, normal;
        MeshUtilites::CalculateTangentSpaceVectors(v0.Position, v1.Position, v2.Position, v0.TextureCoordinates, v1.TextureCoordinates, v2.TextureCoordinates, tangent, binormal, normal);

        v0.Tangent += tangent;
        v0.Binormal += binormal;
        v0.Normal += normal;
        v1.Tangent += tangent;
        v1.Binormal += binormal;
        v1.Normal += normal;
        v2.Tangent += tangent;
        v2.Binormal += binormal;
        v2.Normal += normal;
    }

    // normalize everything
    for (uint32 i = 0; i < m_vertices.GetSize(); i++)
    {
        Vertex &vertex = m_vertices[i];
        if (vertex.Tangent.SquaredLength() > 0.0f)
            vertex.Tangent.NormalizeInPlace();
        if (vertex.Binormal.SquaredLength() > 0.0f)
            vertex.Binormal.NormalizeInPlace();
        if (vertex.Normal.SquaredLength() > 0.0f)
            vertex.Normal.NormalizeInPlace();
    }
}

bool SkeletalMeshGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
        return false;

    if (!xmlReader.SkipToElement("skeletal-mesh"))
    {
        xmlReader.PrintError("could not skip to skeletal-mesh element.");
        return false;
    }

    // process xml nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            break;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 skeletalMeshSelection = xmlReader.Select("skeleton|flags|bones|vertices|triangles|collision-shape");
            if (skeletalMeshSelection < 0)
                return false;

            switch (skeletalMeshSelection)
            {
                // skeleton
            case 0:
                {
                    const char *nameStr = xmlReader.FetchAttribute("name");
                    if (nameStr == nullptr)
                    {
                        xmlReader.PrintError("missing name attribute");
                        return false;
                    }

                    m_skeletonName = nameStr;
                }
                break;

                // flags
            case 1:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 flagsSelection = xmlReader.Select("vertex-colors|vertex-texture-coordinates");
                                if (flagsSelection < 0)
                                    return false;

                                switch (flagsSelection)
                                {
                                    // vertex-colors
                                case 0:
                                    m_provideVertexColors = true;
                                    break;

                                    // vertex-texture-coordinates
                                case 1:
                                    m_provideVertexTextureCoordinates = true;
                                    break;

                                default:
                                    UnreachableCode();
                                    break;
                                }

                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                    return false;
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "flags") == 0);
                                break;
                            }
                            else
                            {
                                UnreachableCode();
                                break;
                            }
                        }
                    }
                }
                break;

                // bones
            case 2:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 bonesSelection = xmlReader.Select("bone");
                                if (bonesSelection < 0)
                                    return false;

                                const char *boneNameStr = xmlReader.FetchAttribute("name");
                                const char *boneTransformPositionStr = xmlReader.FetchAttribute("transform-position");
                                const char *boneTransformRotationStr = xmlReader.FetchAttribute("transform-rotation");
                                const char *boneTransformScaleStr = xmlReader.FetchAttribute("transform-scale");

                                if (boneNameStr == NULL || boneTransformPositionStr == NULL ||
                                    boneTransformRotationStr == NULL || boneTransformScaleStr == NULL)
                                {
                                    xmlReader.PrintError("missing bone fields");
                                    return false;
                                }

                                if (FindBoneIndexByName(boneNameStr) >= 0)
                                {
                                    xmlReader.PrintError("duplicate bone '%s'", boneNameStr);
                                    return false;
                                }

                                float3 boneTransformPosition = StringConverter::StringToFloat3(boneTransformPositionStr);
                                Quaternion boneTransformRotation = StringConverter::StringToQuaternion(boneTransformRotationStr);
                                float3 boneTransformScale = StringConverter::StringToFloat3(boneTransformScaleStr);

                                m_bones.Add(Bone(m_bones.GetSize(), boneNameStr, Transform(boneTransformPosition, boneTransformRotation, boneTransformScale)));

                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                    return false;
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "bones") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                break;
                            }
                        }
                    }
                }
                break;

                // vertices
            case 3:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 verticesSelection = xmlReader.Select("vertex");
                                if (verticesSelection < 0)
                                    return false;

                                const char *vertexPositionStr = xmlReader.FetchAttribute("position");
                                const char *vertexTangentStr = xmlReader.FetchAttribute("tangent");
                                const char *vertexBinormalStr = xmlReader.FetchAttribute("binormal");
                                const char *vertexNormalStr = xmlReader.FetchAttribute("normal");
                                const char *vertexTextureCoordinatesStr = xmlReader.FetchAttribute("texture-coordinates");
                                const char *vertexColorStr = xmlReader.FetchAttribute("color");
                                if (vertexPositionStr == nullptr || 
                                    vertexTangentStr == nullptr || vertexBinormalStr == nullptr || vertexNormalStr == nullptr ||
                                    vertexTextureCoordinatesStr == nullptr || vertexColorStr == nullptr)
                                {
                                    xmlReader.PrintError("missing attributes");
                                    return false;
                                }

                                // create vertex
                                Vertex vert;
                                vert.Position = StringConverter::StringToFloat3(vertexPositionStr);
                                vert.Tangent = StringConverter::StringToFloat3(vertexTangentStr);
                                vert.Binormal = StringConverter::StringToFloat3(vertexBinormalStr);
                                vert.Normal = StringConverter::StringToFloat3(vertexNormalStr);
                                vert.TextureCoordinates = StringConverter::StringToFloat2(vertexTextureCoordinatesStr);
                                vert.Color = StringConverter::StringToColor(vertexColorStr);
                                Y_memset(vert.BoneIndices, (byte)-1, sizeof(vert.BoneIndices));
                                Y_memzero(vert.BoneWeights, sizeof(vert.BoneWeights));

                                if (!xmlReader.IsEmptyElement())
                                {
                                    for (;;)
                                    {
                                        if (!xmlReader.NextToken())
                                            break;

                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                        {
                                            int32 vertexSelection = xmlReader.Select("weight");
                                            if (vertexSelection < 0)
                                                return false;

                                            switch (vertexSelection)
                                            {
                                                // weight
                                            case 0:
                                                {
                                                    const char *boneStr = xmlReader.FetchAttribute("bone");
                                                    const char *weightStr = xmlReader.FetchAttribute("weight");
                                                    if (boneStr == nullptr || weightStr == nullptr)
                                                    {
                                                        xmlReader.PrintError("missing attributes");
                                                        return false;
                                                    }

                                                    // find free index
                                                    uint32 weightIndex = 0;
                                                    for (; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                                                    {
                                                        if (vert.BoneIndices[weightIndex] == -1)
                                                            break;
                                                    }
                                                    if (weightIndex == SKELETAL_MESH_MAX_BONES_PER_VERTEX)
                                                    {
                                                        xmlReader.PrintError("too many weights");
                                                        return false;
                                                    }

                                                    // check bone index
                                                    int32 boneIndex = FindBoneIndexByName(boneStr);
                                                    if (boneIndex < 0)
                                                    {
                                                        xmlReader.PrintError("unknown bone");
                                                        return false;
                                                    }

                                                    vert.BoneIndices[weightIndex] = boneIndex;
                                                    vert.BoneWeights[weightIndex] = StringConverter::StringToFloat(weightStr);

                                                    if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                                        return false;
                                                }
                                                break;

                                            default:
                                                UnreachableCode();
                                                break;
                                            }
                                        }
                                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                        {
                                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "vertex") == 0);
                                            break;
                                        }
                                        else
                                        {
                                            UnreachableCode();
                                            break;
                                        }
                                    }
                                }

                                // check the sum equals one
                                float weightSum = 0.0f;
                                for (uint32 i = 0; i < SKELETAL_MESH_MAX_BONES_PER_VERTEX; i++)
                                    weightSum += vert.BoneWeights[i];
                                if (!Math::NearEqual(weightSum, 1.0f, Y_FLT_EPSILON))
                                {
                                    xmlReader.PrintWarning("vertex weights do not sum to 1, fixing");
                                    for (uint32 i = 0; i < SKELETAL_MESH_MAX_BONES_PER_VERTEX; i++)
                                        vert.BoneWeights[i] *= 1.0f / weightSum;
                                }

                                // add vert
                                m_vertices.Add(vert);
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "vertices") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                break;
                            }
                        }
                    }
                }
                break;

                // triangles
            case 4:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                break;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 trianglesSelection = xmlReader.Select("triangle");
                                if (trianglesSelection < 0)
                                    return false;

                                const char *materialNameStr = xmlReader.FetchAttribute("material");
                                const char *smoothingGroupsStr = xmlReader.FetchAttribute("smoothing-groups");
                                const char *vertex1Str = xmlReader.FetchAttribute("vertex1");
                                const char *vertex2Str = xmlReader.FetchAttribute("vertex2");
                                const char *vertex3Str = xmlReader.FetchAttribute("vertex3");
                                if (materialNameStr == nullptr || smoothingGroupsStr == nullptr || vertex1Str == nullptr || vertex2Str == nullptr || vertex3Str == nullptr)
                                {
                                    xmlReader.PrintError("incomplete triangle");
                                    return false;
                                }

                                int32 materialIndex = FindMaterialNameIndex(materialNameStr);
                                if (materialIndex < 0)
                                    materialIndex = AddMaterialName(materialNameStr);

                                uint64 smoothingGroups = StringConverter::StringToUInt64(smoothingGroupsStr);
                                uint32 i0 = StringConverter::StringToUInt32(vertex1Str);
                                uint32 i1 = StringConverter::StringToUInt32(vertex2Str);
                                uint32 i2 = StringConverter::StringToUInt32(vertex3Str);
                                if (i0 > m_vertices.GetSize() || i1 > m_vertices.GetSize() || i2 > m_vertices.GetSize())
                                {
                                    xmlReader.PrintError("one or more indices are out of range");
                                    return false;
                                }

                                Triangle tri;
                                tri.MaterialIndex = materialIndex;
                                tri.SmoothingGroups = smoothingGroups;
                                tri.VertexIndices[0] = i0;
                                tri.VertexIndices[1] = i1;
                                tri.VertexIndices[2] = i2;
                                m_triangles.Add(tri);

                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                    return false;
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "triangles") == 0);
                                break;
                            }
                            else
                            {
                                xmlReader.PrintError("parse error");
                                break;
                            }
                        }
                    }
                }
                break;

                // collision-shape
            case 5:
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

            default:
                UnreachableCode();
                break;
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "skeletal-mesh") == 0);
            break;
        }
        else
        {
            xmlReader.PrintError("parse error");
            return false;
        }
    }

    if (m_skeletonName.IsEmpty())
    {
        xmlReader.PrintError("missing skeleton");
        return false;
    }

    return true;
}

bool SkeletalMeshGenerator::SaveToXML(ByteStream *pStream) const
{
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("skeletal-mesh");
    {
        xmlWriter.StartElement("skeleton");
        xmlWriter.WriteAttribute("name", m_skeletonName);
        xmlWriter.EndElement(); // </skeleton>

        xmlWriter.StartElement("flags");
        {
            if (m_provideVertexColors)
                xmlWriter.WriteEmptyElement("vertex-colors");
            if (m_provideVertexTextureCoordinates)
                xmlWriter.WriteEmptyElement("vertex-texture-coordinates");
        }
        xmlWriter.EndElement(); // </flags>

        xmlWriter.StartElement("bones");
        {
            for (uint32 i = 0; i < m_bones.GetSize(); i++)
            {
                const Bone *pBone = &m_bones[i];

                xmlWriter.StartElement("bone");
                {
                    xmlWriter.WriteAttribute("name", pBone->Name);
                    xmlWriter.WriteAttribute("transform-position", StringConverter::Float3ToString(pBone->LocalToBoneTransform.GetPosition()));
                    xmlWriter.WriteAttribute("transform-rotation", StringConverter::QuaternionToString(pBone->LocalToBoneTransform.GetRotation()));
                    xmlWriter.WriteAttribute("transform-scale", StringConverter::Float3ToString(pBone->LocalToBoneTransform.GetScale()));
                }
                xmlWriter.EndElement(); // </bone>
            }
        }
        xmlWriter.EndElement(); // </bones>

        xmlWriter.StartElement("vertices");
        {
            for (uint32 j = 0; j < m_vertices.GetSize(); j++)
            {
                const Vertex *pVertex = &m_vertices[j];
                xmlWriter.StartElement("vertex");
                {
                    xmlWriter.WriteAttribute("position", StringConverter::Float3ToString(pVertex->Position));
                    xmlWriter.WriteAttribute("tangent", StringConverter::Float3ToString(pVertex->Tangent));
                    xmlWriter.WriteAttribute("binormal", StringConverter::Float3ToString(pVertex->Binormal));
                    xmlWriter.WriteAttribute("normal", StringConverter::Float3ToString(pVertex->Normal));

                    xmlWriter.WriteAttribute("texture-coordinates", StringConverter::Float2ToString(pVertex->TextureCoordinates));
                    xmlWriter.WriteAttribute("color", StringConverter::ColorToString(pVertex->Color));

                    for (uint32 k = 0; k < SKELETAL_MESH_MAX_BONES_PER_VERTEX; k++)
                    {
                        if (pVertex->BoneIndices[k] < 0)
                            continue;

                        DebugAssert((uint32)pVertex->BoneIndices[k] < m_bones.GetSize());
                        xmlWriter.StartElement("weight");
                        {
                            xmlWriter.WriteAttribute("bone", m_bones[pVertex->BoneIndices[k]].Name);
                            xmlWriter.WriteAttribute("weight", StringConverter::FloatToString(pVertex->BoneWeights[k]));
                        }
                        xmlWriter.EndElement(); // </weight>
                    }
                }
                xmlWriter.EndElement(); // </vertex>
            }                        
        }
        xmlWriter.EndElement(); // </vertices>

        xmlWriter.StartElement("triangles");
        {
            for (uint32 j = 0; j < m_triangles.GetSize(); j++)
            {
                const Triangle *pTriangle = &m_triangles[j];
                xmlWriter.StartElement("triangle");
                {
                    xmlWriter.WriteAttribute("material", m_materialNames[pTriangle->MaterialIndex]);
                    xmlWriter.WriteAttribute("smoothing-groups", StringConverter::UInt64ToString(pTriangle->SmoothingGroups));
                    xmlWriter.WriteAttribute("vertex1", StringConverter::UInt32ToString(pTriangle->VertexIndices[0]));
                    xmlWriter.WriteAttribute("vertex2", StringConverter::UInt32ToString(pTriangle->VertexIndices[1]));
                    xmlWriter.WriteAttribute("vertex3", StringConverter::UInt32ToString(pTriangle->VertexIndices[2]));
                }
                xmlWriter.EndElement(); // </triangle>
            }
        }
        xmlWriter.EndElement(); // </triangles>

        if (m_pCollisionShapeGenerator != nullptr)
        {
            xmlWriter.StartElement("collision-shape");
            //xmlWriter.WriteAttribute("type", NameTable_GetNameString(Physics::NameTables::CollisionShapeType, m_pCollisionShapeGenerator->GetType()));
            if (!m_pCollisionShapeGenerator->SaveToXML(xmlWriter))
                return false;

            xmlWriter.EndElement(); // </collision-shape>
        }
    }
    xmlWriter.EndElement(); // </skeletal-mesh>

    return (!xmlWriter.InErrorState() && !pStream->InErrorState());
}

bool SkeletalMeshGenerator::BuildBoxCollisionShape()
{
    // get all vertex positions
    MemArray<float3> vertexPositions;
    for (const Vertex &vertex : m_vertices)
        vertexPositions.Add(vertex.Position);

    // calculate bounding box of all vertices
    AABox boundingBox(AABox::FromPoints(vertexPositions.GetBasePointer(), vertexPositions.GetSize()));

    // create shape
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_BOX);
    m_pCollisionShapeGenerator->SetBoxCenter(boundingBox.GetCenter());
    m_pCollisionShapeGenerator->SetBoxHalfExtents(boundingBox.GetExtents() / 2.0f);
    return true;
}

bool SkeletalMeshGenerator::BuildSphereCollisionShape()
{
    // get all vertex positions
    MemArray<float3> vertexPositions;
    for (const Vertex &vertex : m_vertices)
        vertexPositions.Add(vertex.Position);

    // calculate bounding box of all vertices
    Sphere boundingSphere(Sphere::FromPoints(vertexPositions.GetBasePointer(), vertexPositions.GetSize()));

    // create shape
    delete m_pCollisionShapeGenerator;
    m_pCollisionShapeGenerator = new Physics::CollisionShapeGenerator(Physics::COLLISION_SHAPE_TYPE_SPHERE);
    m_pCollisionShapeGenerator->SetSphereCenter(boundingSphere.GetCenter());
    m_pCollisionShapeGenerator->SetSphereRadius(boundingSphere.GetRadius());
    return true;
}


bool SkeletalMeshGenerator::BuildTriangleMeshCollisionShape()
{
#if 0
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
#endif
    return false;
}


bool SkeletalMeshGenerator::BuildConvexHullCollisionShape()
{
    //InternalBuildTriangleMeshCollisionShape(buildFromLOD);
    //m_pCollisionShapeGenerator->ConvertToConvexHull();
    return false;
}

bool SkeletalMeshGenerator::Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const
{
    // load the skeleton we're using
    AutoReleasePtr<const Skeleton> pSkeleton = pCallbacks->GetCompiledSkeleton(m_skeletonName);
    if (pSkeleton == nullptr)
    {
        Log_ErrorPrintf("SkeletalMeshGenerator::Compile: Could not load Skeleton '%s'", m_skeletonName.GetCharArray());
        return false;
    }

    // calculate mesh flags
    uint32 meshFlags = 0;
    if (m_provideVertexTextureCoordinates)
        meshFlags |= DF_SKELETALMESH_FLAG_USE_TEXTURE_COORDINATES;
    if (m_provideVertexColors)
        meshFlags |= DF_SKELETALMESH_FLAG_USE_VERTEX_COLORS;

    // sanity check
    if (m_skeletonName.GetLength() == 0 || m_vertices.GetSize() == 0 || m_bones.GetSize() == 0 || m_triangles.GetSize() == 0)
    {
        Log_ErrorPrintf("SkeletalMeshGenerator::Compile: Incomplete mesh.");
        return false;
    }

    // map our bone names to the skeleton's bone names
    PODArray<const Skeleton::Bone *> skeletonBoneNameMapping;
    for (uint32 boneIndex = 0; boneIndex < m_bones.GetSize(); boneIndex++)
    {
        const Bone *pOurBone = &m_bones[boneIndex];
        const Skeleton::Bone *pSkeletonBone = pSkeleton->GetBoneByName(pOurBone->Name);
        if (pSkeletonBone == nullptr)
        {
            Log_ErrorPrintf("SkeletalMeshGenerator::Compile: Mesh references bone '%s' not present in skeleton '%s'", pOurBone->Name.GetCharArray(), pSkeleton->GetName().GetCharArray());
            return false;
        }

        skeletonBoneNameMapping.Add(pSkeletonBone);
    }

    // transform all vertices to bone space
    /*MemArray<Vertex> boneSpaceVertices;
    boneSpaceVertices.Reserve(m_vertices.GetSize());
    for (uint32 vertexIndex = 0; vertexIndex < m_vertices.GetSize(); vertexIndex++)
    {
        const Vertex *pVertex = &m_vertices[vertexIndex];

        Vertex boneSpaceVertex;
        boneSpaceVertex.Position = float3::Zero;
        boneSpaceVertex.Tangent = float3::Zero;
        boneSpaceVertex.Binormal = float3::Zero;
        boneSpaceVertex.Normal = float3::Zero;
        boneSpaceVertex.TextureCoordinates = pVertex->TextureCoordinates;
        boneSpaceVertex.Color = pVertex->Color;

        for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
        {
            int32 boneIndex = pVertex->BoneIndices[weightIndex];
            if (boneIndex >= 0)
            {
                DebugAssert((uint32)boneIndex < m_bones.GetSize());

                const Bone *pOurBone = &m_bones[boneIndex];
                float boneWeight = pVertex->BoneWeights[weightIndex];

                // transform to bone space
                boneSpaceVertex.Position += pOurBone->LocalToBoneTransform.TransformPoint(pVertex->Position) * boneWeight;
                boneSpaceVertex.Tangent += pOurBone->LocalToBoneTransform.TransformNormal(pVertex->Tangent) * boneWeight;
                boneSpaceVertex.Binormal += pOurBone->LocalToBoneTransform.TransformNormal(pVertex->Binormal) * boneWeight;
                boneSpaceVertex.Normal += pOurBone->LocalToBoneTransform.TransformNormal(pVertex->Normal) * boneWeight;

                // set index + weight
                boneSpaceVertex.BoneIndices[weightIndex] = skeletonBoneNameMapping[boneIndex]->GetIndex();
                boneSpaceVertex.BoneWeights[weightIndex] = boneWeight;
            }
            else
            {
                boneSpaceVertex.BoneIndices[weightIndex] = -1;
                boneSpaceVertex.BoneWeights[weightIndex] = 0.0f;
            }
        }

        // normalize normals
        boneSpaceVertex.Tangent.SafeNormalizeInPlace();
        boneSpaceVertex.Binormal.SafeNormalizeInPlace();
        boneSpaceVertex.Normal.SafeNormalizeInPlace();

        // add to output list
        boneSpaceVertices.Add(boneSpaceVertex);
    }*/

    // bbox/bsphere
    AABox boundingBox;
    Sphere boundingSphere;
    {
        // find vertex positions in base frame space
        MemArray<float3> baseFrameVertexPositions;
        baseFrameVertexPositions.Reserve(m_vertices.GetSize());

        // iterate over vertices
        for (uint32 vertexIndex = 0; vertexIndex < m_vertices.GetSize(); vertexIndex++)
        {
            const Vertex *pVertex = &m_vertices[vertexIndex];
            const float3 &localSpacePosition(pVertex->Position);
            float3 baseFrameSpacePosition(float3::Zero);

            for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
            {
                int32 boneIndex = pVertex->BoneIndices[weightIndex];
                if (boneIndex >= 0)
                {
                    float boneWeight = pVertex->BoneWeights[weightIndex];
                    const Bone *pBone = &m_bones[boneIndex];
                    DebugAssert((uint32)boneIndex < skeletonBoneNameMapping.GetSize());

                    // transform bone space, then to base frame
                    baseFrameSpacePosition += (skeletonBoneNameMapping[boneIndex]->GetAbsoluteBaseFrameTransform().GetTransformMatrix4x4() * 
                                               pBone->LocalToBoneTransform.GetTransformMatrix4x4()).TransformPoint(localSpacePosition) * 
                                               boneWeight;
                }
            }

            baseFrameVertexPositions.Add(baseFrameSpacePosition);
        }

        // calculate bounding box/sphere
        boundingBox = AABox::FromPoints(baseFrameVertexPositions.GetBasePointer(), baseFrameVertexPositions.GetSize());
        boundingSphere = Sphere::FromPoints(baseFrameVertexPositions.GetBasePointer(), baseFrameVertexPositions.GetSize());
    }

    // build output bones
    MemArray<DF_SKELETALMESH_BONE> outputBones;
    outputBones.Reserve(m_bones.GetSize());
    for (uint32 boneIndex = 0; boneIndex < m_bones.GetSize(); boneIndex++)
    {
        DF_SKELETALMESH_BONE outputBone;
        outputBone.SkeletonBoneIndex = skeletonBoneNameMapping[boneIndex]->GetIndex();
        m_bones[boneIndex].LocalToBoneTransform.GetPosition().Store(outputBone.LocalToBonePosition);
        m_bones[boneIndex].LocalToBoneTransform.GetRotation().Store(outputBone.LocalToBoneRotation);
        m_bones[boneIndex].LocalToBoneTransform.GetScale().Store(outputBone.LocalToBoneScale);
        outputBones.Add(outputBone);
    }

    // build output indices and batches
    MemArray<DF_SKELETALMESH_VERTEX> outputVertices;
    PODArray<uint16> outputIndices;
    MemArray<DF_SKELETALMESH_BATCH> outputBatches;
    PODArray<uint16> outputBoneRefs;
    PODArray<uint16> currentBoneRefs;
    PODArray<int32> currentVertexMapping;

    // copy the triangle list
    MemArray<Triangle> triangleList(m_triangles);

    // drain it
    while (triangleList.GetSize() > 0)
    {
        uint32 templateMaterialIndex;
        uint32 templateWeightCount;

        // use the first triangle as a template
        {
            const Triangle *firstTriangle = &triangleList[0];
            templateMaterialIndex = firstTriangle->MaterialIndex;

#if 0
            // find the number of weights for this triangle
            uint32 vertexWeightCounts[3];
            Y_memzero(vertexWeightCounts, sizeof(vertexWeightCounts));
            for (uint32 vertexIndex = 0; vertexIndex < 3; vertexIndex++)
            {
                const Vertex *pVertex = &m_vertices[firstTriangle->VertexIndices[vertexIndex]];
                for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                {
                    if (pVertex->BoneIndices[weightIndex] >= 0)
                        vertexWeightCounts[vertexIndex]++;
                    else
                        break;
                }
            }

            // select the maximum of the three vertices
            templateWeightCount = Max(vertexWeightCounts[0], Max(vertexWeightCounts[1], vertexWeightCounts[2]));
#else
            templateWeightCount = SKELETAL_MESH_MAX_BONES_PER_VERTEX;
#endif
        }

        // new batch
        DF_SKELETALMESH_BATCH batch;
        batch.MaterialIndex = templateMaterialIndex;
        batch.BaseBoneRef = outputBoneRefs.GetSize();
        batch.BoneRefCount = 0;
        batch.WeightCount = templateWeightCount;
        batch.BaseVertex = 0;// outputVertices.GetSize();
        batch.VertexCount = 0;
        batch.StartIndex = outputIndices.GetSize();
        batch.IndexCount = 0;

        // initialize arrays
        uint32 currentVertexCount = outputVertices.GetSize();
        currentBoneRefs.Clear();
        currentVertexMapping.Resize(m_vertices.GetSize());
        Y_memset(currentVertexMapping.GetBasePointer(), 0xFF, currentVertexMapping.GetStorageSizeInBytes());

        // find matching triangles
        for (uint32 triangleIndex = 0; triangleIndex < triangleList.GetSize(); )
        {
            const Triangle *triangle = &triangleList[triangleIndex];

            // check the material index
            if (triangle->MaterialIndex != templateMaterialIndex)
            {
                triangleIndex++;
                continue;
            }

#if 0
            // find the number of weights for this triangle
            Y_memzero(vertexWeightCounts, sizeof(vertexWeightCounts));
            for (uint32 vertexIndex = 0; vertexIndex < 3; vertexIndex++)
            {
                const Vertex *pVertex = &m_vertices[triangle->VertexIndices[vertexIndex]];
                for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                {
                    if (pVertex->BoneIndices[weightIndex] >= 0)
                        vertexWeightCounts[vertexIndex]++;
                    else
                        break;
                }
            }

            // weight count matches?
            uint32 weightCount = Max(vertexWeightCounts[0], Max(vertexWeightCounts[1], vertexWeightCounts[2]));
            if (weightCount != templateWeightCount)
            {
                triangleIndex++;
                continue;
            }
#endif

            // cool, we have a matching triangle
            // find out how much vertex space we need, and how many bone slots we need
            uint32 extraVerticesNeeded = 0;
            uint32 extraBoneSlotsNeeded = 0;
            for (uint32 vertexIndex = 0; vertexIndex < 3; vertexIndex++)
            {
                const uint32 vertexArrayIndex = triangle->VertexIndices[vertexIndex];
                const Vertex *pVertex = &m_vertices[vertexArrayIndex];

                // has a mapping for this vertex in this batch?
                if (currentVertexMapping[vertexArrayIndex] == -1)
                {
                    extraVerticesNeeded++;

                    // has a mapping for each of the bones in this batch?
                    for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                    {
                        if (pVertex->BoneIndices[weightIndex] >= 0)
                        {
                            if (currentBoneRefs.IndexOf(static_cast<uint16>(pVertex->BoneIndices[weightIndex])) < 0)
                                extraBoneSlotsNeeded++;
                        }
                    }
                }
            }

            // enough space?
            // this will screw up if the triangle is degenerate (ie two identical vertices), but that shouldn't happen anyway
            // currently the bone slots (if identical for each 3 vertices) will be 3 times the correct size.. fix me at some point?
            if ((currentVertexCount + extraVerticesNeeded) > MAX_VERTICES_PER_BATCH ||
                (currentBoneRefs.GetSize() + extraBoneSlotsNeeded) > MAX_BONES_PER_BATCH)
            {
                Log_PerfPrintf("Splitting skeletal mesh due to %s overrun", ((currentVertexCount + extraVerticesNeeded) > MAX_VERTICES_PER_BATCH) ? "vertex index" : "bone index");
                triangleIndex++;
                continue;
            }

            // this triangle is good to add!
            // loop through each of the vertices
            for (uint32 vertexIndex = 0; vertexIndex < 3; vertexIndex++)
            {
                const uint32 vertexArrayIndex = triangle->VertexIndices[vertexIndex];

                // vertex already done?
                int32 batchVertexIndex = currentVertexMapping[vertexArrayIndex];
                if (batchVertexIndex >= 0)
                {
                    // add this index
                    DebugAssert(batchVertexIndex < MAX_VERTICES_PER_BATCH);
                    outputIndices.Add(static_cast<uint16>(batchVertexIndex));
                    batch.IndexCount++;
                    continue;
                }

                // have to add the vertex, so copy the common information
                const Vertex *sourceVertex = &m_vertices[vertexArrayIndex];
                DF_SKELETALMESH_VERTEX outVertex;
                sourceVertex->Position.Store(outVertex.Position);
                sourceVertex->Tangent.Store(outVertex.Tangent);
                sourceVertex->Binormal.Store(outVertex.Binormal);
                sourceVertex->Normal.Store(outVertex.Normal);
                sourceVertex->TextureCoordinates.Store(outVertex.TextureCoordinates);
                outVertex.Color = sourceVertex->Color;

                // set all the bone indices to zero and the weights to zero
                bool hasWeights = false;
                Y_memzero(outVertex.BoneIndices, sizeof(outVertex.BoneIndices));
                Y_memzero(outVertex.BoneWeights, sizeof(outVertex.BoneWeights));

                // now the bones are a bit tricker, we have to re-index them
                for (uint32 weightIndex = 0; weightIndex < templateWeightCount; weightIndex++)
                {
                    int32 boneIndex = sourceVertex->BoneIndices[weightIndex];
                    DebugAssert(boneIndex < 0 || static_cast<uint32>(boneIndex) < m_bones.GetSize());

                    // if this vertex has no weight for this index, just set the ref index to zero (ie ignore it), there should be at least one ref always.
                    if (boneIndex < 0)
                        continue;

                    // in bone ref array?
                    int32 boneRefIndex = currentBoneRefs.IndexOf(static_cast<uint16>(boneIndex));
                    if (boneRefIndex >= 0)
                    {
                        // set the reference
                        DebugAssert(boneRefIndex < MAX_BONES_PER_BATCH);
                        outVertex.BoneIndices[weightIndex] = static_cast<uint8>(boneRefIndex);
                    }
                    else
                    {
                        // add it
                        DebugAssert(currentBoneRefs.GetSize() < MAX_BONES_PER_BATCH);
                        outVertex.BoneIndices[weightIndex] = static_cast<uint8>(currentBoneRefs.GetSize());
                        currentBoneRefs.Add(static_cast<uint16>(boneIndex));
                    }

                    // and carry the weight across
                    outVertex.BoneWeights[weightIndex] = sourceVertex->BoneWeights[weightIndex];
                    hasWeights = true;
                }

                // handle badly-exported models with no weights
                if (!hasWeights)
                {
                    Log_WarningPrintf("SkeletalMeshGenerator::Compile: Batch %u (%s): has a vertex with no weights", outputBatches.GetSize(), m_materialNames[templateMaterialIndex].GetCharArray());
                    if (currentBoneRefs.IsEmpty())
                        currentBoneRefs.Add(static_cast<uint16>(0));
                    outVertex.BoneIndices[0] = 0;
                    outVertex.BoneWeights[0] = 1.0f;
                }

                // add the vertex, and set the index
                DebugAssert(currentVertexCount < MAX_VERTICES_PER_BATCH);
                batchVertexIndex = static_cast<int32>(currentVertexCount);
                currentVertexMapping[vertexArrayIndex] = batchVertexIndex;
                outputVertices.Add(outVertex);
                currentVertexCount++;
                outputIndices.Add(static_cast<uint16>(batchVertexIndex));
                batch.IndexCount++;
            }

            // toss the triangle out
            triangleList.OrderedRemove(triangleIndex);
        }

        // this batch should have some stuff...
        DebugAssert(currentVertexCount > 0 && batch.IndexCount > 0 && currentBoneRefs.GetSize() > 0);

        // temp logging
        Log_DevPrintf("SkeletalMeshGenerator::Compile: Batch %u (%s): %u vertices, %u indices, %u bone refs with %u weights", outputBatches.GetSize(), m_materialNames[templateMaterialIndex].GetCharArray(), currentVertexCount, batch.IndexCount, currentBoneRefs.GetSize(), templateWeightCount);

        // add the bone refs back to the main list, and update the batch info
        batch.VertexCount = currentVertexCount;
        batch.BoneRefCount = currentBoneRefs.GetSize();
        outputBoneRefs.AddArray(currentBoneRefs);
        outputBatches.Add(batch);
    }
    
    // didn't get any batches?
    if (outputBatches.GetSize() == 0)
    {
        Log_ErrorPrintf("SkeletalMeshGenerator::Compile: No batches generated.");
        return false;
    }

    // temp logging
    Log_DevPrintf("SkeletalMeshGenerator::Compile: Built %u batches", outputBatches.GetSize());

    // determine flags
    uint32 skeletalMeshFlags = 0;
    if (m_provideVertexTextureCoordinates)
        skeletalMeshFlags |= DF_SKELETALMESH_FLAG_USE_TEXTURE_COORDINATES;
    if (m_provideVertexColors)
        skeletalMeshFlags |= DF_SKELETALMESH_FLAG_USE_VERTEX_COLORS;

    // build header
    DF_SKELETALMESH_HEADER fileHeader;
    fileHeader.Magic = DF_SKELETALMESH_HEADER_MAGIC;
    fileHeader.HeaderSize = sizeof(fileHeader);
    fileHeader.Flags = skeletalMeshFlags;
    boundingBox.GetMinBounds().Store(fileHeader.BoundingBoxMin);
    boundingBox.GetMaxBounds().Store(fileHeader.BoundingBoxMax);
    boundingSphere.GetCenter().Store(fileHeader.BoundingSphereCenter);
    fileHeader.BoundingSphereRadius = boundingSphere.GetRadius();
    fileHeader.MaterialCount = m_materialNames.GetSize();
    fileHeader.SkeletonBoneCount = pSkeleton->GetBoneCount();
    fileHeader.BoneCount = outputBones.GetSize();
    fileHeader.BoneRefCount = outputBoneRefs.GetSize();
    fileHeader.VertexCount = outputVertices.GetSize();
    fileHeader.IndexCount = outputIndices.GetSize();
    fileHeader.BatchCount = outputBatches.GetSize();
    fileHeader.CollisionShapeType = (m_pCollisionShapeGenerator != nullptr) ? m_pCollisionShapeGenerator->GetType() : Physics::COLLISION_SHAPE_TYPE_NONE;

    // write header
    pStream->Write2(&fileHeader, sizeof(fileHeader));

    // initialize chunk writer
    ChunkFileWriter cfw;
    if (!cfw.Initialize(pStream, DF_SKELETALMESH_CHUNK_COUNT))
        return false;

    // write skeleton
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_SKELETON);
    {
        DF_SKELETALMESH_SKELETON fileSkeleton;
        fileSkeleton.SkeletonNameStringIndex = cfw.AddString(m_skeletonName);
        cfw.WriteChunkData(&fileSkeleton, sizeof(fileSkeleton));
    }
    cfw.EndChunk();

    // write bones
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_BONES);
    cfw.WriteChunkData(outputBones.GetBasePointer(), outputBones.GetStorageSizeInBytes());
    cfw.EndChunk();

    // write bone refs
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_BONE_REFS);
    cfw.WriteChunkData(outputBoneRefs.GetBasePointer(), outputBoneRefs.GetStorageSizeInBytes());
    cfw.EndChunk();

    // write materials
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_MATERIALS);
    {
        for (uint32 i = 0; i < m_materialNames.GetSize(); i++)
        {
            DF_SKELETALMESH_MATERIAL fileMaterial;
            fileMaterial.MaterialNameStringIndex = cfw.AddString(m_materialNames[i]);
            cfw.WriteChunkData(&fileMaterial, sizeof(fileMaterial));
        }
    }
    cfw.EndChunk();

    // write vertices
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_VERTICES);
    cfw.WriteChunkData(outputVertices.GetBasePointer(), outputVertices.GetStorageSizeInBytes());
    cfw.EndChunk();

    // write indices
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_INDICES);
    cfw.WriteChunkData(outputIndices.GetBasePointer(), outputIndices.GetStorageSizeInBytes());
    cfw.EndChunk();
    
    // write batches
    cfw.BeginChunk(DF_SKELETALMESH_CHUNK_BATCHES);
    cfw.WriteChunkData(outputBatches.GetBasePointer(), outputBatches.GetStorageSizeInBytes());
    cfw.EndChunk();

    // for collision shape:
    if (m_pCollisionShapeGenerator != nullptr)
    {
        // transform it with the root bone transform
        Physics::CollisionShapeGenerator *pCollisionShapeGeneratorCopy = new Physics::CollisionShapeGenerator();

        // bit of a hack, but ehh.. for box/sphere we use the post-skeleton-transformed vertices
        if (m_pCollisionShapeGenerator->GetType() == Physics::COLLISION_SHAPE_TYPE_BOX)
        {
            pCollisionShapeGeneratorCopy->SetType(Physics::COLLISION_SHAPE_TYPE_BOX);
            pCollisionShapeGeneratorCopy->SetBoxCenter(boundingBox.GetCenter());
            pCollisionShapeGeneratorCopy->SetBoxHalfExtents(boundingBox.GetExtents() / 2.0f);
        }
        else if (m_pCollisionShapeGenerator->GetType() == Physics::COLLISION_SHAPE_TYPE_SPHERE)
        {
            pCollisionShapeGeneratorCopy->SetType(Physics::COLLISION_SHAPE_TYPE_SPHERE);
            pCollisionShapeGeneratorCopy->SetSphereCenter(boundingSphere.GetCenter());
            pCollisionShapeGeneratorCopy->SetSphereRadius(boundingSphere.GetRadius());
        }
        else
        {
            // transform the shape
            pCollisionShapeGeneratorCopy->Copy(m_pCollisionShapeGenerator);
            pCollisionShapeGeneratorCopy->ApplyTransform(pSkeleton->GetRootBone()->GetAbsoluteBaseFrameTransform().GetTransformMatrix4x4());
        }

        // compile it
        BinaryBlob *pCollisionShapeBlob;
        if (!pCollisionShapeGeneratorCopy->Compile(&pCollisionShapeBlob))
            return false;

        // write it
        cfw.BeginChunk(DF_SKELETALMESH_CHUNK_COLLISION_SHAPE);
        cfw.WriteChunkData(pCollisionShapeBlob->GetDataPointer(), pCollisionShapeBlob->GetDataSize());
        cfw.EndChunk();
        pCollisionShapeBlob->Release();
    }

    if (!cfw.Close())
        return false;

    return !(pStream->InErrorState());
}

// Interface
BinaryBlob *ResourceCompiler::CompileSkeletalMesh(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.skm.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileSkeletalMesh: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    SkeletalMeshGenerator *pGenerator = new SkeletalMeshGenerator();
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
    if (!pGenerator->Compile(pCallbacks, pOutputStream))
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
