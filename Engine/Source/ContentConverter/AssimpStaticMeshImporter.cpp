#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpCommon.h"
#include "ContentConverter/AssimpStaticMeshImporter.h"
#include "ContentConverter/AssimpMaterialImporter.h"
#include "ResourceCompiler/StaticMeshGenerator.h"

AssimpStaticMeshImporter::AssimpStaticMeshImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(NULL),
      m_pLogStream(NULL),
      m_pScene(NULL),
      m_globalTransform(float4x4::Identity),
      m_pOutputGenerator(NULL)
{

}

AssimpStaticMeshImporter::~AssimpStaticMeshImporter()
{
    delete m_pOutputGenerator;
    delete m_pLogStream;
    delete m_pImporter;
}

void AssimpStaticMeshImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->ImportMaterials = false;
    pOptions->DefaultMaterialName = "materials/engine/default";
    pOptions->BuildCollisionShapeType = StaticMeshGenerator::CollisionShapeType_TriangleMesh;
    pOptions->CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
    pOptions->FlipFaceOrder = false;
    pOptions->TransformMatrix.SetIdentity();
}

bool AssimpStaticMeshImporter::GetFileSubMeshNames(const char *filename, Array<String> &meshNames, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // create temporary log callback
    AssimpHelpers::ProgressCallbacksLogStream logCallback(pProgressCallbacks);

    // import the file
    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile(filename, 0);
    if (pScene == nullptr)
    {
        pProgressCallbacks->DisplayFormattedError("Failed to parse file '%s'", filename);
        return false;
    }

    // post process the scene, but only to reduce the amount of meshes
    if (!importer.ApplyPostProcessing(aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph))
    {
        pProgressCallbacks->DisplayFormattedError("Failed to postprocess file '%s'", filename);
        return false;
    }

    // get the mesh names
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
    {
        const aiMesh *pMesh = pScene->mMeshes[i];
        if (pMesh->mName.length == 0)
            meshNames.Add(String::FromFormat("* Unnamed Mesh %u", i));
        else
            meshNames.Add(pMesh->mName.C_Str());
    }

    return true;    
}

