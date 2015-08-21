set(HEADER_FILES
    Source/DemoGame/BaseDemoGameState.h
    Source/DemoGame/BaseDemoWorldGameState.h
    Source/DemoGame/DemoCamera.h
    Source/DemoGame/DemoGame.h
    Source/DemoGame/DemoUtilities.h
    Source/DemoGame/DrawCallStressDemo.h
    Source/DemoGame/ImGuiDemo.h
    Source/DemoGame/LaunchpadGameState.h
    Source/DemoGame/PrecompiledHeader.h
    Source/DemoGame/SkeletalAnimationDemo.h
)

set(SOURCE_FILES
    Source/DemoGame/BaseDemoGameState.cpp
    Source/DemoGame/BaseDemoWorldGameState.cpp
    Source/DemoGame/DemoCamera.cpp
    Source/DemoGame/DemoGame.cpp
    Source/DemoGame/DemoUtilities.cpp
    Source/DemoGame/DrawCallStressDemo.cpp
    Source/DemoGame/ImGuiDemo.cpp
    Source/DemoGame/LaunchpadGameState.cpp
    Source/DemoGame/Main.cpp
    Source/DemoGame/PrecompiledHeader.cpp
    Source/DemoGame/SkeletalAnimationDemo.cpp
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
