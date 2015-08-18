#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/AssimpSkeletalMeshImporter.h"
#include "ContentConverter/AssimpMaterialImporter.h"
#include "ContentConverter/AssimpCommon.h"
#include "ResourceCompiler/SkeletalMeshGenerator.h"

AssimpSkeletalMeshImporter::AssimpSkeletalMeshImporter(const Options *pOptions, ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks),
      m_pOptions(pOptions),
      m_pImporter(NULL),
      m_pLogStream(NULL),
      m_pScene(NULL),
      m_pOutputGenerator(NULL)
{

}

AssimpSkeletalMeshImporter::~AssimpSkeletalMeshImporter()
{
    delete m_pOutputGenerator;
    delete m_pLogStream;
    delete m_pImporter;
}

void AssimpSkeletalMeshImporter::SetDefaultOptions(Options *pOptions)
{
    pOptions->ImportMaterials = false;
    pOptions->DefaultMaterialName = "materials/engine/default";
    pOptions->CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
    pOptions->FlipFaceOrder = false;
    pOptions->CollisionShapeType = "Box";
}

bool AssimpSkeletalMeshImporter::Execute()
{
    Timer timer;

    if (m_pOptions->SourcePath.IsEmpty() ||
        m_pOptions->SkeletonName.IsEmpty() || 
        m_pOptions->OutputResourceName.IsEmpty() ||
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

    timer.Reset();
    if (!CreateCollisionShape())
    {
        m_pProgressCallbacks->ModalError("CreateCollisionShape() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("CreateCollisionShape(): %.4f ms", timer.GetTimeMilliseconds());

    timer.Reset();
    if (!WriteOutput())
    {
        m_pProgressCallbacks->ModalError("WriteOutput() failed. The log may contain more information as to why.");
        return false;
    }
    m_pProgressCallbacks->DisplayFormattedInformation("WriteOutput(): %.4f ms", timer.GetTimeMilliseconds());

    return true;
}

bool AssimpSkeletalMeshImporter::LoadScene()
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
    uint32 postProcessFlags = AssimpHelpers::GetAssimpSkeletalMeshImportPostProcessingFlags();

    // get coordinate system transform
    bool reverseWinding;
    float4x4::MakeCoordinateSystemConversionMatrix(m_pOptions->CoordinateSystem, COORDINATE_SYSTEM_Z_UP_RH, &reverseWinding);
    if (m_pOptions->FlipFaceOrder)
        reverseWinding = !reverseWinding;
    if (reverseWinding)
        postProcessFlags |= aiProcess_FlipWindingOrder;

//     // create global transform
//     m_globalTransform = m_pOptions->TransformMatrix * coordinateSystemTransform;
//     m_inverseGlobalTransform = m_globalTransform.Inverse();

    // apply postprocessing
    m_pScene = m_pImporter->ApplyPostProcessing(postProcessFlags);
    if (m_pScene == NULL)
    {
        const char *errorMessage = m_pImporter->GetErrorString();
        m_pProgressCallbacks->DisplayFormattedError("Importer::ApplyPostProcessing failed. Error: %s", (errorMessage != NULL) ? errorMessage : "none");
        return false;
    }

    // flip normals if flipping faces, assimp doesn't do this
    /*if (reverseWinding)
    {
        m_pProgressCallbacks->DisplayDebugMessage("Flipping normals...");
        for (uint32 meshIndex = 0; meshIndex < m_pScene->mNumMeshes; meshIndex++)
        {
            aiMesh *pMesh = m_pScene->mMeshes[meshIndex];
            for (uint32 vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
            {
                if (pMesh->HasTangentsAndBitangents())
                {
                    if (pMesh->mTangents[vertexIndex].SquareLength() > 0.0f)
                        pMesh->mTangents[vertexIndex] = -pMesh->mTangents[vertexIndex];
                    if (pMesh->mBitangents[vertexIndex].SquareLength() > 0.0f)
                        pMesh->mBitangents[vertexIndex] = -pMesh->mBitangents[vertexIndex];
                    if (pMesh->mNormals[vertexIndex].SquareLength() > 0.0f)
                        pMesh->mNormals[vertexIndex] = -pMesh->mNormals[vertexIndex];
                }
                else if (pMesh->HasNormals())
                {
                    if (pMesh->mNormals[vertexIndex].SquareLength() > 0.0f)
                        pMesh->mNormals[vertexIndex] = -pMesh->mNormals[vertexIndex];
                }
            }
        }
    }*/

    // display info
    m_pProgressCallbacks->DisplayFormattedInformation("Scene info: %u animations, %u camera, %u lights, %u materials, %u meshes, %u textures", m_pScene->mNumAnimations, m_pScene->mNumCameras, m_pScene->mNumLights, m_pScene->mNumMaterials, m_pScene->mNumMeshes, m_pScene->mNumTextures);
    
    // ok
    return true;
}

bool AssimpSkeletalMeshImporter::CreateGenerator()
{
    // create generator
    m_pOutputGenerator = new SkeletalMeshGenerator();
    m_pOutputGenerator->SetSkeletonName(m_pOptions->SkeletonName);
    return true;
}

bool AssimpSkeletalMeshImporter::CreateMaterials()
{
    if (!m_pOptions->ImportMaterials)
    {
        for (uint32 i = 0; i < m_pScene->mNumMaterials; i++)
        {
            m_materialMapping.Add(m_pOutputGenerator->AddMaterialName(m_pOptions->DefaultMaterialName));
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
            m_materialMapping.Add(m_pOutputGenerator->AddMaterialName(materialResourceName));
            continue;
        }

        // import the material
        if (!AssimpMaterialImporter::ImportMaterial(pAIMaterial, m_pOptions->SourcePath, m_pOptions->MaterialDirectory, m_pOptions->MaterialPrefix, materialResourceName, m_pProgressCallbacks))
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Material '%s' failed import, using default material.", materialResourceName.GetCharArray());
            m_materialMapping.Add(m_pOutputGenerator->AddMaterialName(m_pOptions->DefaultMaterialName));
            continue;
        }

        // add it
        m_materialMapping.Add(m_pOutputGenerator->AddMaterialName(materialResourceName));

    }

    return true;
}

bool AssimpSkeletalMeshImporter::CreateMesh()
{
    // todo: global transform
    const aiNode *pRootNode = m_pScene->mRootNode;

    // parse root node
    if (!ParseNode(pRootNode))
        return false;

    // ok
    return true;
}

bool AssimpSkeletalMeshImporter::ParseNode(const aiNode *pNode)
{
    // parse node children
    for (uint32 childIndex = 0; childIndex < pNode->mNumChildren; childIndex++)
    {
        if (!ParseNode(pNode->mChildren[childIndex]))
            return false;
    }

    // CS transform
    //const aiMatrix4x4 CSTransformMatrix(AssimpHelpers::GetAssimpToWorldCoordinateSystemAIMatrix4x4());
    
    // for each mesh...
    uint64 smoothingGroupMask = (1 << 0);
    for (uint32 meshIndex = 0; meshIndex < pNode->mNumMeshes; meshIndex++)
    {
        const aiMesh *pMesh = m_pScene->mMeshes[pNode->mMeshes[meshIndex]];
        DebugAssert(pMesh != NULL);

        // get mesh name
        SmallString meshName;
        if (pMesh->mName.length == 0)
            meshName.Format("__submesh-%u__", meshIndex);
        else
            meshName = pMesh->mName.C_Str();

        // create output mesh, using max nweights for now
        DebugAssert(pMesh->mMaterialIndex < m_materialMapping.GetSize());
        m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] using material %s", meshName.GetCharArray(), m_pOutputGenerator->GetMaterialNameByIndex(m_materialMapping[pMesh->mMaterialIndex]).GetCharArray());

        // fixup flags
        if (pMesh->GetNumUVChannels() > 0)
            m_pOutputGenerator->SetProvideVertexTextureCoordinates(true);
        if (pMesh->GetNumColorChannels() > 0)
            m_pOutputGenerator->SetProvideVertexColors(true);

        // for this mesh
        MemArray<SkeletalMeshGenerator::Vertex> meshVertices;

        // create vertices without any weights in them
        for (uint32 vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            SkeletalMeshGenerator::Vertex vertex;

            //vertex.Position = (AssimpHelpers::AssimpVector3ToFloat3(CSTransformMatrix * pMesh->mVertices[vertexIndex]));
            //vertex.Position = m_globalTransform.TransformPoint(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mVertices[vertexIndex]));
            vertex.Position = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mVertices[vertexIndex]));
            vertex.TextureCoordinates = ((pMesh->GetNumUVChannels() > 0) ? AssimpHelpers::AssimpVector3ToFloat3(pMesh->mTextureCoords[0][vertexIndex]).xy() : float2::Zero);
            vertex.Color = ((pMesh->GetNumColorChannels() > 0) ? AssimpHelpers::AssimpColor4ToColor(pMesh->mColors[0][vertexIndex]) : MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
            
            if (pMesh->HasTangentsAndBitangents())
            {
                //vertex.Tangent = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mTangents[vertexIndex]));
                //vertex.Binormal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mBitangents[vertexIndex]));
                //vertex.Normal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
                vertex.Tangent = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mTangents[vertexIndex]));
                vertex.Binormal = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mBitangents[vertexIndex]));
                vertex.Normal = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
            }
            else if (pMesh->HasNormals())
            {
                vertex.Tangent = float3::UnitX;
                vertex.Binormal = float3::UnitY;
                //vertex.Normal = m_globalTransform.TransformNormal(AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
                vertex.Normal = (AssimpHelpers::AssimpVector3ToFloat3(pMesh->mNormals[vertexIndex]));
            }
            else
            {
                vertex.Tangent = float3::UnitX;
                vertex.Binormal = float3::UnitY;
                vertex.Normal = float3::UnitZ;
            }

            // no weights for now
            for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
            {
                vertex.BoneIndices[weightIndex] = -1;
                vertex.BoneWeights[weightIndex] = 0.0f;
            }

            meshVertices.Add(vertex);
        }
        m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] added %u vertices", meshName.GetCharArray(), pMesh->mNumVertices);

        // read bones
        uint32 maxBonesPerVertex = 0;
        m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] references %u bones", meshName.GetCharArray(), pMesh->mNumBones);
        for (uint32 boneIndex = 0; boneIndex < pMesh->mNumBones; boneIndex++)
        {
            const aiBone *pBone = pMesh->mBones[boneIndex];
            DebugAssert(pBone != NULL);

            int32 outputBoneIndex = m_pOutputGenerator->FindBoneIndexByName(pBone->mName.C_Str());
            if (outputBoneIndex < 0)
            {
                aiVector3D scaling;
                aiQuaternion rotation;
                aiVector3D translation;
                //aiMatrix4x4 correctedMatrix(AssimpHelpers::Float4x4ToAssimpMatrix4x4(m_inverseGlobalTransform) * pBone->mOffsetMatrix * AssimpHelpers::Float4x4ToAssimpMatrix4x4(m_globalTransform));
                //correctedMatrix.Decompose(scaling, rotation, translation);
                pBone->mOffsetMatrix.Decompose(scaling, rotation, translation);

                SkeletalMeshGenerator::Bone *pGeneratorBone = m_pOutputGenerator->AddBone(pBone->mName.C_Str(), Transform(AssimpHelpers::AssimpVector3ToFloat3(translation),
                                                                                                                          AssimpHelpers::AssimpQuaternionToQuaternion(rotation).Normalize(),
                                                                                                                          AssimpHelpers::AssimpVector3ToFloat3(scaling)));

                outputBoneIndex = (int32)pGeneratorBone->Index;
            }
            else
            {
                aiVector3D scaling;
                aiQuaternion rotation;
                aiVector3D translation;
                pBone->mOffsetMatrix.Decompose(scaling, rotation, translation);

                // should be equal
                const SkeletalMeshGenerator::Bone *pGeneratorBone = m_pOutputGenerator->GetBoneByIndex(outputBoneIndex);
                DebugAssert(AssimpHelpers::AssimpVector3ToFloat3(translation) == pGeneratorBone->LocalToBoneTransform.GetPosition());
                DebugAssert(AssimpHelpers::AssimpQuaternionToQuaternion(rotation).Normalize() == pGeneratorBone->LocalToBoneTransform.GetRotation());
                DebugAssert(AssimpHelpers::AssimpVector3ToFloat3(scaling) == pGeneratorBone->LocalToBoneTransform.GetScale());
            }

            // read weights for this bone
            for (uint32 weightIndex = 0; weightIndex < pBone->mNumWeights; weightIndex++)
            {
                const aiVertexWeight *pWeight = &pBone->mWeights[weightIndex];

                // get vertex
                DebugAssert(pWeight->mVertexId < meshVertices.GetSize());
                SkeletalMeshGenerator::Vertex *pOutputVertex = &meshVertices[pWeight->mVertexId];

                // find the first free bone in the vertex's weights
                uint32 freeBoneIndex;
                for (freeBoneIndex = 0; freeBoneIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; freeBoneIndex++)
                {
                    if (pOutputVertex->BoneIndices[freeBoneIndex] == -1)
                        break;
                }
                if (freeBoneIndex == SKELETAL_MESH_MAX_BONES_PER_VERTEX)
                {
                    m_pProgressCallbacks->DisplayFormattedError("in mesh %s: vertex %u references more than SKELETAL_MESH_MAX_BONES_PER_VERTEX (%u) bones", pMesh->mName.C_Str(), pWeight->mVertexId, (uint32)SKELETAL_MESH_MAX_BONES_PER_VERTEX);
                    return false;
                }

                // add to the vertex
                pOutputVertex->BoneIndices[freeBoneIndex] = outputBoneIndex;
                pOutputVertex->BoneWeights[freeBoneIndex] = pWeight->mWeight;
                maxBonesPerVertex = Max(maxBonesPerVertex, freeBoneIndex + 1);
            }
            m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] bone %s references %u vertices", meshName.GetCharArray(), pBone->mName.C_Str(), pBone->mNumWeights);
        }
        m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] maximum bone count per vertex: %u", meshName.GetCharArray(), maxBonesPerVertex);
        //pOutputMesh->SetWeightCount(maxBonesPerVertex);

        // add vertices to the generator
        PODArray<uint32> vertexMapping;
        vertexMapping.Reserve(meshVertices.GetSize());
        for (uint32 vertexIndex = 0; vertexIndex < meshVertices.GetSize(); vertexIndex++)
        {
            // normalize weights
            float weightTotal = 0.0f;
            for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                weightTotal += meshVertices[vertexIndex].BoneWeights[weightIndex];

            // should be 1
            if (!Math::NearEqual(weightTotal, 1.0f, Y_FLT_EPSILON))
            {
                m_pProgressCallbacks->DisplayFormattedWarning("[mesh %s] vertex %u weights do not sum to 1 (%.3f), fixing.", meshName.GetCharArray(), vertexIndex, weightTotal);

                for (uint32 weightIndex = 0; weightIndex < SKELETAL_MESH_MAX_BONES_PER_VERTEX; weightIndex++)
                    meshVertices[vertexIndex].BoneWeights[weightIndex] *= 1.0f / weightTotal;
            }


            uint32 realVertexIndex = m_pOutputGenerator->AddVertex(meshVertices[vertexIndex]);
            vertexMapping.Add(realVertexIndex);
        }

        // read triangles
        for (uint32 faceIndex = 0; faceIndex < pMesh->mNumFaces; faceIndex++)
        {
            const aiFace *pFace = &pMesh->mFaces[faceIndex];
            if (pFace->mNumIndices != 3)
            {
                m_pProgressCallbacks->DisplayFormattedError("in mesh %s: face %u does not have 3 indices", meshName.GetCharArray(), faceIndex);
                return false;
            }

            DebugAssert(pFace->mIndices[0] < vertexMapping.GetSize() &&
                        pFace->mIndices[1] < vertexMapping.GetSize() &&
                        pFace->mIndices[2] < vertexMapping.GetSize());

            m_pOutputGenerator->AddTriangle(m_materialMapping[pMesh->mMaterialIndex], smoothingGroupMask, 
                                            vertexMapping[pFace->mIndices[0]],
                                            vertexMapping[pFace->mIndices[1]], 
                                            vertexMapping[pFace->mIndices[2]]);
        }
        m_pProgressCallbacks->DisplayFormattedInformation("[mesh %s] added %u faces/triangles", meshName.GetCharArray(), pMesh->mNumFaces);

        // set next smoothing group
        smoothingGroupMask <<= 1;
    }
    
    return true;
}

