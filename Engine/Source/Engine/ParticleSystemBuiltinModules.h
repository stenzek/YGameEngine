#pragma once
#include "Engine/ParticleSystemModule.h"

class ParticleSystemModule_TimeBased : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_TimeBased, ParticleSystemModule);
    DECLARE_OBJECT_NO_FACTORY(ParticleSystemModule_TimeBased);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_TimeBased);

public:
    ParticleSystemModule_TimeBased();
    virtual ~ParticleSystemModule_TimeBased();

    const float GetTimeFraction() const { return m_timeFraction; }
    void SetTimeFraction(float timeFraction) { m_timeFraction = timeFraction; }

    const EasingFunction::Type GetEasingFunction() const { return (EasingFunction::Type)m_easingFunction; }
    void SetEasingFunction(EasingFunction::Type easingFunction) { m_easingFunction = easingFunction; }

protected:
    // Helper to get a coefficient for a particle's time, ie fraction of time passed
    float GetCoefficient(const ParticleData *pParticle) const;

    // Helper to get the inverse coefficient for a particle's time, ie fraction of time remaining
    float GetInverseCoefficient(const ParticleData *pParticle) const;

    float m_timeFraction;
    uint32 m_easingFunction;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SpawnLifetime : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnLifetime, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnLifetime);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnLifetime);

public:
    ParticleSystemModule_SpawnLifetime();
    virtual ~ParticleSystemModule_SpawnLifetime();

    const float GetMinTime() const { return m_minTime; }
    const float GetMaxTime() const { return m_maxTime; }
    void SetMinTime(float minTime) { m_minTime = minTime; }
    void SetMaxTime(float maxTime) { m_maxTime = maxTime; }
    void SetTime(float minTime, float maxTime) { m_minTime = minTime; m_maxTime = maxTime; }

    // Only affects creation of particles
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

private:
    float m_minTime;
    float m_maxTime;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SpawnLocation : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnLocation, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnLocation);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnLocation);

public:
    // Emitter starting location types
    enum SpawnLocationType
    {
        LocationType_Point,
        LocationType_Box,
        LocationType_Sphere,
        LocationType_Cylinder,
        LocationType_Triangle,
        LocationType_Count
    };

public:
    ParticleSystemModule_SpawnLocation();
    virtual ~ParticleSystemModule_SpawnLocation();

    // Spawn location
    const SpawnLocationType GetSpawnLocationType() const { return (SpawnLocationType)m_spawnLocationType; }
    void SetSpawnLocationPoint(const float3 &point);
    void SetSpawnLocationBox(const float3 &boxMinBounds, const float3 &boxMaxBounds);
    void SetSpawnLocationSphere(const float3 &sphereCenter, float sphereRadius);
    void SetSpawnLocationCylinder(const float3 &cylinderBase, float width, float height, uint32 heightAxis);
    void SetSpawnLocationTriangle(const float3 &triangleBaseCenter, float height, float depth);

    // Only affects creation of particles
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

private:
    // Spawn location setup
    uint32 m_spawnLocationType;
    struct
    {
        struct
        {
            float Position[3];
        } Fixed;
        struct
        {
            float MinBounds[3];
            float MaxBounds[3];
        } Box;
        struct
        {
            float Center[3];
            float Radius;
        } Sphere;
        struct
        {
            float Center[3];
            float Width;
            float Height;
            uint32 HeightAxis;
        } Cylinder;
        struct
        {
            float BaseCenter[3];
            float Height;
            float Depth;
        } Triangle;
    } m_spawnLocationProperties;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SpawnSize : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnSize, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnSize);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnSize);

public:
    ParticleSystemModule_SpawnSize();
    virtual ~ParticleSystemModule_SpawnSize();

    float GetMinWidth() const { return m_minWidth; }
    float GetMaxWidth() const { return m_maxWidth; }
    float GetMinHeight() const { return m_minHeight; }
    float GetMaxHeight() const { return m_maxHeight; }
    void SetMinWidth(float value) { m_minWidth = value; }
    void SetMaxWidth(float value) { m_maxWidth = value; }
    void SetMinHeight(float value) { m_minHeight = value; }
    void SetMaxHeight(float value) { m_maxHeight = value; }
    void SetWidth(float minWidth, float maxWidth) { m_minWidth = minWidth; m_maxWidth = maxWidth; }
    void SetHeight(float minHeight, float maxHeight) { m_minHeight = minHeight; m_maxHeight = maxHeight; }

    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

