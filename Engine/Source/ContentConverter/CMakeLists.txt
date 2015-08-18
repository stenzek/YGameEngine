set(HEADER_FILES
    AssimpCommon.h
    AssimpMaterialImporter.h
    AssimpSceneImporter.h
    AssimpSkeletalAnimationImporter.h
    AssimpSkeletalMeshImporter.h
    AssimpSkeletonImporter.h
    AssimpStaticMeshImporter.h
    BaseImporter.h
    Common.h
    FontImporter.h
    OBJImporter.h
    stb_truetype.h
    TextureImporter.h
)

set(SOURCE_FILES
    AssimpCommon.cpp
    AssimpSceneImporter.cpp
    AssimpMaterialImporter.cpp
    AssimpSkeletalAnimationImporter.cpp
    AssimpSkeletalMeshImporter.cpp
    AssimpSkeletonImporter.cpp
    AssimpStaticMeshImporter.cpp
    BaseImporter.cpp
    FontImporter.cpp
    OBJImporter.cpp
    TextureImporter.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY} ${FREETYPE_INCLUDE_DIRS})

add_library(EngineContentConverter STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineContentConverter 
                      EngineMapCompiler 
                      EngineResourceCompiler 
                      EngineCore
                      EngineMain 
                      assimp
                      ${FREETYPE_LIBRARIES})

