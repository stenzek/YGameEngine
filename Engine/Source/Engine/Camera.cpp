#include "Engine/PrecompiledHeader.h"
#include "Engine/Camera.h"
#include "Renderer/RendererTypes.h"

// takes from z-up to y-up
DEFINE_CONST_MATRIX4X4F(s_ZUpToYUpMatrix,
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);


DEFINE_CONST_MATRIX4X4F(s_YUpToZUpMatrix,
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, -1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);


const float4x4 &Camera::GetWorldToCameraCoordinateSystemTransform()
{
    return s_ZUpToYUpMatrix;
}

const float4x4 &Camera::GetCameraToWorldCoordinateSystemTransform()
{
    return s_YUpToZUpMatrix;
}

float3 Camera::TransformFromWorldToCameraCoordinateSystem(const float3 &v)
{
    return float3(v.x, v.z, -v.y);
}

float3 Camera::TransformFromCameraToWorldCoordinateSystem(const float3 &v)
{
    return float3(v.x, -v.z, v.y);
}

Camera::Camera()
    : m_position(float3::Zero),
      m_rotation(Quaternion::Identity),
      m_projectionType(CAMERA_PROJECTION_TYPE_PERSPECTIVE),
      m_nearPlaneDistance(1.0f),
      m_farPlaneDistance(1000.0f),
      m_objectCullDistance(1000.0f),
      m_orthoWindowLeft(-100.0f),
      m_orthoWindowRight(100.0f),
      m_orthoWindowTop(100.0f),
      m_orthoWindowBottom(-100.0f),
      m_perspectiveFieldOfView(45.0f),
      m_perspectiveAspect(640.0f / 480.0f)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

Camera::Camera(const Camera &camera)
    : m_position(camera.m_position),
      m_rotation(camera.m_rotation),
      m_objectCullDistance(camera.m_objectCullDistance),
      m_projectionType(camera.m_projectionType),
      m_nearPlaneDistance(camera.m_nearPlaneDistance),
      m_farPlaneDistance(camera.m_farPlaneDistance),
      m_orthoWindowLeft(camera.m_orthoWindowLeft),
      m_orthoWindowRight(camera.m_orthoWindowRight),
      m_orthoWindowTop(camera.m_orthoWindowTop),
      m_orthoWindowBottom(camera.m_orthoWindowBottom),
      m_perspectiveFieldOfView(camera.m_perspectiveFieldOfView),
      m_perspectiveAspect(camera.m_perspectiveAspect),
      m_frustum(camera.m_frustum),
      m_viewMatrix(camera.m_viewMatrix),
      m_projectionMatrix(camera.m_projectionMatrix),
      m_inverseViewMatrix(camera.m_inverseViewMatrix),
      m_inverseProjectionMatrix(camera.m_inverseProjectionMatrix),
      m_viewProjectionMatrix(camera.m_viewProjectionMatrix),
      m_inverseViewProjectionMatrix(camera.m_inverseViewProjectionMatrix)
{

}

Camera::Camera(const float4x4 &viewMatrix)
    : Camera()
{
    SetViewMatrix(viewMatrix);
}

float3 Camera::CalculateViewDirection() const
{
    return (m_rotation * float3::UnitY);
}

float3 Camera::CalculateUpDirection() const
{
    return (m_rotation * float3::UnitZ);
}

float Camera::CalculateDepthToPoint(const float3 &point) const
{
    return Math::Abs(m_viewMatrix.GetRow(1).xyz().Dot(point) + m_viewMatrix(1, 3));
}

float Camera::CalculateDepthToBox(const AABox &box) const
{
//     // optimize me!
//     float minDistance = Y_FLT_INFINITE;
//     float3 boxCorners[8];
//     box.GetCornerPoints(boxCorners);
//     for (uint32 i = 0; i < 8; i++)
//         minDistance = Min(minDistance, Math::Abs(m_viewMatrix.GetRow(1).xyz().Dot(boxCorners[i]) + m_viewMatrix(1, 3)));
//     return minDistance;

    // transform box to view space
    AABox transformedBox(box.GetTransformed(m_viewMatrix));
    return Math::Abs(transformedBox.GetMinBounds().z);
}

