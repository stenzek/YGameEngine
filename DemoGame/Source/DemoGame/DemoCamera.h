#pragma once
#include "Engine/Camera.h"

class World;

class DemoCamera : public Camera
{
public:
    DemoCamera();
    ~DemoCamera();

    // Moved flag
    const bool IsMoved() const { return m_moved; }

    // Move the camera in the specified direction.
    void Move(const float3 &direction, float distance);

    // Pitch/yaw/roll the camera.
    void ModPitch(float Amount);
    void ModYaw(float Amount);
    void ModRoll(float Amount);

    // Rotate the camera around a specified axis and angle.
    void Rotate(const float3 &axis, float angle);

    // Rotate using the specified quaternion.
    void Rotate(const Quaternion &rotation);

    // time update
    void Update(const float &dt);

    // clip mode
    const World *GetWorld() const { return m_pWorld; }
    void SetWorld(const World *pWorld) { m_pWorld = pWorld; }
    float GetCameraSpeed() const { return m_fCameraSpeed; }
    void SetCameraSpeed(float speed) { m_fCameraSpeed = speed; }
    bool GetClippingEnabled() const { return m_clippingEnabled; }
    void SetClippingEnabled(bool enabled) { m_clippingEnabled = enabled; }

    // event handler
    bool HandleSDLEvent(const union SDL_Event *pEvent);

protected:
    bool m_moved;
    bool m_clippingEnabled;
    float m_fCameraSpeed;
    const World *m_pWorld;

    // key states
    float3 m_moveDirection;
    bool m_turbo;
};
