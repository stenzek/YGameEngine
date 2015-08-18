set(HEADER_FILES
)

set(SOURCE_FILES
    Source/TestMath.cpp
    Source/TestRenderer.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} ${ENGINE_BASE_DIRECTORY}/Tests ${SDL2_INCLUDE_DIR})

if(ANDROID)
    add_library(EngineTestRunner SHARED ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(EngineTestRunner ${HEADER_FILES} ${SOURCE_FILES})
endif() 

target_link_libraries(EngineTestRunner
                      ${SDL2MAIN_LIBRARY}
                      EngineMain
                      EngineCore)