void Camera::SetPosition(const float3 &position)
{
    m_position = position;
    UpdateViewMatrix();
    UpdateFrustum();
}

void Camera::SetRotation(const Quaternion &rotation)
{
    m_rotation = rotation;
    UpdateViewMatrix();
    UpdateFrustum();
}

void Camera::SetViewMatrix(const float4x4 &viewMatrix)
{
    m_position = -viewMatrix.GetColumn(3).xyz();
    m_rotation = Quaternion::FromFloat4x4(viewMatrix);

    // switch to z-up
    m_viewMatrix = s_ZUpToYUpMatrix * viewMatrix;
    m_inverseViewMatrix = m_viewMatrix.Inverse();
    UpdateFrustum();
}

void Camera::LookAt(const float3 &eye, const float3 &target, const float3 &upVector)
{
    // transform to camera space
    float3 cameraSpaceEye(TransformFromWorldToCameraCoordinateSystem(eye));
    float3 cameraSpaceTarget(TransformFromWorldToCameraCoordinateSystem(target));
    float3 cameraSpaceUp(TransformFromWorldToCameraCoordinateSystem(upVector));

    // add some distance to target
    if (cameraSpaceEye == cameraSpaceTarget)
        cameraSpaceTarget.z -= 1.0f;

    // get matrix
    float4x4 lookAtMatrix = float4x4::MakeLookAtViewMatrix(cameraSpaceEye, cameraSpaceTarget, cameraSpaceUp);

    // go back to world space
    //lookAtMatrix = GetCameraToWorldCoordinateSystemTransform() * lookAtMatrix;

    // update rotation
    m_position = eye;
    m_rotation = Quaternion::FromFloat4x4(lookAtMatrix).Inverse();
    UpdateViewMatrix();
    UpdateFrustum();
}

void Camera::LookDirection(const float3 &direction, const float3 &upVector)
{
    // transform to camera space
    float3 cameraSpaceDirection(TransformFromWorldToCameraCoordinateSystem(direction).Normalize());
    float3 cameraSpaceUp(TransformFromWorldToCameraCoordinateSystem(upVector));

    // get matrix
    float4x4 lookAtMatrix = float4x4::MakeLookAtViewMatrix(float3::Zero, cameraSpaceDirection, cameraSpaceUp);

    // go back to world space
    //lookAtMatrix = GetCameraToWorldCoordinateSystemTransform() * lookAtMatrix;

    // update rotation
    m_rotation = Quaternion::FromFloat4x4(lookAtMatrix).Inverse();
    UpdateViewMatrix();
    UpdateFrustum();
}

void Camera::SetProjectionType(CAMERA_PROJECTION_TYPE type)
{
    DebugAssert(type < CAMERA_PROJECTION_TYPE_COUNT);
    m_projectionType = type;
    UpdateProjectionMatrix();
    UpdateFrustum();
}

void Camera::SetNearFarPlaneDistances(float nearDistance, float farDistance)
{
    m_nearPlaneDistance = nearDistance;
    m_farPlaneDistance = farDistance;
    UpdateProjectionMatrix();
    UpdateFrustum();
}

void Camera::SetNearPlaneDistance(float distance)
{
    m_nearPlaneDistance = distance;
    UpdateProjectionMatrix();
    UpdateFrustum();
}

void Camera::SetFarPlaneDistance(float distance)
{
    m_farPlaneDistance = distance;
    UpdateProjectionMatrix();
    UpdateFrustum();
}

void Camera::SetProjectionMatrix(const float4x4 &projectionMatrix)
{
    m_projectionMatrix = projectionMatrix;
    m_inverseProjectionMatrix = projectionMatrix.Inverse();
    UpdateFrustum();
}

