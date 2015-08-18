set(HEADER_FILES
    BlockMeshBrush.h
    BlockMeshComponent.h
    BlockMeshEntity.h
    Common.h
    DirectionalLightEntity.h
    InterpolatorComponent.h
    ParticleEmitterComponent.h
    ParticleEmitterEntity.h
    PointLightComponent.h
    PointLightEntity.h
    PositionInterpolatorComponent.h
    PrecompiledHeader.h
    RotationInterpolatorComponent.h
    SpriteComponent.h
    SpriteEntity.h
    StaticMeshBrush.h
    StaticMeshComponent.h
    StaticMeshEntity.h
    StaticMeshRigidBodyEntity.h
    SunLightEntity.h
)

set(SOURCE_FILES
    BlockMeshBrush.cpp
    BlockMeshComponent.cpp
    BlockMeshEntity.cpp
    DirectionalLightEntity.cpp
    InterpolatorComponent.cpp
    ParticleEmitterComponent.cpp
    ParticleEmitterEntity.cpp
    PointLightComponent.cpp
    PointLightEntity.cpp
    PositionInterpolatorComponent.cpp
    PrecompiledHeader.cpp
    RotationInterpolatorComponent.cpp
    SpriteComponent.cpp
    SpriteEntity.cpp
    StaticMeshBrush.cpp
    StaticMeshComponent.cpp
    StaticMeshEntity.cpp
    StaticMeshRigidBodyEntity.cpp
    SunLightEntity.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${ENGINE_BASE_DIRECTORY}/../Dependancies/microprofile
                    ${SDL2_INCLUDE_DIR})

add_library(EngineGameFramework STATIC ${HEADER_FILES} ${SOURCE_FILES})

set(EXTRA_LIBRARIES "")

LIST(APPEND EXTRA_LIBRARIES EngineRenderer)

if(WITH_RESOURCECOMPILER_EMBEDDED)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface EngineResourceCompiler")
elseif(WITH_RESOURCECOMPILER_SUBPROCESS)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface")
endif()

target_link_libraries(EngineGameFramework EngineMain)