bool AssimpSkeletalMeshImporter::PostProcess()
{
    //m_pOutputGenerator->CalculateTangentVectorsAndNormals();
    return true;
}

bool AssimpSkeletalMeshImporter::CreateCollisionShape()
{
    bool result = false;
    if (m_pOptions->CollisionShapeType.Compare("none"))
        return true;
    else if (m_pOptions->CollisionShapeType.CompareInsensitive("Box"))
        result = m_pOutputGenerator->BuildBoxCollisionShape();
    else if (m_pOptions->CollisionShapeType.CompareInsensitive("Sphere"))
        result = m_pOutputGenerator->BuildSphereCollisionShape();
    else if (m_pOptions->CollisionShapeType.CompareInsensitive("TriangleMesh"))
        result = m_pOutputGenerator->BuildTriangleMeshCollisionShape();
    else if (m_pOptions->CollisionShapeType.CompareInsensitive("ConvexHull"))
        result = m_pOutputGenerator->BuildConvexHullCollisionShape();
    else
    {
        m_pProgressCallbacks->DisplayFormattedError("Unknown collision shape type '%s'", m_pOptions->CollisionShapeType.GetCharArray());
        return false;
    }

    if (!result)
    {
        m_pProgressCallbacks->DisplayFormattedError("failed to build collision shape");
        return false;
    }

    return true;
}

bool AssimpSkeletalMeshImporter::WriteOutput()
{
    PathString outputResourceFileName;
    outputResourceFileName.Format("%s.skm.xml", m_pOptions->OutputResourceName.GetCharArray());

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