bool AssimpStaticMeshImporter::Execute()
{
    Timer timer;

    if (m_pOptions->SourcePath.IsEmpty() ||
        m_pOptions->DefaultMaterialName.IsEmpty() ||
        (m_pOptions->ImportMaterials && m_pOptions->MaterialDirectory.IsEmpty()))
    {
        m_pProgressCallbacks->ModalError("Missing parameters");
        return false;
    }

    // ensure assimp is initialized
    if (!AssimpHelpers::InitializeAssimp())
    {
        m_pProgressCallbacks->ModalError("assimp initialization failed");
        return false;
    }

    // determine base output path
    {
        int32 lastSlashPosition = m_pOptions->OutputResourceName.RFind('/');
        if (lastSlashPosition >= 0)
            m_baseOutputPath = m_pOptions->OutputResourceName.SubString(0, lastSlashPosition);
    }

    timer.Reset();
    if (!LoadScene())
    {
        m_pProgressCallbacks->ModalError("LoadScene() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("LoadScene(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!CreateGenerator())
    {
        m_pProgressCallbacks->ModalError("CreateGenerator() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateGenerator(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!CreateMaterials())
    {
        m_pProgressCallbacks->ModalError("CreateMaterials() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateMaterials(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!CreateMesh())
    {
        m_pProgressCallbacks->ModalError("CreateMesh() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateMesh(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!PostProcess())
    {
        m_pProgressCallbacks->ModalError("PostProcess() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("PostProcess(): %.4f ms", timer.GetTimeMilliseconds());

    if (m_pOptions->OutputResourceName.GetLength() > 0)
    {
        timer.Reset();
        if (!WriteOutput())
        {
            m_pProgressCallbacks->ModalError("WriteOutput() failed. The log may contain more information as to why.");
            return false;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("WriteOutput(): %.4f ms", timer.GetTimeMilliseconds());
    }

    return true;
}

bool AssimpStaticMeshImporter::LoadScene()
{
    // create importer
    m_pImporter = new Assimp::Importer();

    // hook up log interface
    m_pLogStream = new AssimpHelpers::ProgressCallbacksLogStream(m_pProgressCallbacks);

    // pass filename to assimp
    m_pScene = m_pImporter->ReadFile(m_pOptions->SourcePath, 0);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ReadFile failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // get post process flags
    uint32 postProcessFlags = AssimpHelpers::GetAssimpStaticMeshImportPostProcessingFlags();

    // get coordinate system transform
    bool reverseWinding;
    float4x4 coordinateSystemTransform(float4x4::MakeCoordinateSystemConversionMatrix(m_pOptions->CoordinateSystem, COORDINATE_SYSTEM_Z_UP_RH, &reverseWinding));
    if (m_pOptions->FlipFaceOrder)
        reverseWinding = !reverseWinding;
    if (reverseWinding)
        postProcessFlags |= aiProcess_FlipWindingOrder;

    // create global transform
    m_globalTransform = m_pOptions->TransformMatrix * coordinateSystemTransform;

    // apply postprocessing
    m_pScene = m_pImporter->ApplyPostProcessing(postProcessFlags);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ApplyPostProcessing failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // display info
    m_pProgressCallbacks->DisplayFormattedInformation("Scene info: %u animations, %u camera, %u lights, %u materials, %u meshes, %u textures", m_pScene->mNumAnimations, m_pScene->mNumCameras, m_pScene->mNumLights, m_pScene->mNumMaterials, m_pScene->mNumMeshes, m_pScene->mNumTextures);
    
    // ok
    return true;
}

bool AssimpStaticMeshImporter::CreateGenerator()
{
    bool hasTextureCoordinates = false;
    bool hasVertexColors = false;

    // determine whether there are any texture coordinates, vertex colors
    for (uint32 i = 0; i < m_pScene->mNumMeshes; i++)
    {
        if (m_pScene->mMeshes[i]->GetNumUVChannels() > 0)
            hasTextureCoordinates = true;

        if (m_pScene->mMeshes[i]->GetNumColorChannels() > 0)
            hasVertexColors = true;
    }

    // create generator
    m_pOutputGenerator = new StaticMeshGenerator();
    m_pOutputGenerator->Create(hasTextureCoordinates, hasVertexColors);

    // add level zero lod
    m_pOutputGenerator->AddLOD();
    return true;
}

bool AssimpStaticMeshImporter::CreateMaterials()
{
    if (!m_pOptions->ImportMaterials)
    {
        for (uint32 i = 0; i < m_pScene->mNumMaterials; i++)
        {
            m_materialMapping.Add(m_pOptions->DefaultMaterialName);
        }

        return true;
    }
    
    for (uint32 i = 0; i < m_pScene->mNumMaterials; i++)
    {
        const aiMaterial *pAIMaterial = m_pScene->mMaterials[i];

        aiString aiMaterialName;
        if (!aiGetMaterialString(pAIMaterial, AI_MATKEY_NAME, &aiMaterialName) || aiMaterialName.length == 0)
            aiMaterialName.Set(String::FromFormat("material%u", i));

        SmallString sanitizedMaterialName;
        FileSystem::SanitizeFileName(sanitizedMaterialName, aiMaterialName.C_Str());

        PathString materialResourceName;
        PathString fileName;
        materialResourceName.Format("%s/%s%s", m_pOptions->MaterialDirectory.GetCharArray(), m_pOptions->MaterialPrefix.GetCharArray(), sanitizedMaterialName.GetCharArray());

        m_pProgressCallbacks->DisplayFormattedInformation("Material '%s' -> '%s'", aiMaterialName.C_Str(), materialResourceName.GetCharArray());

        // does this material exist?
        fileName.Format("%s.mtl.xml", materialResourceName.GetCharArray());
        if (g_pVirtualFileSystem->FileExists(fileName))
        {
            m_pProgressCallbacks->DisplayFormattedInformation("Material '%s' already exists, skipping import...", materialResourceName.GetCharArray());
            m_materialMapping.Add(materialResourceName);
            continue;
        }

        // import the material
        if (!AssimpMaterialImporter::ImportMaterial(pAIMaterial, m_pOptions->SourcePath, m_pOptions->MaterialDirectory, m_pOptions->MaterialPrefix, materialResourceName, m_pProgressCallbacks))
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Material '%s' failed import, using default material.", materialResourceName.GetCharArray());
            m_materialMapping.Add(m_pOptions->DefaultMaterialName);
            continue;
        }

        // add it
        m_materialMapping.Add(materialResourceName);
    }

    return true;
}

bool AssimpStaticMeshImporter::CreateMesh()
{
    // todo: global transform
    //const aiNode *pRootNode = m_pScene->mRootNode;

    // parse all meshes
    for (uint32 i = 0; i < m_pScene->mNumMeshes; i++)
    {
        const aiMesh *pMesh = m_pScene->mMeshes[i];
        if (pMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Skipping non-triangle mesh %u", i);
            continue;
        }

        if (pMesh->mNumVertices == 0 || pMesh->mNumFaces == 0)
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Skipping empty mesh %u", i);
            continue;
        }

        if (!ParseMesh(pMesh, m_pOutputGenerator, 0, pMesh->mName.C_Str()))
            return false;
    }

    // ok
    return true;
}

bool AssimpStaticMeshImporter::ParseMesh(const aiMesh *pMesh, StaticMeshGenerator *pOutputGenerator, uint32 lodIndex, const char *meshName)
{
    // for each mesh...
    //uint64 smoothingGroupMask = (1 << 0);

    // create output mesh, using max nweights for now
    m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] using material %s", meshName, m_materialMapping[pMesh->mMaterialIndex].GetCharArray());

    // for this mesh
    MemArray<uint32> meshVertexIndices;

    // create batch
    uint32 batchIndex = pOutputGenerator->AddBatch(lodIndex, m_materialMapping[pMesh->mMaterialIndex]);

    // create vertices without any weights in them
    for (uint32 vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
    {
        float3 vertexPosition = m_globalTransform.TransformPoint(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mVertices[vertexIndex]));
        float2 vertexTextureCoordinates = ((pMesh->GetNumUVChannels() > 0) ? AssimpHelpers::AssimpVector3ToFloat3(pMesh->mTextureCoords[0][vertexIndex]).xy() : float2::Zero);
        uint32 vertexColor = ((pMesh->GetNumColorChannels() > 0) ? AssimpHelpers::AssimpColor4ToColor(pMesh->mColors[0][vertexIndex]) : MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));

        float3 vertexTangent;
        float3 vertexBinormal;
        float3 vertexNormal;

        if (pMesh->HasTangentsAndBitangents())
        {
            vertexTangent = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mTangents[vertexIndex]));
            vertexBinormal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mBitangents[vertexIndex]));
            vertexNormal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
        }
        else if (pMesh->HasNormals())
        {
            vertexTangent = float3::UnitX;
            vertexBinormal = float3::UnitY;
            vertexNormal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
        }
        else
        {
            vertexTangent = float3::UnitX;
            vertexBinormal = float3::UnitY;
            vertexNormal = float3::UnitZ;
        }

        // add to list
        uint32 outVertexIndex = pOutputGenerator->AddVertex(lodIndex, vertexPosition, vertexTangent, vertexBinormal, vertexNormal, vertexTextureCoordinates, vertexColor);
        meshVertexIndices.Add(outVertexIndex);
    }
    m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] added %u vertices", meshName, pMesh->mNumVertices);
    
    // read triangles
    for (uint32 faceIndex = 0; faceIndex < pMesh->mNumFaces; faceIndex++)
    {
        const aiFace *pFace = &pMesh->mFaces[faceIndex];
        if (pFace->mNumIndices != 3)
        {
            m_pProgressCallbacks->DisplayFormattedError("in mesh %s: face %u does not have 3 indices", meshName, faceIndex);
            return false;
        }

        DebugAssert(pFace->mIndices[0] < meshVertexIndices.GetSize() &&
                    pFace->mIndices[1] < meshVertexIndices.GetSize() &&
                    pFace->mIndices[2] < meshVertexIndices.GetSize());

        m_pOutputGenerator->AddTriangle(lodIndex, batchIndex, meshVertexIndices[pFace->mIndices[0]], meshVertexIndices[pFace->mIndices[1]], meshVertexIndices[pFace->mIndices[2]]);
    }
    m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] added %u faces/triangles", meshName, pMesh->mNumFaces);
   
    return true;
}

bool AssimpStaticMeshImporter::PostProcess()
{
    m_pOutputGenerator->GenerateTangents(0);
    m_pOutputGenerator->CalculateBounds();

    // build collision shape
    if (m_pOptions->BuildCollisionShapeType != StaticMeshGenerator::CollisionShapeType_Count)
    {
        // build box
        m_pProgressCallbacks->DisplayFormattedInformation("Generating collision shape...");
        if (!m_pOutputGenerator->BuildCollisionShape((StaticMeshGenerator::CollisionShapeType)m_pOptions->BuildCollisionShapeType))
        {
            m_pProgressCallbacks->DisplayError("Failed to generate collision shape.");
            return false;
        }
    }

    return true;
}

bool AssimpStaticMeshImporter::WriteOutput()
{
    PathString outputResourceFileName;
    outputResourceFileName.Format("%s.staticmesh.xml", m_pOptions->OutputResourceName.GetCharArray());

    ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputResourceFileName,
                                                         BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | 
                                                         BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | 
                                                         BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
    {
        m_pProgressCallbacks->DisplayFormattedError("could not open file '%s'", outputResourceFileName.GetCharArray());
        return false;
    }

    if (!m_pOutputGenerator->SaveToXML(pStream))
    {
        m_pProgressCallbacks->DisplayError("failed to save xml file");
        pStream->Discard();
        pStream->Release();
        return false;
    }

    pStream->Commit();
    pStream->Release();
    return true;
}

StaticMeshGenerator *AssimpStaticMeshImporter::DetachOutputGenerator()
{
    StaticMeshGenerator *pGenerator = m_pOutputGenerator;
    m_pOutputGenerator = nullptr;
    return pGenerator;
}

