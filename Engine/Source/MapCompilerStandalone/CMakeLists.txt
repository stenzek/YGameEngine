set(HEADER_FILES
    Common.h
)

set(SOURCE_FILES
    MapCompiler.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} 
                    ${ENGINE_BASE_DIRECTORY}/MapCompilerStandalone)

add_executable(MapCompiler ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(MapCompiler
                      EngineMapCompiler
                      EngineResourceCompiler
                      EngineMain
                      EngineCore)

install(TARGETS MapCompiler DESTINATION ${INSTALL_BINARIES_DIRECTORY})

