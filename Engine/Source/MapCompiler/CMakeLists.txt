set(HEADER_FILES
    Common.h
    MapCompiler.h
    MapSourceGeometryData.h
    MapSource.h
    MapSourceTerrainData.h
)

set(SOURCE_FILES
    MapCompiler.cpp
    MapSource.cpp
    MapSourceGeometryData.cpp
    MapSourceTerrainData.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY})

add_library(EngineMapCompiler STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineMapCompiler 
                      EngineMain 
                      EngineCore)

