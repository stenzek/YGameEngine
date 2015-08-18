set(HEADER_FILES
    Common.h
    ResourceCompilerInterface.h
    ResourceCompilerInterfaceIntegrated.h
    ResourceCompilerInterfaceRemote.cpp
    ResourceCompilerInterfaceRemote.h
)

set(SOURCE_FILES
    ResourceCompilerInterfaceRemote.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY})

add_library(EngineResourceCompilerInterface STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineResourceCompilerInterface 
                      EngineMain 
                      EngineCore)

