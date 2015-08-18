#pragma once
#include "Core/Common.h"

#ifdef HAVE_SFMT

// uses mersenne twister
class RandomNumberGenerator
{
public:
    // constructor
    RandomNumberGenerator();
    RandomNumberGenerator(uint32 seed);
    RandomNumberGenerator(const byte *pKey, uint32 keyLength);
    RandomNumberGenerator(const RandomNumberGenerator &copy);
    ~RandomNumberGenerator();

    // reinitialize state
    void Reseed();
    void Reseed(uint32 seed);
    void Reseed(const byte *pKey, uint32 keyLength);

    // copier
    RandomNumberGenerator &operator=(const RandomNumberGenerator &copy);

    // uniform-based generators
    uint32 NextUInt();
    float NextUniformFloat();
    double NextUniformDouble();

    // gaussian i.e. -1 >= result <= 1
    float NextGaussianFloat();
    double NextGaussianDouble();

    // range-based generators
    int32 NextRangeInt(int32 min, int32 max);
    uint32 NextRangeUInt(uint32 min, uint32 max);
    float NextRangeFloat(float min, float max);
    double NextRangeDouble(double min, double max);

    // test a chance-based number, chance is expected to be 0 >= chance <= 100
    bool NextBoolean(float chance);

private:
    void AllocateState();
    void FreeState();

    struct SFMT_T *m_state;
};

#endif      // HAVE_SFMT
