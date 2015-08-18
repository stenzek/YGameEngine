set(HEADER_FILES
    Common.h
    DecalManager.h
    ImGuiBridge.h
    MiniGUIContext.h
    PrecompiledHeader.h
    Renderer.h
    RendererStateBlock.h
    RendererTypes.h
    RenderProfiler.h
    RenderProxies/BlockMeshRenderProxy.h
    RenderProxies/CompositeRenderProxy.h
    RenderProxies/DirectionalLightRenderProxy.h
    RenderProxies/PointLightRenderProxy.h
    RenderProxies/SkeletalMeshRenderProxy.h
    RenderProxies/SpriteRenderProxy.h
    RenderProxies/StaticMeshRenderProxy.h
    RenderProxies/VolumetricLightRenderProxy.h
    RenderProxy.h
    RenderQueue.h
    RenderWorld.h
    ShaderCompilerFrontend.h
    ShaderComponent.h
    ShaderComponentTypeInfo.h
    ShaderConstantBuffer.h
    ShaderMap.h
    ShaderProgram.h
    ShaderProgramSelector.h
    Shaders/DeferredShadingShaders.h
    Shaders/DepthOnlyShader.h
    Shaders/DownsampleShader.h
    Shaders/ForwardShadingShaders.h
    Shaders/FullBrightShader.h
    Shaders/MobileShaders.h
    Shaders/OneColorShader.h
    Shaders/OverlayShader.h
    Shaders/PlainShader.h
    Shaders/ShadowMapShader.h
    Shaders/SSAOShader.h
    Shaders/TextureBlitShader.h
    VertexBufferBindingArray.h
    VertexFactories/BlockMeshVertexFactory.h
    VertexFactories/LocalVertexFactory.h
    VertexFactories/PlainVertexFactory.h
    VertexFactories/SkeletalMeshVertexFactory.h
    VertexFactory.h
    VertexFactoryTypeInfo.h
    WorldRenderer.h
    WorldRenderers/CompositingWorldRenderer.h
    WorldRenderers/CSMShadowMapRenderer.h
    WorldRenderers/CubeMapShadowMapRenderer.h
    WorldRenderers/DebugNormalsWorldRenderer.h
    WorldRenderers/DeferredShadingWorldRenderer.h
    WorldRenderers/ForwardShadingWorldRenderer.h
    WorldRenderers/FullBrightWorldRenderer.h
    WorldRenderers/MobileWorldRenderer.h
    WorldRenderers/SingleShaderWorldRenderer.h
    WorldRenderers/SSMShadowMapRenderer.h
)

set(SOURCE_FILES
    DecalManager.cpp
    ImGuiBridge.cpp
    MiniGUIContext.cpp
    PrecompiledHeader.cpp
    Renderer.cpp
    RendererStateBlock.cpp
    RendererTypes.cpp
    RenderProfiler.cpp
    RenderProxies/BlockMeshRenderProxy.cpp
    RenderProxies/CompositeRenderProxy.cpp
    RenderProxies/DirectionalLightRenderProxy.cpp
    RenderProxies/PointLightRenderProxy.cpp
    RenderProxies/SkeletalMeshRenderProxy.cpp
    RenderProxies/SpriteRenderProxy.cpp
    RenderProxies/StaticMeshRenderProxy.cpp
    RenderProxies/VolumetricLightRenderProxy.cpp
    RenderProxy.cpp
    RenderQueue.cpp
    RenderWorld.cpp
    ShaderCompilerFrontend.cpp
    ShaderComponent.cpp
    ShaderComponentTypeInfo.cpp
    ShaderConstantBuffer.cpp
    ShaderMap.cpp
    ShaderProgram.cpp
    ShaderProgramSelector.cpp
    Shaders/DeferredShadingShaders.cpp
    Shaders/DepthOnlyShader.cpp
    Shaders/DownsampleShader.cpp
    Shaders/ForwardShadingShaders.cpp
    Shaders/FullBrightShader.cpp
    Shaders/MobileShaders.cpp
    Shaders/OneColorShader.cpp
    Shaders/OverlayShader.cpp
    Shaders/PlainShader.cpp
    Shaders/ShadowMapShader.cpp
    Shaders/SSAOShader.cpp
    Shaders/TextureBlitShader.cpp
    VertexBufferBindingArray.cpp
    VertexFactories/BlockMeshVertexFactory.cpp
    VertexFactories/LocalVertexFactory.cpp
    VertexFactories/PlainVertexFactory.cpp
    VertexFactories/SkeletalMeshVertexFactory.cpp
    VertexFactory.cpp
    VertexFactoryTypeInfo.cpp
    WorldRenderer.cpp
    WorldRenderers/CompositingWorldRenderer.cpp
    WorldRenderers/CSMShadowMapRenderer.cpp
    WorldRenderers/CubeMapShadowMapRenderer.cpp
    WorldRenderers/DebugNormalsWorldRenderer.cpp
    WorldRenderers/DeferredShadingWorldRenderer.cpp
    WorldRenderers/ForwardShadingWorldRenderer.cpp
    WorldRenderers/FullBrightWorldRenderer.cpp
    WorldRenderers/MobileWorldRenderer.cpp
    WorldRenderers/SingleShaderWorldRenderer.cpp
    WorldRenderers/SSMShadowMapRenderer.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${ENGINE_BASE_DIRECTORY}/../Dependancies/microprofile
                    ${SDL2_INCLUDE_DIR})

add_library(EngineRenderer STATIC ${HEADER_FILES} ${SOURCE_FILES})

set(EXTRA_LIBRARIES "")

if(WITH_IMGUI)
    LIST(APPEND EXTRA_LIBRARIES imgui)
endif()

if(WITH_RENDERER_D3D11)
    LIST(APPEND EXTRA_LIBRARIES "EngineD3D11Renderer")
endif()

if(WITH_RENDERER_OPENGL)
    LIST(APPEND EXTRA_LIBRARIES "EngineOpenGLRenderer")
endif()

if(WITH_RENDERER_OPENGLES2)
    LIST(APPEND EXTRA_LIBRARIES "EngineOpenGLES2Renderer")
endif()

if(WITH_RESOURCECOMPILER_EMBEDDED)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface EngineResourceCompiler")
elseif(WITH_RESOURCECOMPILER_SUBPROCESS)
    LIST(APPEND EXTRA_LIBRARIES "EngineResourceCompilerInterface")
endif()

target_link_libraries(EngineRenderer EngineMain ${EXTRA_LIBRARIES} ${SDL2_LIBRARY})

