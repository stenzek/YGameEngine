set(HEADER_FILES
    BaseGame.h
    Common.h
    GameEntity.h
    GameState.h
    InteractiveEntity.h
    LivingEntity.h
    LoadingScreenProgressCallbacks.h
)

set(SOURCE_FILES
    BaseGame.cpp
    GameEntity.cpp
    InteractiveEntity.cpp
    LivingEntity.cpp
    LoadingScreenProgressCallbacks.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${ENGINE_BASE_DIRECTORY}/../Dependancies/microprofile
                    ${SDL2_INCLUDE_DIR})

add_library(EngineBaseGame STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineBaseGame
                      EngineGameFramework
                      EngineMain 
                      EngineCore)

