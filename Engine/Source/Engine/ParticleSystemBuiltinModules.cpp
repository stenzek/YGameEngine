#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemBuiltinModules.h"
#include "Core/RandomNumberGenerator.h"
Log_SetChannel(ParticleSystemBuiltinModules);

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_TimeBased);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_TimeBased)
    PROPERTY_TABLE_MEMBER_FLOAT("TimeFraction", 0, offsetof(ParticleSystemModule_TimeBased, m_timeFraction), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("EasingFunction", 0, offsetof(ParticleSystemModule_TimeBased, m_easingFunction), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_TimeBased::ParticleSystemModule_TimeBased()
    : m_timeFraction(1.0f),
      m_easingFunction(EasingFunction::Linear)
{

}

ParticleSystemModule_TimeBased::~ParticleSystemModule_TimeBased()
{

}

float ParticleSystemModule_TimeBased::GetCoefficient(const ParticleData *pParticle) const
{
    float coefficient = Min(pParticle->LifeRemaining / (pParticle->LifeSpan * m_timeFraction), 1.0f);
    return EasingFunction::GetCoefficient((EasingFunction::Type)m_easingFunction, coefficient);
}

float ParticleSystemModule_TimeBased::GetInverseCoefficient(const ParticleData *pParticle) const
{
    float coefficient = Min(pParticle->LifeRemaining / (pParticle->LifeSpan * m_timeFraction), 1.0f);
    return 1.0f - EasingFunction::GetCoefficient((EasingFunction::Type)m_easingFunction, coefficient);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnLifetime);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnLifetime);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnLifetime)
    PROPERTY_TABLE_MEMBER_FLOAT("MinTime", 0, offsetof(ParticleSystemModule_SpawnLifetime, m_minTime), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("MaxTime", 0, offsetof(ParticleSystemModule_SpawnLifetime, m_maxTime), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_SpawnLifetime::ParticleSystemModule_SpawnLifetime()
    : ParticleSystemModule(),
      m_minTime(1.0f),
      m_maxTime(1.0f)
{

}

ParticleSystemModule_SpawnLifetime::~ParticleSystemModule_SpawnLifetime()
{

}

bool ParticleSystemModule_SpawnLifetime::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    // update the time to live of the particle
    float timeToLive = (m_minTime == m_maxTime) ? m_minTime : pRNG->NextRangeFloat(m_minTime, m_maxTime);
    pParticle->LifeSpan = timeToLive;
    pParticle->LifeRemaining = timeToLive;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnLocation);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnLocation);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnLocation)
    PROPERTY_TABLE_MEMBER_UINT("LocationType", 0, offsetof(ParticleSystemModule_SpawnLocation, m_spawnLocationType), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT3("PointPosition", 0, offsetof(ParticleSystemModule_SpawnLocation, m_spawnLocationProperties.Fixed.Position), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_SpawnLocation::ParticleSystemModule_SpawnLocation()
    : ParticleSystemModule(),
      m_spawnLocationType(LocationType_Point)
{
    Y_memzero(&m_spawnLocationProperties, sizeof(m_spawnLocationProperties));
}

ParticleSystemModule_SpawnLocation::~ParticleSystemModule_SpawnLocation()
{

}

void ParticleSystemModule_SpawnLocation::SetSpawnLocationPoint(const float3 &point)
{
    m_spawnLocationType = LocationType_Point;
    point.Store(m_spawnLocationProperties.Fixed.Position);
}

void ParticleSystemModule_SpawnLocation::SetSpawnLocationBox(const float3 &boxMinBounds, const float3 &boxMaxBounds)
{
    m_spawnLocationType = LocationType_Box;
    boxMinBounds.Store(m_spawnLocationProperties.Box.MinBounds);
    boxMaxBounds.Store(m_spawnLocationProperties.Box.MaxBounds);
}

void ParticleSystemModule_SpawnLocation::SetSpawnLocationSphere(const float3 &sphereCenter, float sphereRadius)
{
    m_spawnLocationType = LocationType_Sphere;
    sphereCenter.Store(m_spawnLocationProperties.Sphere.Center);
    m_spawnLocationProperties.Sphere.Radius = sphereRadius;
}

void ParticleSystemModule_SpawnLocation::SetSpawnLocationCylinder(const float3 &cylinderBase, float width, float height, uint32 heightAxis)
{
    m_spawnLocationType = LocationType_Cylinder;
    cylinderBase.Store(m_spawnLocationProperties.Cylinder.Center);
    m_spawnLocationProperties.Cylinder.Width = width;
    m_spawnLocationProperties.Cylinder.Height = height;
    m_spawnLocationProperties.Cylinder.HeightAxis = heightAxis;
}

void ParticleSystemModule_SpawnLocation::SetSpawnLocationTriangle(const float3 &triangleBaseCenter, float height, float depth)
{
    m_spawnLocationType = LocationType_Triangle;
    triangleBaseCenter.Store(m_spawnLocationProperties.Triangle.BaseCenter);
    m_spawnLocationProperties.Triangle.Height = height;
    m_spawnLocationProperties.Triangle.Depth = depth;
}

bool ParticleSystemModule_SpawnLocation::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    switch (m_spawnLocationType)
    {
    case LocationType_Point:
        pParticle->Position = pBaseTransform->TransformPoint(float3(m_spawnLocationProperties.Fixed.Position));
        break;

    case LocationType_Box:
        {
            SIMDVector3f minBounds(m_spawnLocationProperties.Box.MinBounds);
            SIMDVector3f maxBounds(m_spawnLocationProperties.Box.MaxBounds);
            SIMDVector3f rng(pRNG->NextUniformFloat(), pRNG->NextUniformFloat(), pRNG->NextUniformFloat());
            pParticle->Position = pBaseTransform->TransformPoint(minBounds + ((maxBounds - minBounds) * rng));
        }
        break;

    case LocationType_Sphere:
        {
            float radius = m_spawnLocationProperties.Sphere.Radius;
            float radiusSq = radius * radius;
            float3 location;
            do
            {
                location.Set(pRNG->NextRangeFloat(-radius, radius),
                             pRNG->NextRangeFloat(-radius, radius),
                             pRNG->NextRangeFloat(-radius, radius));
            }
            while (location.SquaredLength() > radiusSq);
            pParticle->Position = location;
        }
        break;

    case LocationType_Cylinder:
        break;
        
    case LocationType_Triangle:
        break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnSize);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnSize);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnSize)
    PROPERTY_TABLE_MEMBER_FLOAT("MinWidth", 0, offsetof(ParticleSystemModule_SpawnSize, m_minWidth), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("MaxWidth", 0, offsetof(ParticleSystemModule_SpawnSize, m_maxWidth), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("MinHeight", 0, offsetof(ParticleSystemModule_SpawnSize, m_minHeight), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("MaxHeight", 0, offsetof(ParticleSystemModule_SpawnSize, m_maxHeight), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_SpawnSize::ParticleSystemModule_SpawnSize()
    : m_minWidth(1.0f),
      m_maxWidth(1.0f),
      m_minHeight(1.0f),
      m_maxHeight(1.0f)
{

}

ParticleSystemModule_SpawnSize::~ParticleSystemModule_SpawnSize()
{

}

bool ParticleSystemModule_SpawnSize::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    pParticle->Width = (m_minWidth == m_maxWidth) ? m_minWidth : pRNG->NextRangeFloat(m_minWidth, m_maxWidth);
    pParticle->Height = (m_minHeight == m_maxHeight) ? m_minHeight : pRNG->NextRangeFloat(m_minHeight, m_maxHeight);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnVelocity);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnVelocity);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnVelocity)
    PROPERTY_TABLE_MEMBER_UINT("Type", 0, offsetof(ParticleSystemModule_SpawnVelocity, m_spawnVelocityType), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT3("FixedVelocity", 0, offsetof(ParticleSystemModule_SpawnVelocity, m_spawnVelocityProperties.Fixed.Velocity), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()


ParticleSystemModule_SpawnVelocity::ParticleSystemModule_SpawnVelocity()
    : ParticleSystemModule(),
      m_spawnVelocityType(SpawnVelocityType_Fixed)
{
    Y_memzero(&m_spawnVelocityProperties, sizeof(m_spawnVelocityProperties));
}

ParticleSystemModule_SpawnVelocity::~ParticleSystemModule_SpawnVelocity()
{

}

void ParticleSystemModule_SpawnVelocity::SetSpawnVelocityFixed(const float3 &fixedVelocity)
{
    m_spawnVelocityType = SpawnVelocityType_Fixed;
    fixedVelocity.Store(m_spawnVelocityProperties.Fixed.Velocity);
}

void ParticleSystemModule_SpawnVelocity::SetSpawnVelocityArc(float minAngle, float maxAngle, float distance, uint32 axis)
{

}

void ParticleSystemModule_SpawnVelocity::SetSpawnVelocityRandom(float minSpeed, float maxSpeed)
{
    m_spawnVelocityType = SpawnVelocityType_Random;
    m_spawnVelocityProperties.Random.MinSpeed = minSpeed;
    m_spawnVelocityProperties.Random.MaxSpeed = maxSpeed;
}

void ParticleSystemModule_SpawnVelocity::SetSpawnVelocityRandomFixedAxis(float minSpeed, float maxSpeed, uint32 axis)
{
    m_spawnVelocityType = SpawnVelocityType_RandomFixedAxis;
    m_spawnVelocityProperties.RandomFixedAxis.MinSpeed = minSpeed;
    m_spawnVelocityProperties.RandomFixedAxis.MaxSpeed = maxSpeed;
    m_spawnVelocityProperties.RandomFixedAxis.Axis = axis;
}

bool ParticleSystemModule_SpawnVelocity::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    switch (m_spawnVelocityType)
    {
    case SpawnVelocityType_Fixed:
        pParticle->Velocity = float3(m_spawnVelocityProperties.Fixed.Velocity);
        break;

    case SpawnVelocityType_Arc:
        break;

    case SpawnVelocityType_Random:
        {
            // get the vector first
            SIMDVector3f spawnVector(pRNG->NextGaussianFloat(), pRNG->NextGaussianFloat(), pRNG->NextGaussianFloat());

            // normalize it, and set result
            pParticle->Velocity = spawnVector.Normalize() * pRNG->NextRangeFloat(m_spawnVelocityProperties.Random.MinSpeed, m_spawnVelocityProperties.Random.MaxSpeed);
        }
        break;

    case SpawnVelocityType_RandomFixedAxis:
        {
            // get the vector first, set the axis
            static const float3 *lookupVectors[3] = { &float3::UnitX, &float3::UnitX, &float3::UnitZ };
            DebugAssert(m_spawnVelocityProperties.RandomFixedAxis.Axis < 3);
            pParticle->Velocity = *lookupVectors[m_spawnVelocityProperties.RandomFixedAxis.Axis] * pRNG->NextRangeFloat(m_spawnVelocityProperties.RandomFixedAxis.MinSpeed, m_spawnVelocityProperties.RandomFixedAxis.MaxSpeed);
        }
        break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnRotation);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnRotation);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnRotation)
    PROPERTY_TABLE_MEMBER_FLOAT("MinRotation", 0, offsetof(ParticleSystemModule_SpawnRotation, m_minRotation), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("MaxRotation", 0, offsetof(ParticleSystemModule_SpawnRotation, m_maxRotation), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_SpawnRotation::ParticleSystemModule_SpawnRotation()
    : m_minRotation(0.0f), 
      m_maxRotation(0.0f)
{

}

ParticleSystemModule_SpawnRotation::~ParticleSystemModule_SpawnRotation()
{

}

bool ParticleSystemModule_SpawnRotation::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    pParticle->Rotation = (m_minRotation == m_maxRotation) ? m_minRotation : pRNG->NextRangeFloat(m_minRotation, m_maxRotation);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_LockToEmitter);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_LockToEmitter);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_LockToEmitter)
    PROPERTY_TABLE_MEMBER_FLOAT3("OffsetPosition", 0, offsetof(ParticleSystemModule_LockToEmitter, m_offsetPosition), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_LockToEmitter::ParticleSystemModule_LockToEmitter()
    : m_offsetPosition(float3::Zero)
{

}

ParticleSystemModule_LockToEmitter::~ParticleSystemModule_LockToEmitter()
{

}

bool ParticleSystemModule_LockToEmitter::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    // initialize position
    pParticle->Position = pBaseTransform->TransformPoint(m_offsetPosition);
    return true;
}

