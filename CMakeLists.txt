cmake_minimum_required(VERSION 2.8)

# handle clang
if("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR APPLE)
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# pull in modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules/")

# set the install directory to the build directory
#SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}" CACHE STRING "Foo" INTERNAL "")

# set project
project(GameEngineDev CXX C)

# compile os-dependant dependancies, this can't be done with a CMakeLists in dependancies
# due to how the variables are exported back to us through set(... PARENT_SCOPE)
if(ANDROID)
    add_subdirectory(Dependancies/Android)
endif()

# determine 32bit vs 64bit build
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITNESS 64)
else()
    set(BITNESS 32)
endif()

# we don't want SDLmain by default
if(NOT ANDROID)
    SET(SDL2_BUILDING_LIBRARY TRUE)
endif()

# find system dependancies
if(EMSCRIPTEN)
    # emscripten libraries
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2 -s USE_ZLIB=1")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s USE_ZLIB=1")
elseif(ANDROID)
    # android libraries
else()
    find_package(ZLIB REQUIRED)
    find_package(Threads REQUIRED)
    find_package(LibXml2 REQUIRED)
    find_package(PCRE REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(Freetype REQUIRED)
endif()

# platform-specific stuff
if(APPLE)
    include_directories("${CMAKE_SOURCE_DIR}/Dependancies/OSX/include")
    link_directories("${CMAKE_SOURCE_DIR}/Dependancies/OSX/lib")
    set(QT5_BASE_DIR "${CMAKE_SOURCE_DIR}/Dependancies/OSX/qt")
    
    # include frameworks
    find_library(CARBON_LIBRARY Carbon)
    find_library(APPKIT_LIBRARY AppKit)
elseif(UNIX AND NOT ANDROID)
    include_directories("${CMAKE_SOURCE_DIR}/Dependancies/Linux/include")
    if(BITNESS EQUAL 64)
        link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Linux/lib64")
    #    set(QT5_BASE_DIR "${CMAKE_SOURCE_DIR}/Dependancies/Linux/qt64")
    else()
        link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Linux/lib32")
    #    set(QT5_BASE_DIR "${CMAKE_SOURCE_DIR}/Dependancies/Linux/qt32")
    endif()
    
    # needs X11
    find_package(X11)
elseif(WIN32)
    include_directories("${CMAKE_SOURCE_DIR}/Dependancies/Windows/include")
    if(BITNESS EQUAL 64)
        if(CMAKE_BUILD_TYPE EQUAL "Debug")
            link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Windows/lib64-debug")
        else()
            link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Windows/lib64")
        endif()
        set(QT5_BASE_DIR "${CMAKE_SOURCE_DIR}/Dependancies/Windows/qt64")
    else()
        if(CMAKE_BUILD_TYPE EQUAL "Debug")
            link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Windows/lib32-debug")
        else()
            link_directories("${CMAKE_SOURCE_DIR}/Dependancies/Windows/lib32")
        endif()
        set(QT5_BASE_DIR "${CMAKE_SOURCE_DIR}/Dependancies/Windows/qt32")
    endif()
endif()

# set up install directory
if(BITNESS EQUAL 64)
    if(CMAKE_BUILD_TYPE EQUAL "Debug")
        set(INSTALL_BINARIES_DIRECTORY "bin64-debug")
    else()
        set(INSTALL_BINARIES_DIRECTORY "bin64")
    endif()
else()
    if(CMAKE_BUILD_TYPE EQUAL "Debug")
        set(INSTALL_BINARIES_DIRECTORY "bin32-debug")
    else()
        set(INSTALL_BINARIES_DIRECTORY "bin32")
    endif()
endif()    

# use bundled qt
if(DEFINED QT5_BASE_DIR)
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${QT5_BASE_DIR}/lib/cmake")
    set(Qt5Core_DIR "${QT5_BASE_DIR}/lib/cmake/Qt5Core")
    set(Qt5Gui_DIR "${QT5_BASE_DIR}/lib/cmake/Qt5Gui")
    set(Qt5Widgets_DIR "${QT5_BASE_DIR}/lib/cmake/Qt5Widgets")
endif()

# pull in qt dependancies
if(NOT EMSCRIPTEN AND NOT ANDROID)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Gui COMPONENTS Private REQUIRED)
    find_package(Qt5Widgets REQUIRED)
