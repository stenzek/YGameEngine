#pragma once
#include "Engine/Common.h"

struct RENDERER_VIEWPORT;

enum CAMERA_PROJECTION_TYPE
{
    CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC,
    CAMERA_PROJECTION_TYPE_PERSPECTIVE,
    CAMERA_PROJECTION_TYPE_COUNT,
};

class Camera
{
public:
    Camera();
    Camera(const Camera &camera);
    Camera(const float4x4 &viewMatrix);

    // all types
    const float3 &GetPosition() const { return m_position; }
    const Quaternion &GetRotation() const { return m_rotation; }
    const CAMERA_PROJECTION_TYPE GetProjectionType() const { return m_projectionType; }
    const float GetNearPlaneDistance() const { return m_nearPlaneDistance; }
    const float GetFarPlaneDistance() const { return m_farPlaneDistance; }

    // object culling distance
    const float GetObjectCullDistance() const { return m_objectCullDistance; }
    void SetObjectCullDistance(float distance) { m_objectCullDistance = distance; }
    void SetObjectCullDistanceFromNearFar() { m_objectCullDistance = m_farPlaneDistance + m_nearPlaneDistance; }

    // ortho-specific
    const float GetOrthoWindowLeft() const { return m_orthoWindowLeft; }
    const float GetOrthoWindowRight() const { return m_orthoWindowRight; }
    const float GetOrthoWindowBottom() const { return m_orthoWindowBottom; }
    const float GetOrthoWindowTop() const { return m_orthoWindowTop; }

    // perspective-specific
    const float GetPerspectiveFieldOfView() const { return m_perspectiveFieldOfView; }
    const float GetPerspectiveAspect() const { return m_perspectiveAspect; }
    
    // cache -- replace with dirty flags at some point?
    const Frustum &GetFrustum() const { return m_frustum; }
    const float4x4 &GetViewMatrix() const { return m_viewMatrix; }
    const float4x4 &GetProjectionMatrix() const { return m_projectionMatrix; }
    const float4x4 &GetInverseViewMatrix() const { return m_inverseViewMatrix; }
    const float4x4 &GetInverseProjectionMatrix() const { return m_inverseProjectionMatrix; }
    const float4x4 &GetViewProjectionMatrix() const { return m_viewProjectionMatrix; }
    const float4x4 &GetInverseViewProjectionMatrix() const { return m_inverseViewProjectionMatrix; }

    // determined on-the-fly
    float3 CalculateViewDirection() const;
    float3 CalculateUpDirection() const;

    // calculate the depth or distance to a point in world space
    float CalculateDepthToPoint(const float3 &point) const;

    // calculate the depth to any of the planes in a box in world space
    float CalculateDepthToBox(const AABox &box) const;

    // setters
    void SetPosition(const float3 &position);
    void SetRotation(const Quaternion &rotation);

    // setting a view matrix assumes that the matrix coming in is still using a z-up convention
    void SetViewMatrix(const float4x4 &viewMatrix);

    // look at a specific point
    void LookAt(const float3 &eye, const float3 &target, const float3 &upVector);

    // look in a direction, keeping position intact
    void LookDirection(const float3 &direction, const float3 &upVector);

    // projection setup
    void SetProjectionType(CAMERA_PROJECTION_TYPE type);
    void SetNearFarPlaneDistances(float nearDistance, float farDistance);
    void SetNearPlaneDistance(float distance);
    void SetFarPlaneDistance(float distance);

    // set an override projection matrix
    void SetProjectionMatrix(const float4x4 &projectionMatrix);

    // get a biased projection matrix, *will* ignore any custom projection matrix set
    float4x4 GetBiasedProjectionMatrix(float amount) const;

    // perspective -- field of view is in degrees
    void SetPerspectiveFieldOfView(float fieldOfView);
    void SetPerspectiveAspect(float aspect);
    void SetPerspectiveAspect(float width, float height);

    // orthographic
    void SetOrthographicWindow(float left, float right, float bottom, float top);
    void SetOrthographicWindow(float width, float height);

    // get a picking ray
    Ray GetPickRay(float x, float y, const RENDERER_VIEWPORT *pViewport) const;

    // project a world coordinate to window coordinates
    float3 Project(const float3 &worldCoordindates, const RENDERER_VIEWPORT *pViewport) const;

    // unproject window coordinates to world coordinates
    float3 Unproject(const float3 &windowCoordinates, const RENDERER_VIEWPORT *pViewport) const;

    // operators
    Camera &operator=(const Camera &camera);
    
protected:
    // update view matrix and associated variables
    void UpdateViewMatrix();

    // update projection matrix and associated variables
    void UpdateProjectionMatrix();

    // update anything affected by view or projection changes
    void UpdateFrustum();

    // common to all
    float3 m_position;
    Quaternion m_rotation;

    // object maximum render distance
    float m_objectCullDistance;

    // projection
    CAMERA_PROJECTION_TYPE m_projectionType;
    float m_nearPlaneDistance;
    float m_farPlaneDistance;

    // orthographic
    float m_orthoWindowLeft;
    float m_orthoWindowRight;
    float m_orthoWindowTop;
    float m_orthoWindowBottom;

    // perspective
    float m_perspectiveFieldOfView;
    float m_perspectiveAspect;

    // cache
    Frustum m_frustum;
    float4x4 m_viewMatrix;
    float4x4 m_projectionMatrix;
    float4x4 m_inverseViewMatrix;
    float4x4 m_inverseProjectionMatrix;
    float4x4 m_viewProjectionMatrix;
    float4x4 m_inverseViewProjectionMatrix;

public:
    // common stuff
    static const float4x4 &GetWorldToCameraCoordinateSystemTransform();
    static const float4x4 &GetCameraToWorldCoordinateSystemTransform();
    static float3 TransformFromWorldToCameraCoordinateSystem(const float3 &v);
    static float3 TransformFromCameraToWorldCoordinateSystem(const float3 &v);
};