private:
    float m_minWidth;
    float m_maxWidth;
    float m_minHeight;
    float m_maxHeight;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SpawnVelocity : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnVelocity, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnVelocity);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnVelocity);

public:
    // Emitter starting velocity types
    enum SpawnVelocityType
    {
        SpawnVelocityType_Fixed,
        SpawnVelocityType_Arc,
        SpawnVelocityType_Random,
        SpawnVelocityType_RandomFixedAxis,
        SpawnVelocityType_Count
    };

public:
    ParticleSystemModule_SpawnVelocity();
    virtual ~ParticleSystemModule_SpawnVelocity();

    // Only affects creation of particles
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

    // Spawn velocity
    const SpawnVelocityType GetSpawnVelocityType() const { return (SpawnVelocityType)m_spawnVelocityType; }
    void SetSpawnVelocityFixed(const float3 &fixedVelocity);
    void SetSpawnVelocityArc(float minAngle, float maxAngle, float distance, uint32 axis);
    void SetSpawnVelocityRandom(float minSpeed, float maxSpeed);
    void SetSpawnVelocityRandomFixedAxis(float minSpeed, float maxSpeed, uint32 axis);

private:
    // Spawn velocity setup
    uint32 m_spawnVelocityType;
    struct
    {
        struct
        {
            float Velocity[3];
        } Fixed;
        struct
        {
            float MinAngle;
            float MaxAngle;
            float Distance;
            uint32 Axis;
        } Arc;
        struct
        {
            float MinSpeed;
            float MaxSpeed;
        } Random;
        struct
        {
            float MinSpeed;
            float MaxSpeed;
            uint32 Axis;
        } RandomFixedAxis;
    } m_spawnVelocityProperties;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SpawnRotation : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SpawnRotation, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SpawnRotation);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SpawnRotation);

public:
    ParticleSystemModule_SpawnRotation();
    virtual ~ParticleSystemModule_SpawnRotation();

    const float GetMinRotation() const { return m_minRotation; }
    const float GetMaxRotation() const { return m_maxRotation; }
    void SetMinRotation(float minRotation) { m_minRotation = minRotation; }
    void SetMaxRotation(float maxRotation) { m_maxRotation = maxRotation; }
    void SetRotation(float minRotation, float maxRotation) { m_minRotation = minRotation; m_maxRotation = maxRotation; }

    // Only affects creation of particles
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

private:
    float m_minRotation;
    float m_maxRotation;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_LockToEmitter : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_LockToEmitter, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_LockToEmitter);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_LockToEmitter);

public:
    ParticleSystemModule_LockToEmitter();
    virtual ~ParticleSystemModule_LockToEmitter();

    const float3 &GetOffsetPosition() const { return m_offsetPosition; }
    void SetOffsetPosition(const float3 &offsetPosition) { m_offsetPosition = offsetPosition; }

    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float3 m_offsetPosition;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_FadeOut : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_FadeOut, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_FadeOut);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_FadeOut);

public:
    ParticleSystemModule_FadeOut();
    virtual ~ParticleSystemModule_FadeOut();

    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float m_startFadeTime;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_Flipbook : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_Flipbook, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_Flipbook);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_Flipbook);

public:
    ParticleSystemModule_Flipbook();
    virtual ~ParticleSystemModule_Flipbook();

    const uint32 GetColumnCount() const { return m_columns; }
    const uint32 GetRowCount() const { return m_rows; }
    const float GetFlipInterval() const { return m_flipInterval; }
    void SetColumnCount(uint32 columns) { m_columns = columns; }
    void SetRowCount(uint32 rows) { m_rows = rows; }
    void SetFlipInterval(float flipInterval) { m_flipInterval = flipInterval; }

    // Initialize the texture coordinates of the particle to the first frame of the flipbook
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

    // Update the texture coordinates of the particle to the next frame of the flipbook if enough time has passed
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    // helper function to get the minimum/maximum texture coordinates of a frame
    float2 GetTextureCoordinateRange() const;
    float2 GetMinTextureCoordinates(uint32 frameNumber) const;
    float2 GetMaxTextureCoordinates(uint32 frameNumber) const;

    // properties
    uint32 m_columns;
    uint32 m_rows;
    float m_flipInterval;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_ColorOverTime : public ParticleSystemModule_TimeBased
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_ColorOverTime, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_ColorOverTime);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_ColorOverTime);

