set(HEADER_FILES
    AABox.h
    Angle.h
    CollisionDetection.h
    Common.h
    Frustum.h
    HashTraits.h
    Interpolator.h
    Line.h
    Matrixf.h
    Plane.h
    Quaternion.h
    Ray.h
    SIMDMatrixf.h
    SIMDMatrixf_scalar.h
    SIMDMatrixi.h
    SIMDMatrixi_scalar.h
    SIMDVectorf.h
    SIMDVectorf_scalar.h
    SIMDVectorf_sse.h
    SIMDVectori.h
    SIMDVectori_scalar.h
    SIMDVectori_sse.h
    Sphere.h
    StreamOperators.h
    StringConverters.h
    Transform.h
    Vectorf.h
    Vectorh.h
    Vectori.h
    VectorShuffles.h
    Vectoru.h
)

set(SOURCE_FILES
    AABox.cpp
    Angle.cpp
    CollisionDetection.cpp
    Frustum.cpp
    HashTraits.cpp
    Interpolator.cpp
    Matrixf.cpp
    Plane.cpp
    Quaternion.cpp
    Ray.cpp
    SIMDMatrixf_scalar.cpp
    SIMDVectorf_scalar.cpp
    SIMDVectorf_sse.cpp
    SIMDVectori_scalar.cpp
    SIMDVectori_sse.cpp
    Sphere.cpp
    StreamOperators.cpp
    StringConverters.cpp
    Transform.cpp
    Vectorf.cpp
    Vectorh.cpp
    Vectori.cpp
    Vectoru.cpp
)

include_directories(${ENGINE_BASE_DIRECTORY})

add_library(EngineMathLib STATIC ${HEADER_FILES} ${SOURCE_FILES})
                    
target_link_libraries(EngineMathLib
                      YBaseLib)

