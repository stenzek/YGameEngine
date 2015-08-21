set(HEADER_FILES
    OpenGLES2Common.h
    OpenGLES2ConstantLibrary.h
    OpenGLES2CVars.h
    OpenGLES2Defines.h
    OpenGLES2GPUBuffer.h
    OpenGLES2GPUContext.h
    OpenGLES2GPUShaderProgram.h
    OpenGLES2GPUTexture.h
    OpenGLES2Renderer.h
    OpenGLES2RendererOutputBuffer.h
)

set(SOURCE_FILES
    OpenGLES2ConstantLibrary.cpp
    OpenGLES2CVars.cpp
    OpenGLES2Defines.cpp
    OpenGLES2GPUBuffer.cpp
    OpenGLES2GPUContext.cpp
    OpenGLES2GPUShaderProgram.cpp
    OpenGLES2GPUTexture.cpp
    OpenGLES2Renderer.cpp
    OpenGLES2RendererOutputBuffer.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${OPENGL_INCLUDE_DIR}
                    ${SDL2_INCLUDE_DIR})

add_library(EngineOpenGLES2Renderer STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineOpenGLES2Renderer
                      EngineMain
                      EngineCore
                      glad
                      ${SDL2_LIBRARY})

