#pragma once
#include "ContentConverter/BaseImporter.h"
#include "ContentConverter/TextureImporter.h"

#define OBJIMPORTER_MAX_VERTICES_PER_FACE 16

struct OBJImporterOptions
{
    String InputFileName;
    String MeshDirectory;
    String MeshPrefix;
    bool ImportMaterials;
    String MaterialDirectory;
    String MaterialPrefix;
    String DefaultMaterialName;
    String BuildCollisionShapeType;
    bool UseSmoothingGroups;
    float4x4 TransformMatrix;
    COORDINATE_SYSTEM CoordinateSystem;
    bool FlipWinding;
    uint32 CenterMeshes;
    bool MergeGroups;
    String OutputMapName;
};

class OBJImporter : public BaseImporter
{
public:
    OBJImporter(ProgressCallbacks *pProgressCallbacks);
    ~OBJImporter();

    static void SetDefaultOptions(OBJImporterOptions *pOptions);

    bool Execute(const OBJImporterOptions *pOptions);

private:
    const OBJImporterOptions *m_pOptions;

    //-------------------------------------------------------------------------
    typedef MemArray<float3> SourceVertexArray;
    typedef MemArray<float2> SourceTexCoordArray;
    typedef PODArray<String *> SourceMaterialLibraryArray;
    typedef PODArray<String *> SourceMaterialNameArray;    

    struct SourceFace
    {
        int32 VertexIndicies[OBJIMPORTER_MAX_VERTICES_PER_FACE];
        int32 TexCoordIndices[OBJIMPORTER_MAX_VERTICES_PER_FACE];
        uint32 NumVertices;
        uint32 SmoothingGroups;
        uint32 MaterialIndex;
    };

    typedef MemArray<SourceFace> SourceFaceArray;

    struct SourceGroup
    {
        String Name;
        SourceFaceArray Faces;
    };

    typedef PODArray<SourceGroup *> SourceGroupArray;

    //
    SourceVertexArray m_arrSourceVertices;
    SourceTexCoordArray m_arrSourceTexCoords;
    SourceMaterialLibraryArray m_arrSourceMaterialLibraries;
    SourceMaterialNameArray m_arrSourceMaterialNames;
    SourceGroupArray m_arrSourceGroups;
    uint32 m_uSourceLineNumber;

    //    
    bool ParseSource();
    void PrintSourceError(const char *Format, ...);
    void PrintSourceWarning(const char *Format, ...);

    //-------------------------------------------------------------------------
    struct SourceMaterial
    {
        String Name;
        String OutputName;
        float3 AmbientColor;
        float3 DiffuseColor;
        float3 SpecularColor;
        float Shininess;
        float3 EmissiveColor;
        float AlphaColor;
        uint32 Illum;
        String AmbientMap;
        String DiffuseMap;
        String SpecularMap;
        String AlphaMap;
        String BumpMap;
    };

    //
    typedef PODArray<SourceMaterial *> SourceMaterialArray;
    SourceMaterialArray m_arrSourceMaterials;

    //
    bool ParseMaterials();
    void PrintMaterialError(uint32 i, const char *Format, ...);
    void PrintMaterialWarning(uint32 i, const char *Format, ...);
    bool ImportMaterialTexture(const String &inputFileName, String &outputFullName, TEXTURE_USAGE usage, const String &maskTexture = EmptyString);
    bool GenerateMaterials();

    //-------------------------------------------------------------------------
    struct OutputVertex
    {
        float3 Position;
        float2 TexCoord;
    };
    struct OutputTriangle
    {
        uint32 Indices[3];
        uint32 MaterialIndex;
    };

    typedef MemArray<OutputVertex> OutputVertexArray;
    typedef MemArray<OutputTriangle> OutputTriangleArray;
    typedef Array<String> OutputMaterialArray;

    struct OutputMesh
    {
        String Name;
        OutputVertexArray Vertices;
        OutputTriangleArray Triangles;
        float3 Translation;
    };

    typedef PODArray<OutputMesh *> OutputMeshArray;
    OutputMeshArray m_arrOutputMeshes;
    OutputMaterialArray m_arrOutputMaterials;

    //
    void MergeGroups();
    bool GenerateTriangles();
    bool WriteOutput();

    //
    bool WriteMap();
};