void ParticleSystemModule_LockToEmitter::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];
        pParticle->Position = pBaseTransform->TransformPoint(m_offsetPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_FadeOut);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_FadeOut);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_FadeOut)
    PROPERTY_TABLE_MEMBER_FLOAT("StartFadeTime", 0, offsetof(ParticleSystemModule_FadeOut, m_startFadeTime), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_FadeOut::ParticleSystemModule_FadeOut()
    : m_startFadeTime(0.0f)
{

}

ParticleSystemModule_FadeOut::~ParticleSystemModule_FadeOut()
{

}

bool ParticleSystemModule_FadeOut::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    // set alpha field to full
    pParticle->Color |= MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255);
    return true;
}

void ParticleSystemModule_FadeOut::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];

        // handle start fade-out time
        float elapsed = pParticle->LifeSpan - pParticle->LifeRemaining;
        if (elapsed < m_startFadeTime)
        {
            pParticle->Color |= 0xFF000000;
            continue;
        }

        float fraction = 1.0f - (pParticle->LifeRemaining / (pParticle->LifeSpan - m_startFadeTime));
        pParticle->Color = (pParticle->Color & 0x00FFFFFF) | ((uint32)Math::Truncate(fraction * 255.0f) << 24);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_Flipbook);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_Flipbook);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_Flipbook)
    PROPERTY_TABLE_MEMBER_UINT("Columns", 0, offsetof(ParticleSystemModule_Flipbook, m_columns), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("Rows", 0, offsetof(ParticleSystemModule_Flipbook, m_rows), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("FlipInterval", 0, offsetof(ParticleSystemModule_Flipbook, m_flipInterval), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_Flipbook::ParticleSystemModule_Flipbook()
    : m_columns(1),
      m_rows(1),
      m_flipInterval(1.0f)
{

}

ParticleSystemModule_Flipbook::~ParticleSystemModule_Flipbook()
{

}

float2 ParticleSystemModule_Flipbook::GetTextureCoordinateRange() const
{
    DebugAssert(m_columns > 0 && m_rows > 0);
    return float2(1.0f / (float)m_columns, 1.0f / (float)m_rows);
}

float2 ParticleSystemModule_Flipbook::GetMinTextureCoordinates(uint32 frameNumber) const
{
    DebugAssert(m_columns > 0);
    uint32 imageX = frameNumber % m_columns;
    uint32 imageY = frameNumber / m_columns;
    float2 textureCoordinateRange(GetTextureCoordinateRange());
    return float2(textureCoordinateRange.x * (float)imageX,
                  textureCoordinateRange.y * (float)imageY);
}

float2 ParticleSystemModule_Flipbook::GetMaxTextureCoordinates(uint32 frameNumber) const
{
    DebugAssert(m_columns > 0);
    uint32 imageX = frameNumber % m_columns;
    uint32 imageY = frameNumber / m_columns;
    float2 textureCoordinateRange(GetTextureCoordinateRange());
    return float2(textureCoordinateRange.x * (float)imageX + textureCoordinateRange.x,
                  textureCoordinateRange.y * (float)imageY + textureCoordinateRange.y);
}

bool ParticleSystemModule_Flipbook::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    // Start at the first frame
    pParticle->MinTextureCoordinates = GetMinTextureCoordinates(0);
    pParticle->MaxTextureCoordinates = GetMaxTextureCoordinates(0);
    return true;
}

void ParticleSystemModule_Flipbook::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    // Precalculate texture coord range, and the number of frames
    float2 textureCoordinateRange(GetTextureCoordinateRange());
    uint32 frameCount = m_columns * m_rows;
    float inverseFlipInterval = 1.0f / m_flipInterval;
    
    // For each particle, calculate the frame number
    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];
        
        // calculate fraction of time complete
        float timeElapsed = pParticle->LifeSpan - pParticle->LifeRemaining;
        float frameNumberF = timeElapsed * inverseFlipInterval;
        uint32 frameNumber = Math::Truncate(Math::Floor(frameNumberF)) % frameCount;
        DebugAssert(frameNumber < frameCount);

        // update the uv range for the particle
        uint32 imageX = frameNumber % m_columns;
        uint32 imageY = frameNumber / m_columns;
        pParticle->MinTextureCoordinates.Set(textureCoordinateRange.x * (float)imageX,
                                             textureCoordinateRange.y * (float)imageY);
        pParticle->MaxTextureCoordinates.Set(pParticle->MinTextureCoordinates + textureCoordinateRange);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_ColorOverTime);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_ColorOverTime);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_ColorOverTime)
    PROPERTY_TABLE_MEMBER_COLOR("StartColor", 0, offsetof(ParticleSystemModule_ColorOverTime, m_startColor), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_COLOR("EndColor", 0, offsetof(ParticleSystemModule_ColorOverTime, m_endColor), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_ColorOverTime::ParticleSystemModule_ColorOverTime()
    : m_startColor(MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255)),
      m_endColor(MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255))
{

}

