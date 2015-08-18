set(HEADER_FILES
    BlockAnimation.h
    BlockDrawTemplate.h
    BlockEngineCVars.h
    BlockWorldChunkCollisionShape.h
    BlockWorldChunk.h
    BlockWorldChunkRenderProxy.h
    BlockWorldGenerator.h
    BlockWorld.h
    BlockWorldMesher.h
    BlockWorldSection.h
    BlockWorldTypes.h
    BlockWorldVertexFactory.h
)

set(SOURCE_FILES
    BlockAnimation.cpp
    BlockDrawTemplate.cpp
    BlockEngineCVars.cpp
    BlockWorldChunkCollisionShape.cpp
    BlockWorldChunk.cpp
    BlockWorldChunkRenderProxy.cpp
    BlockWorld.cpp
    BlockWorldGenerator.cpp
    BlockWorldMesher.cpp
    BlockWorldSection.cpp
    BlockWorldVertexFactory.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY})

add_library(EngineBlockEngine STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineBlockEngine
                      EngineMain 
                      EngineCore)

