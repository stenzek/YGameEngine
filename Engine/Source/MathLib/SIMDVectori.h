#pragma once
#include "MathLib/Common.h"

// Integer vector classes only available with SSE2.
#if Y_CPU_SSE_LEVEL >= 2
    #include "MathLib/SIMDVectori_sse.h"
#else
    #include "MathLib/SIMDVectori_scalar.h"
#endif