float4x4 Camera::GetBiasedProjectionMatrix(float amount) const
{
    if (m_projectionType == CAMERA_PROJECTION_TYPE_PERSPECTIVE)
        return float4x4::MakePerspectiveProjectionMatrix(Math::DegreesToRadians(m_perspectiveFieldOfView), m_perspectiveAspect, m_nearPlaneDistance + amount, m_farPlaneDistance + amount);
    else
        return float4x4::MakeOrthographicOffCenterProjectionMatrix(m_orthoWindowLeft, m_orthoWindowRight, m_orthoWindowBottom, m_orthoWindowTop, m_nearPlaneDistance + amount, m_farPlaneDistance + amount);
}

void Camera::SetPerspectiveFieldOfView(float fieldOfView)
{
    m_perspectiveFieldOfView = fieldOfView;

    if (m_projectionType == CAMERA_PROJECTION_TYPE_PERSPECTIVE)
    {
        UpdateProjectionMatrix();
        UpdateFrustum();
    }
}

void Camera::SetPerspectiveAspect(float aspect)
{
    m_perspectiveAspect = aspect;

    if (m_projectionType == CAMERA_PROJECTION_TYPE_PERSPECTIVE)
    {
        UpdateProjectionMatrix();
        UpdateFrustum();
    }
}

void Camera::SetPerspectiveAspect(float width, float height)
{
    DebugAssert(height != 0.0f);
    SetPerspectiveAspect(width / height);
}

void Camera::SetOrthographicWindow(float left, float right, float bottom, float top)
{
    m_orthoWindowLeft = left;
    m_orthoWindowRight = right;
    m_orthoWindowBottom = bottom;
    m_orthoWindowTop = top;

    if (m_projectionType == CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC)
    {
        UpdateProjectionMatrix();
        UpdateFrustum();
    }
}

void Camera::SetOrthographicWindow(float width, float height)
{
    // create width/height around the center
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    SetOrthographicWindow(-halfWidth, halfWidth, -halfHeight, halfHeight);
}

Ray Camera::GetPickRay(float x, float y, const RENDERER_VIEWPORT *pViewport) const
{
#if 0

    // calculate projection space coordinates
    float tx = (float)(x - (int32)pViewport->TopLeftX) / (float)pViewport->Width;
    float ty = (float)(y - (int32)pViewport->TopLeftY) / (float)pViewport->Height;
    float px = (2.0f * tx) - 1.0f;
    float py = -((2.0f * ty) - 1.0f);

    // and vectors
    float4 projSpaceOrigin(px, py, 0.0f, 1.0f);
    float4 projSpaceTarget(px, py, 1.0f, 1.0f);

    // transform to world space
    Vector4 worldSpaceOrigin(m_inverseViewProjectionMatrix * projSpaceOrigin);
    Vector4 worldSpaceTarget(m_inverseViewProjectionMatrix * projSpaceTarget);
    worldSpaceOrigin /= worldSpaceOrigin.w;
    worldSpaceTarget /= worldSpaceTarget.w;

    //Log_DevPrintf("WS Origin = %s, Target = %s", StringConverter::Float3ToString(worldSpaceOrigin.xyz()).GetCharArray(), StringConverter::Float3ToString(worldSpaceTarget.xyz()).GetCharArray());
    return Ray(worldSpaceOrigin.xyz(), worldSpaceTarget.xyz());

#else

    float3 worldSpaceOrigin(Unproject(float3(x, y, 0.0f), pViewport));
    float3 worldSpaceTarget(Unproject(float3(x, y, 1.0f), pViewport));
    
    //Log_DevPrintf("WS Origin = %s, Target = %s", StringConverter::Float3ToString(worldSpaceOrigin.xyz()).GetCharArray(), StringConverter::Float3ToString(worldSpaceTarget.xyz()).GetCharArray());
    return Ray(worldSpaceOrigin, worldSpaceTarget);

#endif    
}

