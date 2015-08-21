set(HEADER_FILES
    BlockMeshGenerator.h
    BlockPaletteGenerator.h
    ClassTableGenerator.h
    CollisionShapeGenerator.h
    Common.h
    FontGenerator.h
    MaterialGenerator.h
    MaterialShaderGenerator.h
    ObjectTemplate.h
    ObjectTemplateManager.h
    ParticleSystemGenerator.h
    ResourceCompilerCallbacks.h
    ResourceCompiler.h
    ShaderCompilerD3D.h
    ShaderCompiler.h
    ShaderCompilerOpenGL.h
    ShaderGraphBuiltinNodes.h
    ShaderGraphCompiler.h
    ShaderGraphCompilerHLSL.h
    ShaderGraph.h
    ShaderGraphNode.h
    ShaderGraphNodeType.h
    ShaderGraphSchema.h
    SkeletalAnimationGenerator.h
    SkeletalMeshGenerator.h
    SkeletonGenerator.h
    StaticMeshGenerator.h
    TerrainLayerListGenerator.h
    TextureGenerator.h
)

set(SOURCE_FILES
    BlockMeshGenerator.cpp
    BlockPaletteGenerator.cpp
    ClassTableGenerator.cpp
    CollisionShapeGenerator.cpp
    FontGenerator.cpp
    MaterialGenerator.cpp
    MaterialShaderGenerator.cpp
    ObjectTemplate.cpp
    ObjectTemplateManager.cpp
    ParticleSystemGenerator.cpp
    ResourceCompiler.cpp
    ResourceCompilerInterfaceIntegrated.cpp
    ResourceCompilerInterfaceRemoteServer.cpp
    ShaderCompiler.cpp
    ShaderCompilerD3D.cpp
    ShaderCompilerOpenGL.cpp
    ShaderGraphBuiltinNodes.cpp
    ShaderGraphCompiler.cpp
    ShaderGraphCompilerHLSL.cpp
    ShaderGraph.cpp
    ShaderGraphNode.cpp
    ShaderGraphNodeType.cpp
    ShaderGraphSchema.cpp
    SkeletalAnimationGenerator.cpp
    SkeletalMeshGenerator.cpp
    SkeletonGenerator.cpp
    StaticMeshGenerator.cpp
    TerrainLayerListGenerator.cpp
    TextureGenerator.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY})

set(EXTRA_LIBRARIES "")
if(HAVE_HLSLTRANSLATOR)
    LIST(APPEND EXTRA_LIBRARIES HLSLTranslator)
endif()

add_library(EngineResourceCompiler STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineResourceCompiler 
                      ${EXTRA_LIBRARIES}
                      EngineMain 
                      EngineCore)

