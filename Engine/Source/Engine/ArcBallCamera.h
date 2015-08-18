#pragma once
#include "Engine/Common.h"
#include "Engine/Camera.h"

class ArcBallCamera : public Camera
{
public:
    ArcBallCamera();
    ~ArcBallCamera();

    // Common properties
    const float3 &GetTarget() const { return m_target; }
    const float GetEyeDistance() const { return m_eyeDistance; }

    // Common properties
    void Reset();
    void SetTarget(const float3 &t) { m_target = t; UpdateArcBallViewMatrix(); }
    void SetEyeDistance(float v) { m_eyeDistance = v; UpdateArcBallViewMatrix(); }

    // handle mouse/keyboard input events
    void RotateFromMouseMovement(int32 mouseDiffX, int32 mouseDiffY);

    // time update
    void Update(const float &dt);

protected:
    void UpdateArcBallViewMatrix();

    // common properties
    float3 m_target;
    float m_eyeDistance;
};
