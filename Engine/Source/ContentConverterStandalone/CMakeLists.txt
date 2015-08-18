set(HEADER_FILES
    ContentConverter.h
)

set(SOURCE_FILES
    AssimpSceneImporter.cpp
    AssimpSkeletalAnimationImporter.cpp
    AssimpSkeletalMeshImporter.cpp
    AssimpSkeletonImporter.cpp
    AssimpStaticMeshImporter.cpp
    FontImporter.cpp
    Main.cpp
    OBJImporter.cpp
    TextureImporter.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} ${TOOLS_BASE_DIRECTORY}/ContentConverter)
add_executable(ContentConverter ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(ContentConverter EngineContentConverter EngineMain EngineCore)

