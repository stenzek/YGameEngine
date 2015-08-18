#pragma once
#include "MathLib/Common.h"

// Float vector classes on SSE+
#if Y_CPU_SSE_LEVEL > 0
    #include "MathLib/SIMDVectorf_sse.h"
#else
    #include "MathLib/SIMDVectorf_scalar.h"
#endif
