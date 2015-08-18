#include "Engine/PrecompiledHeader.h"
#include "Engine/ArcBallCamera.h"
//Log_SetChannel(ArcBallCamera);

ArcBallCamera::ArcBallCamera()
    : Camera()
{
    m_target.SetZero();
    m_eyeDistance = 1.0f;
}

ArcBallCamera::~ArcBallCamera()
{

}

void ArcBallCamera::Reset()
{
    m_position.SetZero();
    m_rotation.SetIdentity();
    m_target.SetZero();
    m_eyeDistance = 1.0f;
    UpdateViewMatrix();
    UpdateFrustum();
}

void ArcBallCamera::RotateFromMouseMovement(int32 mouseDiffX, int32 mouseDiffY)
{
    m_rotation = (m_rotation * Quaternion::FromEulerAngles(float(-mouseDiffY), 0.0f, float(-mouseDiffX))).Normalize();
    UpdateArcBallViewMatrix();
}

void ArcBallCamera::Update(const float &dt)
{

}

void ArcBallCamera::UpdateArcBallViewMatrix()
{
    m_position = m_target + m_rotation * float3(0.0f, m_eyeDistance, 0.0f);
    UpdateViewMatrix();
    UpdateFrustum();
}

// Vector3 ArcBallCamera::MapToSphere(const Vector2i &p) const
// {
//     DebugAssert(m_iViewportWidth > 0 && m_iViewportHeight > 0);
//     Vector2 windowSize = Vector2(float(m_iViewportWidth - 1), float(m_iViewportHeight - 1));
//     Vector2 scaler = Vector2::One / (windowSize * 0.5f);
// 
//     Vector2 temp = Vector2((float)p.x, (float)p.y) * scaler;
//     temp.x = temp.x - 1.0f;
//     temp.y = 1.0f - temp.y;
// 
//     float length = temp.SquaredLength();
//     if (length > 1.0f)
//     {
//         float norm = 1.0f / Math::Sqrt(length);
//         return Vector3(temp, norm);
//     }
//     else
//     {
//         return Vector3(temp, Math::Sqrt(1.0f - length));
//     }
// }
// 
