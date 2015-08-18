set(HEADER_FILES
)

set(SOURCE_FILES
    Main.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} 
                    ${ENGINE_BASE_DIRECTORY}/ResourceCompilerStandalone)
                    
add_executable(ResourceCompiler ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(ResourceCompiler
                      EngineResourceCompiler
                      EngineMain
                      EngineCore)

install(TARGETS ResourceCompiler DESTINATION ${INSTALL_BINARIES_DIRECTORY})

