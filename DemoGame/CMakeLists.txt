set(HEADER_FILES
    DemoGame/BaseDemoGameState.h
    DemoGame/DemoCamera.h
    DemoGame/DemoGame.h
    DemoGame/DemoUtilities.h
    DemoGame/LaunchpadGameState.h
    DemoGame/PrecompiledHeader.h
    DemoGame/SkeletalAnimationDemo.h
)

set(SOURCE_FILES
    DemoGame/BaseDemoGameState.cpp
    DemoGame/DemoCamera.cpp
    DemoGame/DemoGame.cpp
    DemoGame/DemoUtilities.cpp
    DemoGame/LaunchpadGameState.cpp
    DemoGame/Main.cpp
    DemoGame/PrecompiledHeader.cpp
    DemoGame/SkeletalAnimationDemo.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} ${GameEngineDev_SOURCE_DIR}/DemoGame/Source ${SDL2_INCLUDE_DIR})
if(ANDROID)
    add_library(DemoGameBin STATIC ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(DemoGameBin ${HEADER_FILES} ${SOURCE_FILES})
    
    #if(APPLE)
        #set_target_properties(DemoGameBin PROPERTIES MACOSX_BUNDLE TRUE)
    #endif()
    
    install(TARGETS DemoGameBin DESTINATION ${INSTALL_BINARIES_DIRECTORY})
endif()

target_link_libraries(DemoGameBin EngineBaseGame EngineMain EngineCore bullet)
