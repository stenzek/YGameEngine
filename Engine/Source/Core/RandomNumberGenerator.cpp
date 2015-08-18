#include "Core/PrecompiledHeader.h"
#include "Core/RandomNumberGenerator.h"

#ifdef HAVE_SFMT
#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"
#include <ctime>
#include <cmath>

// Pull in SFMT stuff
#define SFMT_MEXP 19937
#if Y_CPU_SSE_LEVEL > 0
    #define HAVE_SSE2
#endif
#include "SFMT.h"

static uint32 GetInitSeed()
{
    // fixme: better seed
    return (uint32_t)time(NULL);
}

RandomNumberGenerator::RandomNumberGenerator()
{
    AllocateState();
    sfmt_init_gen_rand(m_state, GetInitSeed());
}

RandomNumberGenerator::RandomNumberGenerator(uint32 seed)
{
    AllocateState();
    sfmt_init_gen_rand(m_state, seed);
}

RandomNumberGenerator::RandomNumberGenerator(const byte *pKey, uint32 keyLength)
{
    AllocateState();
    sfmt_init_by_array(m_state, const_cast<uint32_t *>(reinterpret_cast<const uint32_t *>(pKey)), keyLength / 4);
}

RandomNumberGenerator::RandomNumberGenerator(const RandomNumberGenerator &copy)
{
    AllocateState();
    Y_memcpy(m_state, copy.m_state, sizeof(sfmt_t));
}

RandomNumberGenerator::~RandomNumberGenerator()
{
    FreeState();
}

void RandomNumberGenerator::AllocateState()
{
#if Y_CPU_SSE_LEVEL > 0
    m_state = (sfmt_t *)Y_aligned_malloc(sizeof(sfmt_t), Y_SSE_ALIGNMENT);
#else
    m_state = (sfmt_t *)Y_malloc(sizeof(sfmt_t));
#endif
}

void RandomNumberGenerator::FreeState()
{
#if Y_CPU_SSE_LEVEL > 0
    Y_aligned_free(m_state);
#else
    Y_free(m_state);
#endif
}

void RandomNumberGenerator::Reseed()
{
    sfmt_init_gen_rand(m_state, GetInitSeed());
}

void RandomNumberGenerator::Reseed(uint32 seed)
{
    sfmt_init_gen_rand(m_state, seed);
}

void RandomNumberGenerator::Reseed(const byte *pKey, uint32 keyLength)
{
    sfmt_init_by_array(m_state, const_cast<uint32_t *>(reinterpret_cast<const uint32_t *>(pKey)), keyLength / 4);
}

RandomNumberGenerator &RandomNumberGenerator::operator=(const RandomNumberGenerator &copy)
{
    Y_memcpy(m_state, copy.m_state, sizeof(sfmt_t));
    return *this;
}

uint32 RandomNumberGenerator::NextUInt()
{
    return sfmt_genrand_uint32(m_state);
}

float RandomNumberGenerator::NextUniformFloat()
{
    return (float)sfmt_genrand_real1(m_state);
}

double RandomNumberGenerator::NextUniformDouble()
{
    return sfmt_genrand_real1(m_state);
}

float RandomNumberGenerator::NextGaussianFloat()
{
    float v1, v2, s;
    do
    {
        v1 = 2.0f * NextUniformFloat() - 1.0f;
        v2 = 2.0f * NextUniformFloat() - 1.0f;
        s = v1 * v1 + v2 * v2;
    }
    while (s >= 1.0f || s == 0.0f);

    float multiplier = sqrtf(-2.0f * logf(s) / s);
    return v1 * multiplier;
}

double RandomNumberGenerator::NextGaussianDouble()
{
    double v1, v2, s;
    do
    {
        v1 = 2.0 * NextUniformDouble() - 1.0;
        v2 = 2.0 * NextUniformDouble() - 1.0;
        s = v1 * v1 + v2 * v2;
    }
    while (s >= 1.0 || s == 0.0);

    double multiplier = sqrt(-2.0 * log(s) / s);
    return v1 * multiplier;
}

int32 RandomNumberGenerator::NextRangeInt(int32 min, int32 max)
{
    uint32 range = (uint32)(max - min);
    DebugAssert(range > 0);

    uint32 val = NextUInt() % range;
    return (int32)val + min;
}

uint32 RandomNumberGenerator::NextRangeUInt(uint32 min, uint32 max)
{
    uint32 range = (uint32)(max - min);
    DebugAssert(range > 0);

    uint32 val = NextUInt() % range;
    return val + min;
}

float RandomNumberGenerator::NextRangeFloat(float min, float max)
{
    return NextUniformFloat() * (max - min) + min;
}

double RandomNumberGenerator::NextRangeDouble(double min, double max)
{
    return NextUniformDouble() * (max - min) + min;
}

bool RandomNumberGenerator::NextBoolean(float chance)
{
    double val = NextUniformDouble();
    return (val >= 0.5);
}

#endif      // HAVE_SFMT