ParticleSystemModule_ColorOverTime::~ParticleSystemModule_ColorOverTime()
{

}

bool ParticleSystemModule_ColorOverTime::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    pParticle->Color = m_startColor;
    return true;
}

void ParticleSystemModule_ColorOverTime::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    // fixme: lerp on int
    float3 startColorFloat(PixelFormatHelpers::ConvertRGBAToFloat4(m_startColor).xyz());
    float3 endColorFloat(PixelFormatHelpers::ConvertRGBAToFloat4(m_endColor).xyz());
    float3 colorRange(endColorFloat - startColorFloat);

    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];

        float timeFraction = GetInverseCoefficient(pParticle);
        pParticle->Color = (pParticle->Color & 0xFF000000) | PixelFormatHelpers::ConvertFloat4ToRGBA(float4(startColorFloat + colorRange * timeFraction, 0.0f));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_OpacityOverTime);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_OpacityOverTime);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_OpacityOverTime)
    PROPERTY_TABLE_MEMBER_FLOAT("StartOpacity", 0, offsetof(ParticleSystemModule_OpacityOverTime, m_startOpacity), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("EndOpacity", 0, offsetof(ParticleSystemModule_OpacityOverTime, m_endOpacity), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_OpacityOverTime::ParticleSystemModule_OpacityOverTime()
    : m_startOpacity(1.0f),
      m_endOpacity(1.0f)
{

}

ParticleSystemModule_OpacityOverTime::~ParticleSystemModule_OpacityOverTime()
{

}

bool ParticleSystemModule_OpacityOverTime::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    pParticle->Color = (pParticle->Color & 0x00FFFFFF) | ((uint32)Math::Clamp(Math::Truncate(m_startOpacity * 255.0f), 0, 255) << 24);
    return true;
}

void ParticleSystemModule_OpacityOverTime::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    float opacityRange = m_endOpacity - m_startOpacity;

    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];

        float particleOpacity = GetInverseCoefficient(pParticle) * opacityRange + m_startOpacity;
        pParticle->Color = (pParticle->Color & 0x00FFFFFF) | ((uint32)Math::Clamp(Math::Truncate(particleOpacity * 255.0f), 0, 255) << 24);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_SizeOverTime);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SizeOverTime);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_SizeOverTime)
    PROPERTY_TABLE_MEMBER_FLOAT("StartWidth", 0, offsetof(ParticleSystemModule_SizeOverTime, m_startWidth), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("EndWidth", 0, offsetof(ParticleSystemModule_SizeOverTime, m_endWidth), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("StartHeight", 0, offsetof(ParticleSystemModule_SizeOverTime, m_startHeight), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("EndHeight", 0, offsetof(ParticleSystemModule_SizeOverTime, m_endHeight), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_SizeOverTime::ParticleSystemModule_SizeOverTime()
    : m_startWidth(1.0f),
      m_endWidth(1.0f),
      m_startHeight(1.0f),
      m_endHeight(1.0f)
{

}

ParticleSystemModule_SizeOverTime::~ParticleSystemModule_SizeOverTime()
{

}

void ParticleSystemModule_SizeOverTime::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    float widthRange = m_endWidth - m_startWidth;
    float heightRange = m_endHeight - m_startHeight;

    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];

        float fraction = GetInverseCoefficient(pParticle);
        pParticle->Width = m_startWidth + widthRange * fraction;
        pParticle->Height = m_startHeight + heightRange * fraction;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_RotationSpeed);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_RotationSpeed);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_RotationSpeed)
    PROPERTY_TABLE_MEMBER_FLOAT("RotationSpeed", 0, offsetof(ParticleSystemModule_RotationSpeed, m_rotationSpeed), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_RotationSpeed::ParticleSystemModule_RotationSpeed()
    : m_rotationSpeed(0.0f)
{

}

ParticleSystemModule_RotationSpeed::~ParticleSystemModule_RotationSpeed()
{

}

void ParticleSystemModule_RotationSpeed::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    float rotationDelta = m_rotationSpeed * deltaTime;

    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];
        pParticle->Rotation += rotationDelta;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule_RotationOverTime);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_RotationOverTime);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemModule_RotationOverTime)
    PROPERTY_TABLE_MEMBER_FLOAT("StartRotation", 0, offsetof(ParticleSystemModule_RotationOverTime, m_startRotation), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("EndRotation", 0, offsetof(ParticleSystemModule_RotationOverTime, m_endRotation), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemModule_RotationOverTime::ParticleSystemModule_RotationOverTime()
    : ParticleSystemModule_TimeBased(),
      m_startRotation(0.0f),
      m_endRotation(0.0f)
{

}

ParticleSystemModule_RotationOverTime::~ParticleSystemModule_RotationOverTime()
{

}

bool ParticleSystemModule_RotationOverTime::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    pParticle->Rotation = m_startRotation;
    return true;
}

void ParticleSystemModule_RotationOverTime::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{
    float rotationRange = m_endRotation - m_startRotation;

    for (uint32 particleIndex = 0; particleIndex < nParticles; particleIndex++)
    {
        ParticleData *pParticle = &pParticles[particleIndex];

        float particleRotation = GetInverseCoefficient(pParticle) * rotationRange + m_startRotation;
        pParticle->Rotation = particleRotation;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ParticleSystemModule::RegisterBuiltinModules()
{
    static bool called = false;
    DebugAssert(!called);
    called = true;

#define REGISTER_TYPE(Type) Type::StaticMutableTypeInfo()->RegisterType()

    REGISTER_TYPE(ParticleSystemModule_TimeBased);
    REGISTER_TYPE(ParticleSystemModule_SpawnLifetime);
    REGISTER_TYPE(ParticleSystemModule_SpawnLocation);
    REGISTER_TYPE(ParticleSystemModule_SpawnSize);
    REGISTER_TYPE(ParticleSystemModule_SpawnVelocity);
    REGISTER_TYPE(ParticleSystemModule_SpawnRotation);
    REGISTER_TYPE(ParticleSystemModule_LockToEmitter);
    REGISTER_TYPE(ParticleSystemModule_FadeOut);
    REGISTER_TYPE(ParticleSystemModule_Flipbook);
    REGISTER_TYPE(ParticleSystemModule_ColorOverTime);
    REGISTER_TYPE(ParticleSystemModule_OpacityOverTime);
    REGISTER_TYPE(ParticleSystemModule_SizeOverTime);
    REGISTER_TYPE(ParticleSystemModule_RotationSpeed);
    REGISTER_TYPE(ParticleSystemModule_RotationOverTime);

#undef REGISTER_TYPE
}
