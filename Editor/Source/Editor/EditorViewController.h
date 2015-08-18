#pragma once
#include "Editor/Common.h"
#include "Engine/Camera.h"
#include "Renderer/WorldRenderer.h"

class EditorViewController
{
public:
    EditorViewController();
    ~EditorViewController();

    // Camera mode
    const EDITOR_CAMERA_MODE GetCameraMode() const { return m_mode; }
    void SetCameraMode(EDITOR_CAMERA_MODE mode);
    void Reset();

    // actual camera
    const WorldRenderer::ViewParameters *GetViewParameters() const { return &m_viewParameters; }
    const Camera &GetCamera() const { return m_viewParameters.ViewCamera; }    

    // Common properties
    const float GetDrawDistance() const { return m_viewParameters.ViewCamera.GetObjectCullDistance(); }
    const float GetShadowDistance() const { return m_viewParameters.MaximumShadowViewDistance; }
    const float GetNearPlaneDistance() const { return m_viewParameters.ViewCamera.GetNearPlaneDistance(); }
    const float GetPerspectiveFieldOfView() const { return m_viewParameters.ViewCamera.GetPerspectiveFieldOfView(); }
    const float GetPerspectiveAcceleration() const { return m_perspectiveAcceleration; }
    const float GetPerspectiveMaxSpeed() const { return m_perspectiveMaxSpeed; }
    const float GetOrthographicScale() const { return m_orthographicScale; }
    const bool IsOrthoView() const { return (m_mode >= EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT && m_mode <= EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP); }
    const bool IsTurboEnabled() const { return m_turboEnabled; }
    const bool IsChanged() const { return m_changed; }

    // Camera properties
    const float3 &GetCameraPosition() const { return m_viewParameters.ViewCamera.GetPosition(); }
    const Quaternion &GetCameraRotation() const { return m_viewParameters.ViewCamera.GetRotation(); }
    const float3 GetCameraRightDirection() const { return (m_viewParameters.ViewCamera.GetRotation() * float3::UnitX).Normalize(); }
    const float3 GetCameraForwardDirection() const { return (m_viewParameters.ViewCamera.GetRotation() * float3::UnitY).Normalize(); }
    const float3 GetCameraUpDirection() const { return (m_viewParameters.ViewCamera.GetRotation() * float3::UnitZ).Normalize(); }

    // Common properties
    void SetDrawDistance(float v);
    void SetShadowDistance(float v);
    void SetNearPlaneDistance(float v);
    void SetPerspectiveFieldOfView(float fov);
    void SetPerspectiveAcceleration(float v);
    void SetPerspectiveMaxSpeed(float v);
    void SetOrthographicScale(float v);
    void SetTurboEnabled(bool enabled);

    // Camera properties
    void SetCameraPosition(const float3 &position);
    void SetCameraRotation(const Quaternion &rotation);

    // viewport
    const RENDERER_VIEWPORT *GetViewport() const { return &m_viewParameters.Viewport; }
    void SetViewportDimensions(uint32 width, uint32 height);

    // Move the camera in the specified direction.
    void Move(const float3 &direction, float distance);

    // Pitch/yaw/roll the camera.
    void ModPitch(float amount);
    void ModYaw(float amount);
    void ModRoll(float amount);

    // Using the current position, change the orientation of the camera to look at the specified coordinates.
    void LookAt(const float3 &lookAtPosition);

    // Rotate the camera around a specified axis and angle.
    void Rotate(const float3 &axis, float angle);

    // Rotate using the specified quaternion.
    void Rotate(const Quaternion &rotation);

    // Handle a keyboard event, returns true if the event was consumed
    bool HandleKeyboardEvent(const QKeyEvent *pKeyboardEvent);

    // Move from mouse position (mostly left-button held down in camera mode)
    void MoveFromMousePosition(const int2 &mousePositionDiff);

    // Move from mouse position (mostly right-button held down in any mode)
    void RotateFromMousePosition(const int2 &mousePositionDiff);

    // time update
    void Update(float dt);

    // get a pick ray
    Ray GetPickRay(int32 x, int32 y) const;

    // project a world coordinate to window coordinates
    float3 Project(const float3 &worldCoordindates) const;

    // unproject window coordinates to world coordinates
    float3 Unproject(const float3 &windowCoordinates) const;

protected:
    // update camera rotation
    void UpdateCameraRotation();

    // update camera settings from our settings
    void UpdateViewportDependancies();

    // current mode
    EDITOR_CAMERA_MODE m_mode;

    // mode properties
    float m_perspectiveAcceleration;
    float m_perspectiveMaxSpeed;
    float m_orthographicScale;

    // state
    float m_yaw;
    float m_pitch;
    float m_roll;
    float3 m_currentPerspectiveMoveVector;
    bool m_leftKeyState;
    bool m_rightKeyState;
    bool m_forwardKeyState;
    bool m_backKeyState;
    bool m_upKeyState;
    bool m_downKeyState;
    bool m_turboEnabled;
    bool m_internalChanged;
    bool m_changed;

    // renderer stuff
    WorldRenderer::ViewParameters m_viewParameters;
};

