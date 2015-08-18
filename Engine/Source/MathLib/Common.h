#pragma once

// Import YStdLib, possibly move SSE stuff to here?
#include "YBaseLib/Common.h"
#include "YBaseLib/Math.h"

// Global stuff, this really should be moved elsewhere though...

// this is intentionally the same as the cubemap order
enum CUBE_FACE
{
    CUBE_FACE_RIGHT,            // 0: positive-x
    CUBE_FACE_LEFT,             // 1: negative-x
    CUBE_FACE_BACK,             // 2: positive-y
    CUBE_FACE_FRONT,            // 3: negative-y
    CUBE_FACE_TOP,              // 4: positive-z
    CUBE_FACE_BOTTOM,           // 5: negative-z
    CUBE_FACE_COUNT,
};

enum COORDINATE_SYSTEM
{
    COORDINATE_SYSTEM_Y_UP_LH,
    COORDINATE_SYSTEM_Y_UP_RH,
    COORDINATE_SYSTEM_Z_UP_LH,
    COORDINATE_SYSTEM_Z_UP_RH,
    COORDINATE_SYSTEM_COUNT,
};