endif()

# enable c99/ c++11
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGXX OR EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# set cflags that are common to dependancies
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-D_GNU_SOURCE)

# update cflags for debug build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-D_DEBUG)
    message("Building debug executable.")
endif()

# build options
if(EMSCRIPTEN OR ANDROID)
    unset(HAVE_LIBXML2)
    unset(HAVE_SQUISH)
    set(HAVE_ZLIB "1")
    set(HAVE_SFMT "1")
    unset(HAVE_FREEIMAGE)
    unset(HAVE_GLSLCOMPILER)
    set(WITH_PROFILER "1" CACHE STRING "Foo")
    set(WITH_IMGUI "1" CACHE STRING "Foo")
    unset(WITH_RENDERER_D3D11)
    unset(WITH_RENDERER_OPENGL)
    set(WITH_RENDERER_OPENGLES2 "1" CACHE STRING "Foo")
    unset(WITH_RESOURCECOMPILER)
    unset(WITH_RESOURCECOMPILER_EMBEDDED)
    unset(WITH_RESOURCECOMPILER_SUBPROCESS)
    unset(WITH_RESOURCECOMPILER_STANDALONE)
    unset(WITH_MAPCOMPILER)
    unset(WITH_MAPCOMPILER_STANDALONE)
    unset(WITH_CONTENTCONVERTER)
    unset(WITH_CONTENTCONVERTER_STANDALONE)
    set(WITH_BASEGAME "1" CACHE STRING "Foo")
    unset(WITH_EDITOR)
    unset(WITH_BLOCKENGINE)
else()
    set(HAVE_LIBXML2 "1")
    set(HAVE_SQUISH "1")
    set(HAVE_ZLIB "1")
    set(HAVE_SFMT "1")
    set(HAVE_FREEIMAGE "1")
    set(HAVE_GLSLCOMPILER "1")
    set(WITH_PROFILER "1" CACHE STRING "Foo")
    set(WITH_IMGUI "1" CACHE STRING "Foo")
    set(WITH_RENDERER_D3D11 "0" CACHE STRING "Foo")
    set(WITH_RENDERER_OPENGL "1" CACHE STRING "Foo")
    set(WITH_RENDERER_OPENGLES2 "1" CACHE STRING "Foo")
    set(WITH_RESOURCECOMPILER "1" CACHE STRING "Foo")
    set(WITH_RESOURCECOMPILER_EMBEDDED "0" CACHE STRING "Foo")
    set(WITH_RESOURCECOMPILER_SUBPROCESS "1" CACHE STRING "Foo")
    set(WITH_RESOURCECOMPILER_STANDALONE "1" CACHE STRING "Foo")
    set(WITH_MAPCOMPILER "1" CACHE STRING "Foo")
    set(WITH_MAPCOMPILER_STANDALONE "1" CACHE STRING "Foo")
    set(WITH_CONTENTCONVERTER "1" CACHE STRING "Foo")
    set(WITH_CONTENTCONVERTER_STANDALONE "1" CACHE STRING "Foo")
    set(WITH_BASEGAME "1" CACHE STRING "Foo")
    set(WITH_EDITOR "1" CACHE STRING "Foo")
    set(WITH_BLOCKENGINE "1" CACHE STRING "Foo")
endif()

# kill annoying clang warnings
if(CMAKE_COMPILER_IS_CLANGXX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()

# same for clang/emscripten
if(EMSCRIPTEN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
endif()

# compile dependancies without cflags set
add_subdirectory(Engine/Dependancies)

# set up common variables
set(ENGINE_BASE_DIRECTORY "${CMAKE_SOURCE_DIR}/Engine/Source")

# set up cflags
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGXX OR EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

    # kill annoying warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
endif()

# pull in include directories
include_directories("${CMAKE_BINARY_DIR}")

# generate config.h
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)

# engine
add_subdirectory(Engine/Source)

# demogame
add_subdirectory(DemoGame/Source)

# TestGame
add_subdirectory(TestGame/Source)

# BlockGame
if(NOT EMSCRIPTEN AND NOT ANDROID)
    add_subdirectory(BlockGame/Source)
endif()

