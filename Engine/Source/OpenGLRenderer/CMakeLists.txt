set(HEADER_FILES
    OpenGLCommon.h
    OpenGLCVars.h
    OpenGLDefines.h
    OpenGLGPUBuffer.h
    OpenGLGPUContext.h
    OpenGLGPUQuery.h
    OpenGLGPUShaderProgram.h
    OpenGLGPUTexture.h
    OpenGLRenderer.h
    OpenGLRendererOutputBuffer.h
    OpenGLShaderCacheEntry.h
)

set(SOURCE_FILES
    OpenGLCVars.cpp
    OpenGLDefines.cpp
    OpenGLGPUBuffer.cpp
    OpenGLGPUContext.cpp
    OpenGLGPUQuery.cpp
    OpenGLGPUShaderProgram.cpp
    OpenGLGPUTexture.cpp
    OpenGLRenderer.cpp
    OpenGLRendererOutputBuffer.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${OPENGL_INCLUDE_DIR}
                    ${SDL2_INCLUDE_DIR})
                    
add_library(EngineOpenGLRenderer STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineOpenGLRenderer
                      EngineMain
                      EngineCore
                      glad
                      ${OPENGL_LIBRARIES}
                      ${EXTRA_LIBRARIES})