Camera &Camera::operator=(const Camera &camera)
{
    m_position = camera.m_position;
    m_rotation = camera.m_rotation;
    m_objectCullDistance = camera.m_objectCullDistance;
    m_projectionType = camera.m_projectionType;
    m_nearPlaneDistance = camera.m_nearPlaneDistance;
    m_farPlaneDistance = camera.m_farPlaneDistance;
    m_orthoWindowLeft = camera.m_orthoWindowLeft;
    m_orthoWindowRight = camera.m_orthoWindowRight;
    m_orthoWindowBottom = camera.m_orthoWindowBottom;
    m_orthoWindowTop = camera.m_orthoWindowTop;
    m_perspectiveFieldOfView = camera.m_perspectiveFieldOfView;
    m_perspectiveAspect = camera.m_perspectiveAspect;
    m_frustum = camera.m_frustum;
    m_viewMatrix = camera.m_viewMatrix;
    m_projectionMatrix = camera.m_projectionMatrix;
    m_inverseViewMatrix = camera.m_inverseViewMatrix;
    m_inverseProjectionMatrix = camera.m_inverseProjectionMatrix;
    m_viewProjectionMatrix = camera.m_viewProjectionMatrix;
    m_inverseViewProjectionMatrix = camera.m_inverseViewProjectionMatrix;

    return *this;
}

void Camera::UpdateViewMatrix()
{
    float4x4 rotationMatrix(m_rotation.Inverse().GetMatrix4x4());
    float4x4 translationMatrix(float4x4::MakeTranslationMatrix(-m_position));

    // transform to y-up last
    // note: unlike object transforms, we first translate then rotate, since we are
    // moving our view around a point, rather than moving a rotated object to a point
    m_viewMatrix = s_ZUpToYUpMatrix * rotationMatrix * translationMatrix;
    m_inverseViewMatrix = m_viewMatrix.Inverse();
}

void Camera::UpdateProjectionMatrix()
{
    if (m_projectionType == CAMERA_PROJECTION_TYPE_PERSPECTIVE)
        m_projectionMatrix = float4x4::MakePerspectiveProjectionMatrix(Math::DegreesToRadians(m_perspectiveFieldOfView), m_perspectiveAspect, m_nearPlaneDistance, m_farPlaneDistance);
    else
        m_projectionMatrix = float4x4::MakeOrthographicOffCenterProjectionMatrix(m_orthoWindowLeft, m_orthoWindowRight, m_orthoWindowBottom, m_orthoWindowTop, m_nearPlaneDistance, m_farPlaneDistance);

    m_inverseProjectionMatrix = m_projectionMatrix.Inverse();
}

void Camera::UpdateFrustum()
{
    // update view+projection matrices
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    //m_inverseViewProjectionMatrix = m_inverseViewMatrix * m_inverseProjectionMatrix;
    m_inverseViewProjectionMatrix = m_viewProjectionMatrix.Inverse();

    // update frustum
    m_frustum.SetFromMatrix(m_viewProjectionMatrix);
}

float3 Camera::Project(const float3 &worldCoordindates, const RENDERER_VIEWPORT *pViewport) const
{
    // assume w=1
    float4 projectedPosition(m_viewProjectionMatrix * float4(worldCoordindates, 1.0f));

    // project back to w==1
    projectedPosition /= projectedPosition.w;

    // transform from ndc to viewport coordinates
    float x = (float)pViewport->TopLeftX + (1.0f + projectedPosition.x) * (float)pViewport->Width / 2.0f;
    float y = (float)pViewport->TopLeftY + (1.0f - projectedPosition.y) * (float)pViewport->Height / 2.0f;
    float z = pViewport->MinDepth + projectedPosition.z * (pViewport->MaxDepth - pViewport->MinDepth);

    // make vector
    return float3(x, y, z);
}

float3 Camera::Unproject(const float3 &windowCoordinates, const RENDERER_VIEWPORT *pViewport) const
{
    float x = 2.0f * (windowCoordinates.x - (float)pViewport->TopLeftX) / (float)pViewport->Width - 1.0f;
    float y = 1.0f - 2.0f * (windowCoordinates.y - (float)pViewport->TopLeftY) / (float)pViewport->Height;
    float z = (windowCoordinates.z - pViewport->MinDepth) / (pViewport->MaxDepth - pViewport->MinDepth);

    // transform back to world space
    float4 worldSpace(m_inverseViewProjectionMatrix * float4(x, y, z, 1.0f));
    return worldSpace.xyz() / worldSpace.w;
}
