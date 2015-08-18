#pragma once
#include "Engine/Common.h"

// Forward declarations of all relevant classes
struct ParticleData;
class ParticleSystem;
class ParticleSystemEmitter;
class ParticleSystemModule;

// Strictly contains the data of particle instances, as compact as possible
struct ParticleData
{
    // Current information
    float3 Position;
    float3 Velocity;
    float Width;
    float Height;
    float Rotation;
    uint32 Color;
    float2 MinTextureCoordinates;
    float2 MaxTextureCoordinates;
    float LifeSpan;
    float LifeRemaining;

    // Light information
    uint8 LightType;
    uint8 LightParameter;
    float LightRange;
    float LightFalloff;
};
