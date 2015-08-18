#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpCommon.h"
#include "ContentConverter/AssimpSceneImporter.h"
#include "ContentConverter/AssimpMaterialImporter.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
#include "MapCompiler/MapSource.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"

using AssimpHelpers::AssimpVector3ToFloat3;
using AssimpHelpers::AssimpQuaternionToQuaternion;
using AssimpHelpers::Float4x4ToAssimpMatrix4x4;

AssimpSceneImporter::AssimpSceneImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(nullptr),
      m_pLogStream(nullptr),
      m_pScene(nullptr)
{

}

AssimpSceneImporter::~AssimpSceneImporter()
{
    delete m_pLogStream;
    delete m_pImporter;

    for (uint32 i = 0; i < m_meshes.GetSize(); i++)
        delete m_meshes[i].pGenerator;
}

void AssimpSceneImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->ImportMaterials = false;
    pOptions->DefaultMaterialName = "materials/engine/default";
    pOptions->BuildCollisionShapeType = StaticMeshGenerator::CollisionShapeType_TriangleMesh;
    pOptions->TransformMatrix.SetIdentity();
    pOptions->CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
    pOptions->CenterMeshes = StaticMeshGenerator::CenterOrigin_Count;
}

bool AssimpSceneImporter::Execute()
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

    timer.Reset();
    if (!LoadScene())
    {
        m_pProgressCallbacks->ModalError("LoadScene() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("LoadScene(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!CreateMaterials())
    {
        m_pProgressCallbacks->ModalError("CreateMaterials() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateMaterials(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!ParseScene())
    {
        m_pProgressCallbacks->ModalError("ParseScene() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("ParseScene(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!WriteMeshes())
    {
        m_pProgressCallbacks->ModalError("WriteMeshes() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("WriteMeshes(): %.4f ms", timer.GetTimeMilliseconds());

    if (m_pOptions->OutputMapName.GetLength() > 0)
    {
        timer.Reset();
        if (!WriteMap())
        {
            m_pProgressCallbacks->ModalError("WriteMap() failed. The log may contain more information as to why.");
            return false;
        }
        m_pProgressCallbacks->DisplayFormattedInformation("WriteMap(): %.4f ms", timer.GetTimeMilliseconds());
    }

    return true;
}

bool AssimpSceneImporter::LoadScene()
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

    // get postprocessing flags
    int postProcessFlags = AssimpHelpers::GetAssimpStaticMeshImportPostProcessingFlags();

    // remove crap
    postProcessFlags &= ~(aiProcess_OptimizeMeshes);
    postProcessFlags &= ~(aiProcess_OptimizeGraph);
    //postProcessFlags &= ~(aiProcess_FlipUVs);
    postProcessFlags |= aiProcess_FindInstances;

    // get coordinate system transform
    bool reverseWinding;
    float4x4 coordinateSystemTransform(float4x4::MakeCoordinateSystemConversionMatrix(m_pOptions->CoordinateSystem, COORDINATE_SYSTEM_Z_UP_RH, &reverseWinding));
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

bool AssimpSceneImporter::CreateMaterials()
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
        if (aiGetMaterialString(pAIMaterial, AI_MATKEY_NAME, &aiMaterialName) != aiReturn_SUCCESS || aiMaterialName.length == 0)
            aiMaterialName.Set(String::FromFormat("material%u", i));

        SmallString sanitizedMaterialName = aiMaterialName.C_Str();
        Resource::SanitizeResourceName(sanitizedMaterialName);

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

bool AssimpSceneImporter::ParseScene()
{
    const aiNode *pRootNode = m_pScene->mRootNode;

    // parse the root node
    aiMatrix4x4 rootTransform(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    return ParseNode(pRootNode, &rootTransform);
}

bool AssimpSceneImporter::ParseNode(const aiNode *pNode, const void *parentTransform)
{
    // create transform for this node
    aiMatrix4x4 nodeTransform(*reinterpret_cast<const aiMatrix4x4 *>(parentTransform) * pNode->mTransformation);

    // parse all meshes in this node
    if (pNode->mNumMeshes > 0)
    {
        // is this mesh already done?
        uint32 meshIndex;
        for (meshIndex = 0; meshIndex < m_meshes.GetSize(); meshIndex++)
        {
            if (m_meshes[meshIndex].nSourceMeshes == pNode->mNumMeshes &&
                Y_memcmp(m_meshes[meshIndex].pSourceMeshes, pNode->mMeshes, sizeof(unsigned int) * pNode->mNumMeshes) == 0)
            {
                // mesh set matches
                break;
            }
        }
        
        // mesh in list?
        if (meshIndex == m_meshes.GetSize())
        {
            uint32 meshAddSuffix = 0;
            String meshName;

            for (;;)
            {
                // create mesh name
                if (pNode->mName.length != 0)
                    meshName.AppendString(pNode->mName.C_Str());
                else if (m_pScene->mMeshes[pNode->mMeshes[0]]->mName.length != 0)
                    meshName.AppendString(m_pScene->mMeshes[pNode->mMeshes[0]]->mName.C_Str());
                else
                    meshName.AppendString("node");

                // append suffix
                if (meshAddSuffix != 0)
                    meshName.AppendFormattedString("_%u", meshAddSuffix);

                // sanitize it
                Resource::SanitizeResourceName(meshName);

                // add mesh prefix
                meshName.PrependFormattedString("%s/%s", m_pOptions->MeshDirectory.GetCharArray(), m_pOptions->MeshPrefix.GetCharArray());

                // do we have anything with this name alreadY?
                for (meshIndex = 0; meshIndex < m_meshes.GetSize(); meshIndex++)
                {
                    if (m_meshes[meshIndex].MeshName.CompareInsensitive(meshName))
                        break;
                }
                if (meshIndex != m_meshes.GetSize())
                {
                    // name already in use
                    meshAddSuffix++;
                    meshName.Clear();
                    continue;
                }

                // use this name
                break;
            }

            // have to generate it
            StaticMeshGenerator *pGenerator = new StaticMeshGenerator();
            uint32 lodIndex = pGenerator->AddLOD();

            // parse each mesh
            for (uint32 subMeshIndex = 0; subMeshIndex < pNode->mNumMeshes; subMeshIndex++)
            {
                const aiMesh *pMesh = m_pScene->mMeshes[pNode->mMeshes[subMeshIndex]];
                if (pMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
                {
                    m_pProgressCallbacks->DisplayFormattedWarning("Skipping non-triangle mesh %u", subMeshIndex);
                    continue;
                }

                if (pMesh->mNumVertices == 0 || pMesh->mNumFaces == 0)
                {
                    m_pProgressCallbacks->DisplayFormattedWarning("Skipping empty mesh %u", subMeshIndex);
                    continue;
                }

                if (!ParseMesh(pMesh, pGenerator, lodIndex, meshName))
                {
                    delete pGenerator;
                    return false;
                }
            }

            // store it
            OutputMesh mesh;
            mesh.pSourceMeshes = pNode->mMeshes;
            mesh.nSourceMeshes = pNode->mNumMeshes;
            mesh.MeshName = meshName;
            mesh.pGenerator = pGenerator;
            mesh.Offset.SetZero();

            // center mesh
            if (m_pOptions->CenterMeshes != StaticMeshGenerator::CenterOrigin_Count)
                pGenerator->CenterMesh((StaticMeshGenerator::CenterOrigin)m_pOptions->CenterMeshes, &mesh.Offset);

            // finalize mesh
            if (!FinalizeMesh(pGenerator, meshName))
            {
                delete pGenerator;
                return false;
            }

            // add to list
            m_meshes.Add(mesh);
        }

        const OutputMesh &outputMesh = m_meshes[meshIndex];

        // get mesh translation matrix
        aiMatrix4x4 meshTranslation;
        aiMatrix4x4::Translation(aiVector3D(outputMesh.Offset.x, outputMesh.Offset.y, outputMesh.Offset.z), meshTranslation);

        // work out transform - remove the global transform first, remove the mesh offset, move it into place, and reapply global transform
        aiMatrix4x4 newNodeTransform = meshTranslation.Inverse() *
                                       Float4x4ToAssimpMatrix4x4(m_globalTransform) *
                                       nodeTransform *
                                       //meshTranslation *
                                       Float4x4ToAssimpMatrix4x4(m_globalTransform.Inverse()) * meshTranslation;

        // decompose transform
        aiVector3D dcPosition;
        aiQuaternion dcRotation;
        aiVector3D dcScale;
        newNodeTransform.Decompose(dcScale, dcRotation, dcPosition);

        // create the instance
        OutputMeshInstance meshInstance;
        meshInstance.MeshIndex = meshIndex;
        meshInstance.Position = AssimpVector3ToFloat3(dcPosition);// -outputMesh.Offset;
        meshInstance.Rotation = AssimpQuaternionToQuaternion(dcRotation);
        meshInstance.Scale = AssimpVector3ToFloat3(dcScale);
        m_meshInstances.Add(meshInstance);

        m_pProgressCallbacks->DisplayFormattedInformation("NNR: %f %f %f %f", dcRotation.x, dcRotation.y, dcRotation.z, dcRotation.w);
    }

    // parse node children
    for (uint32 i = 0; i < pNode->mNumChildren; i++)
    {
        if (!ParseNode(pNode->mChildren[i], &nodeTransform))
            return false;
    }

    // ok
    return true;
}

bool AssimpSceneImporter::ParseMesh(const aiMesh *pMesh, StaticMeshGenerator *pOutputGenerator, uint32 lodIndex, const char *meshName)
{
    // CS transform
    //const aiMatrix4x4 CSTransformMatrix(AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4());
    
    // for each mesh...
    //uint64 smoothingGroupMask = (1 << 0);

    // create output mesh, using max nweights for now
    //m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] using material %s", meshName, m_pOutputGenerator->GetMaterialNameByIndex(m_materialMapping[pMesh->mMaterialIndex]).GetCharArray());

    // set mesh flags
    if (pMesh->GetNumUVChannels() > 0)
        pOutputGenerator->SetVertexTextureCoordinatesEnabled(true);
    if (pMesh->GetNumColorChannels() > 0)
        pOutputGenerator->SetVertexColorsEnabled(true);

    // for this mesh
    PODArray<uint32> meshVertexIndices;

    // create the batch
    DebugAssert(pMesh->mMaterialIndex < m_materialMapping.GetSize());
    uint32 batchIndex = pOutputGenerator->AddBatch(lodIndex, m_materialMapping[pMesh->mMaterialIndex]);

    // create vertices without any weights in them
    for (uint32 vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
    {
        float3 vertexPosition = m_globalTransform.TransformPoint(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mVertices[vertexIndex]));
        //float3 vertexPosition = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mVertices[vertexIndex]));
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

        pOutputGenerator->AddTriangle(lodIndex, batchIndex, meshVertexIndices[pFace->mIndices[0]], meshVertexIndices[pFace->mIndices[1]], meshVertexIndices[pFace->mIndices[2]]);
    }

    m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] added %u faces/triangles", meshName, pMesh->mNumFaces);
    return true;
}

bool AssimpSceneImporter::FinalizeMesh(StaticMeshGenerator *pOutputGenerator, const char *meshName)
{   
    if (!pOutputGenerator->IsCompleteMesh())
        return false;

    pOutputGenerator->GenerateTangents(0);
    pOutputGenerator->CalculateBounds();

    // build collision shape
    if (m_pOptions->BuildCollisionShapeType != StaticMeshGenerator::CollisionShapeType_Count)
    {
        // build box
        m_pProgressCallbacks->DisplayFormattedInformation("Generating collision shape...");
        if (!pOutputGenerator->BuildCollisionShape((StaticMeshGenerator::CollisionShapeType)m_pOptions->BuildCollisionShapeType))
        {
            m_pProgressCallbacks->DisplayError("Failed to generate collision shape.");
            return false;
        }
    }
    
    return true;
}

bool AssimpSceneImporter::WriteMeshes()
{
    PathString outputResourceFileName;

    // write meshes
    for (uint32 meshIndex = 0; meshIndex < m_meshes.GetSize(); meshIndex++)
    {
        outputResourceFileName.Format("%s.staticmesh.xml", m_meshes[meshIndex].MeshName.GetCharArray());

        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputResourceFileName,
                                                             BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | 
                                                             BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | 
                                                             BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
        if (pStream == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("could not open file '%s'", outputResourceFileName.GetCharArray());
            return false;
        }

        if (!m_meshes[meshIndex].pGenerator->SaveToXML(pStream))
        {
            m_pProgressCallbacks->DisplayError("failed to save xml file");
            pStream->Discard();
            pStream->Release();
            return false;
        }

        pStream->Commit();
        pStream->Release();
    }

    return true;
}

bool AssimpSceneImporter::WriteMap()
{
    // get template for StaticMeshBrush
    const ObjectTemplate *pStaticMeshTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("StaticMeshBrush");
    if (pStaticMeshTemplate == nullptr)
    {
        m_pProgressCallbacks->DisplayError("Failed to get StaticMeshBrush template.");
        return false;
    }

    // create map
    MapSource *pMapSource = new MapSource();
    pMapSource->Create();

    // create entities
    for (uint32 i = 0; i < m_meshInstances.GetSize(); i++)
    {
        // get mesh
        const OutputMeshInstance &meshInstance = m_meshInstances[i];
        const OutputMesh &mesh = m_meshes[meshInstance.MeshIndex];

        // create entity
        MapSourceEntityData *pEntityData = pMapSource->CreateEntityFromTemplate(pStaticMeshTemplate);
        DebugAssert(pEntityData != nullptr);

        // set mesh name property
        pEntityData->SetPropertyValue("StaticMeshName", mesh.MeshName);

        // work out the transform
        float3 offsetPosition(meshInstance.Position + mesh.Offset);
        pEntityData->SetPropertyValueFloat3("Position", offsetPosition);
        pEntityData->SetPropertyValueQuaternion("Rotation", meshInstance.Rotation);
        pEntityData->SetPropertyValueFloat3("Scale", meshInstance.Scale);
    }

    // write out the map
    bool result = pMapSource->SaveAs(m_pOptions->OutputMapName, m_pProgressCallbacks);
    delete pMapSource;
    return result;
}

