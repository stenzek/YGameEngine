set(HEADER_FILES
    BSPTree.h
    ChunkDataFormat.h
    ChunkFileReader.h
    ChunkFileWriter.h
    ClassTableDataFormat.h
    ClassTable.h
    Common.h
    Console.h
    DDSFormat.h
    DDSReader.h
    DDSWriter.h
    DefinitionFile.h
    FIFVolume.h
    ImageCodec.h
    Image.h
    KDTree.h
    MeshUtilties.h
    Object.h
    ObjectSerializer.h
    ObjectTypeInfo.h
    PixelFormat.h
    PrecompiledHeader.h
    Property.h
    PropertyTable.h
    PropertyTemplate.h
    RandomNumberGenerator.h
    Resource.h
    ResourceTypeInfo.h
    TexturePacker.h
    TypeRegistry.h
    VirtualFileSystem.h
    XDisplay.h
)

set(SOURCE_FILES
    ChunkFileReader.cpp
    ChunkFileWriter.cpp
    ClassTable.cpp
    Console.cpp
    DDSReader.cpp
    DDSWriter.cpp
    DefinitionFile.cpp
    FIFVolume.cpp
    ImageCodecBMP.cpp
    ImageCodec.cpp
    ImageCodecDDS.cpp
    ImageCodecDevIL.cpp
    ImageCodecFreeImage.cpp
    ImageCodecJPEG.cpp
    Image.cpp
    MeshUtilties.cpp
    Object.cpp
    ObjectSerializer.cpp
    ObjectTypeInfo.cpp
    PixelFormatConverters.cpp
    PixelFormat.cpp
    PrecompiledHeader.cpp
    Property.cpp
    PropertyTable.cpp
    PropertyTemplate.cpp
    RandomNumberGenerator.cpp
    Resource.cpp
    ResourceTypeInfo.cpp
    TexturePacker.cpp
    VirtualFileSystem.cpp
    XDisplay.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY}
                    ${PCRE_INCLUDE_DIR})

set(EXTRA_LIBRARIES "")
if(HAVE_SQUISH)
    LIST(APPEND EXTRA_LIBRARIES squish)
endif()
if(HAVE_FREEIMAGE)
    LIST(APPEND EXTRA_LIBRARIES freeimage)
endif()

add_library(EngineCore STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(EngineCore
                      fif
                      SFMT
                      YBaseLib
                      EngineMathLib
                      ${EXTRA_LIBRARIES}
                      ${PCRE_LIBRARIES}) 