public:
    ParticleSystemModule_ColorOverTime();
    virtual ~ParticleSystemModule_ColorOverTime();

    const uint32 GetStartColor() const { return m_startColor; }
    const uint32 GetEndColor() const { return m_endColor; }
    void SetStartColor(uint32 startColor) { m_startColor = startColor; }
    void SetEndColor(uint32 endColor) { m_endColor = endColor; }

    // Initialize the particle to starting color
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

    // Update the particle to the correct color
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    uint32 m_startColor;
    uint32 m_endColor;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_OpacityOverTime : public ParticleSystemModule_TimeBased
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_OpacityOverTime, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_OpacityOverTime);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_OpacityOverTime);

public:
    ParticleSystemModule_OpacityOverTime();
    virtual ~ParticleSystemModule_OpacityOverTime();

    const float GetStartOpacity() const { return m_startOpacity; }
    const float GetEndOpacity() const { return m_endOpacity; }
    void SetStartOpacity(float startOpacity) { m_startOpacity = startOpacity; }
    void SetEndOpacity(float endOpacity) { m_endOpacity = endOpacity; }

    // Initialize the particle to starting color
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;

    // Update the particle to the correct color
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float m_startOpacity;
    float m_endOpacity;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_SizeOverTime : public ParticleSystemModule_TimeBased
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_SizeOverTime, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_SizeOverTime);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_SizeOverTime);

public:
    ParticleSystemModule_SizeOverTime();
    virtual ~ParticleSystemModule_SizeOverTime();

    float GetStartWidth() const { return m_startWidth; }
    float GetEndWidth() const { return m_endWidth; }
    float GetStartHeight() const { return m_startHeight; }
    float GetEndHeight() const { return m_endHeight; }
    void SetStartWidth(float value) { m_startWidth = value; }
    void SetEndWidth(float value) { m_endWidth = value; }
    void SetStartHeight(float value) { m_startHeight = value; }
    void SetEndHeight(float value) { m_endHeight = value; }
    void SetWidth(float startWidth, float endWidth) { m_startWidth = startWidth; m_endWidth = endWidth; }
    void SetHeight(float startHeight, float endHeight) { m_startHeight = startHeight; m_endHeight = endHeight; }

    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float m_startWidth;
    float m_endWidth;
    float m_startHeight;
    float m_endHeight;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_RotationSpeed : public ParticleSystemModule
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_RotationSpeed, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_RotationSpeed);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_RotationSpeed);

public:
    ParticleSystemModule_RotationSpeed();
    virtual ~ParticleSystemModule_RotationSpeed();

    float GetRotationSpeed() const { return m_rotationSpeed; }
    void SetRotationSpeed(float rotationSpeed) { m_rotationSpeed = rotationSpeed; }

    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float m_rotationSpeed;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParticleSystemModule_RotationOverTime : public ParticleSystemModule_TimeBased
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule_RotationOverTime, ParticleSystemModule);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemModule_RotationOverTime);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemModule_RotationOverTime);

public:
    ParticleSystemModule_RotationOverTime();
    virtual ~ParticleSystemModule_RotationOverTime();

    float GetStartRotation() const { return m_startRotation; }
    float GetEndRotation() const { return m_endRotation; }
    void SetStartRotation(float rotation) { m_startRotation = rotation; }
    void SetEndRotation(float rotation) { m_endRotation = rotation; }
    void SetRotation(float startRotation, float endRotation) { m_startRotation = startRotation; m_endRotation = endRotation; }

    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const override;
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const override;

private:
    float m_startRotation;
    float m_endRotation;
};

