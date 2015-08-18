#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/DemoUtilities.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/ResourceManager.h"
#include "GameFramework/DirectionalLightEntity.h"
#include "GameFramework/StaticMeshEntity.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
Log_SetChannel(DemoUtilities);

DirectionalLightEntity *DemoUtilities::CreateSunLight(World *pWorld, const float3 &color /*= float3(1.0f, 1.0f, 1.0f)*/, const float brightness /*= 1.0f*/, const float ambientContribution /*= 0.2f*/, const float3 &rotation /*= float3(45.0f, 0.0f, 0.0f)*/)
{
    Quaternion rotationQuat(Quaternion::FromEulerAngles(rotation.x, rotation.y, rotation.z));

    DirectionalLightEntity *pEntity = new DirectionalLightEntity();
    pEntity->Create(pWorld->AllocateEntityID(), ENTITY_MOBILITY_MOVABLE, rotationQuat, true, color, brightness, LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS, ambientContribution);
    pWorld->AddEntity(pEntity);
    return pEntity;
}

Entity *DemoUtilities::CreatePlaneShape(World *pWorld, const float3 &normal, const float3 &origin, float size, const Material *pMaterial, float textureRepeatInterval /* = 1.0f */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
#if 0
    // find rotation vector from ground
    Quaternion shapeRotation(Quaternion::FromTwoUnitVectors(float3::UnitZ, normal).Normalize());

    // generate shape
    StaticMeshGenerator staticMeshGenerator;
    staticMeshGenerator.SetVertexTextureCoordinatesEnabled(true);

    // create lod + batch
    uint32 lodIndex = staticMeshGenerator.AddLOD();
    uint32 batchIndex = staticMeshGenerator.AddBatch(lodIndex, pMaterial->GetName());

    // work out last texture coord
    float startTextureCoord = 0.0f;
    float endTextureCoord = size / textureRepeatInterval;

    // generate vertices
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(-size, size, 1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(startTextureCoord, startTextureCoord));  // 0 top-back-left
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(size, size, 1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(endTextureCoord, startTextureCoord));     // 1 top-back-right
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(-size, -size, 1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(startTextureCoord, endTextureCoord));   // 2 top-front-left
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(size, -size, 1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(endTextureCoord, endTextureCoord));      // 3 top-front-right
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(-size, size, -1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(startTextureCoord, startTextureCoord)); // 4 bottom-back-left
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(size, size, -1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(endTextureCoord, startTextureCoord));    // 5 bottom-back-right
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(-size, -size, -1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(startTextureCoord, endTextureCoord));  // 6 bottom-front-left
    staticMeshGenerator.AddVertex(lodIndex, (shapeRotation * float3(size, -size, -1.0f)), float3::UnitX, float3::UnitY, float3::UnitZ, float2(endTextureCoord, endTextureCoord));     // 7 bottom-front-right

    // generate triangles
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 2, 3, 0);        // top
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 0, 3, 1);        // top
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 6, 4, 7);        // bottom
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 7, 4, 5);        // bottom
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 6, 7, 2);        // front
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 2, 7, 3);        // front
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 4, 0, 5);        // back
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 5, 0, 4);        // back
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 4, 0, 6);        // left
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 6, 0, 2);        // left
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 5, 7, 1);        // right
    staticMeshGenerator.AddTriangle(lodIndex, batchIndex, 1, 7, 3);        // right

    // build batches
    staticMeshGenerator.GenerateTangents(0);
    staticMeshGenerator.CalculateBounds();
    staticMeshGenerator.BuildBoxCollisionShape();

    // create staticmesh from this
    AutoReleasePtr<StaticMesh> pStaticMesh = new StaticMesh();
    if (!pStaticMesh->Create("__planeshape__", &staticMeshGenerator))
    {
        Log_ErrorPrintf("DemoUtilities::CreatePlaneShape: Staticmesh creation failed");
        return nullptr;
    }

    float3 scale(float3::One);

#else
    // fixme
    AutoReleasePtr<const StaticMesh> pStaticMesh = g_pResourceManager->GetStaticMesh("models/engine/unit_cube");
    float3 scale(size, size, 0.01f);

#endif

    // create entity
    StaticMeshEntity *pStaticMeshEntity = new StaticMeshEntity();
    pStaticMeshEntity->Create(pWorld->AllocateEntityID(), ENTITY_MOBILITY_STATIC, origin, Quaternion::Identity, scale, pStaticMesh, true, true, shadowFlags);
    pWorld->AddEntity(pStaticMeshEntity);
    return pStaticMeshEntity;
}

