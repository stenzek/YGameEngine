# libraries
add_subdirectory(Source/Core)
add_subdirectory(Source/MathLib)
add_subdirectory(Source/Engine)
add_subdirectory(Source/Renderer)
add_subdirectory(Source/GameFramework)

if(WITH_RENDERER_D3D11)
    add_subdirectory(Source/D3D11Renderer)
endif()
if(WITH_RENDERER_OPENGL)
    add_subdirectory(Source/OpenGLRenderer)
endif()
if(WITH_RENDERER_OPENGLES2)
    add_subdirectory(Source/OpenGLES2Renderer)
endif()

if(WITH_RESOURCECOMPILER)
    add_subdirectory(Source/ResourceCompiler)
    add_subdirectory(Source/ResourceCompilerInterface)
	if(WITH_RESOURCECOMPILER_STANDALONE)
		add_subdirectory(Source/ResourceCompilerStandalone)
	endif()
endif()

if(WITH_MAPCOMPILER)
    add_subdirectory(Source/MapCompiler)
	if(WITH_MAPCOMPILER_STANDALONE)
		add_subdirectory(Source/MapCompilerStandalone)
	endif()
endif()

if(WITH_CONTENTCONVERTER)
    add_subdirectory(Source/ContentConverter)
	if(WITH_CONTENTCONVERTER_STANDALONE)
		add_subdirectory(Source/ContentConverterStandalone)
    endif()
endif()

if(WITH_BASEGAME)
	add_subdirectory(Source/BaseGame)
endif()

if(WITH_BLOCKENGINE)
	add_subdirectory(Source/BlockEngine)
endif()

