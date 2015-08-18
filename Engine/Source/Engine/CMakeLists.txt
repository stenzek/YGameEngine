include(${CMAKE_MODULE_PATH}/PrecompiledHeader.cmake)

set(HEADER_FILES
    ArcBallCamera.h
    BlockMeshBuilder.h
    BlockMeshCollisionShape.h
    BlockMesh.h
    BlockMeshUtilities.h
    BlockMeshVolume.h
    BlockPalette.h
    Brush.h
    BulletDebugDraw.h
    Camera.h
    CommandQueue.h
    Common.h
    Component.h
    ComponentTypeInfo.h
    DataFormats.h
    Defines.h
    DynamicWorld.h
    EngineCVars.h
    Engine.h
    Entity.h
    EntityTypeInfo.h
    Font.h
    FPSCounter.h
    InputManager.h
    Map.h
    Material.h
    MaterialShader.h
    OverlayConsole.h
    ParticleSystemBuiltinEmitters.h
    ParticleSystemBuiltinModules.h
    ParticleSystemCommon.h
    ParticleSystemEmitter.h
    ParticleSystem.h
    ParticleSystemModule.h
    ParticleSystemRenderProxy.h
    ParticleSystemVertexFactory.h
    Physics/BoxCollisionShape.h
    Physics/BulletHeaders.h
    Physics/CollisionObject.h
    Physics/CollisionShape.h
    Physics/ConvexHullCollisionShape.h
    Physics/GhostObject.h
    Physics/KinematicObject.h
    Physics/PhysicsProxy.h
    Physics/PhysicsWorld.h
    Physics/RigidBody.h
    Physics/ScaledTriangleMeshCollisionShape.h
    Physics/SphereCollisionShape.h
    Physics/StaticObject.h
    Physics/TriangleMeshCollisionShape.h
    PrecompiledHeader.h
    Profiling.h
    ResourceManager.h
    ScriptManager.h
    ScriptObject.h
    ScriptObjectTypeInfo.h
    ScriptProxyFunctions.h
    ScriptTypes.h
    SDLHeaders.h
    SkeletalAnimation.h
    SkeletalAnimationPlayer.h
    SkeletalMesh.h
    Skeleton.h
    StaticMesh.h
    TerrainLayerList.h
    TerrainQuadTree.h
    TerrainRendererCDLOD.h
    TerrainRenderer.h
    TerrainRendererNull.h
    TerrainSectionCollisionShape.h
    TerrainSection.h
    TerrainTypes.h
    Texture.h
    World.h
)

set(SOURCE_FILES
    ArcBallCamera.cpp
    BlockMeshBuilder.cpp
    BlockMeshCollisionShape.cpp
    BlockMesh.cpp
    BlockMeshUtilities.cpp
    BlockMeshVolume.cpp
    BlockPalette.cpp
    Brush.cpp
    BulletDebugDraw.cpp
    Camera.cpp
    CommandQueue.cpp
    Component.cpp
    ComponentTypeInfo.cpp
    Defines.cpp
    DynamicWorld.cpp
    Engine.cpp
    EngineCVars.cpp
    EngineTypes.cpp
    Entity.cpp
    EntityTypeInfo.cpp
    Font.cpp
    FPSCounter.cpp
    InputManager.cpp
    Map.cpp
    Material.cpp
    MaterialShader.cpp
    OverlayConsole.cpp
    ParticleSystemBuiltinEmitters.cpp
    ParticleSystemBuiltinModules.cpp
    ParticleSystem.cpp
    ParticleSystemEmitter.cpp
    ParticleSystemModule.cpp
    ParticleSystemRenderProxy.cpp
    ParticleSystemVertexFactory.cpp
    Physics/BoxCollisionShape.cpp
    Physics/CollisionObject.cpp
    Physics/CollisionShape.cpp
    Physics/ConvexHullCollisionShape.cpp
    Physics/GhostObject.cpp
    Physics/KinematicObject.cpp
    Physics/PhysicsProxy.cpp
    Physics/PhysicsWorld.cpp
    Physics/RigidBody.cpp
    Physics/ScaledTriangleMeshCollisionShape.cpp
    Physics/SphereCollisionShape.cpp
    Physics/StaticObject.cpp
    Physics/TriangleMeshCollisionShape.cpp
    PrecompiledHeader.cpp
    Profiling.cpp
    ResourceManager.cpp
    ScriptBuiltinFunctions.cpp
    ScriptManager.cpp
    ScriptObject.cpp
    ScriptObjectTypeInfo.cpp
    ScriptPrimitiveTypes.cpp
    ScriptTypes.cpp
    SkeletalAnimation.cpp
    SkeletalAnimationPlayer.cpp
    SkeletalMesh.cpp
    Skeleton.cpp
    StaticMesh.cpp
    TerrainLayerList.cpp
    TerrainQuadTree.cpp
    TerrainRendererCDLOD.cpp
    TerrainRenderer.cpp
    TerrainRendererNull.cpp
    TerrainSectionCollisionShape.cpp
    TerrainSection.cpp
    TerrainTypes.cpp
    Texture.cpp
    World.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${ENGINE_BASE_DIRECTORY}/../Dependancies/microprofile
                    ${SDL2_INCLUDE_DIR})

add_library(EngineMain STATIC ${HEADER_FILES} ${SOURCE_FILES})

set(EXTRA_LIBRARIES "")

LIST(APPEND EXTRA_LIBRARIES "EngineRenderer")

# remove once EngineTypes.cpp mess is fixed
LIST(APPEND EXTRA_LIBRARIES "EngineGameFramework")

if(WITH_RESOURCECOMPILER_EMBEDDED)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface EngineResourceCompiler")
elseif(WITH_RESOURCECOMPILER_SUBPROCESS)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface")
endif()

#add_precompiled_header(EngineMain PrecompiledHeader.h SOURCE_CXX PrecompiledHeader.cpp)

target_link_libraries(EngineMain 
                      EngineCore 
					  ${EXTRA_LIBRARIES}
                      bullet
                      lua
                      ${SDL2_LIBRARY})

