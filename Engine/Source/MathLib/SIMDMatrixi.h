#pragma once
#include "MathLib/Common.h"

// Integer matrix classes only available with SSE2.
//#if Y_CPU_SSE_LEVEL >= 2
    //#include "MathLib/SIMDMatrixi_sse.h"
//#else
    #include "MathLib/SIMDMatrixi_scalar.h"
//#endif
