#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/DemoCamera.h"
#include "Engine/World.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(DemoCamera);

DemoCamera::DemoCamera()
    : Camera(),
      m_moved(false),
      m_clippingEnabled(false),
      m_fCameraSpeed(1.0f),
      m_pWorld(nullptr),
      m_moveDirection(float3::Zero)
{

}

DemoCamera::~DemoCamera()
{

}

void DemoCamera::Move(const float3 &direction, float distance)
{
    float3 D(direction * distance);
    if (D.SquaredLength() < Y_FLT_EPSILON)
        return;

    m_position += D;
    UpdateViewMatrix();
    UpdateFrustum();
}

void DemoCamera::ModPitch(float Amount)
{
    // rotate around local x axis
    Rotate(m_rotation * float3::UnitX, Amount);
}

void DemoCamera::ModYaw(float Amount)
{
    // rotate around fixed yaw axis
    Rotate(float3::UnitZ, Amount);
}

void DemoCamera::ModRoll(float Amount)
{
    // rotate it
    Rotate(m_rotation * float3::UnitY, Amount);
}

void DemoCamera::Rotate(const float3 &axis, float angle)
{
    Rotate(Quaternion::FromAxisAngle(axis, angle));
}

void DemoCamera::Rotate(const Quaternion &rotation)
{
    m_rotation = rotation * m_rotation;
    m_rotation.NormalizeInPlace();

    UpdateViewMatrix();
    UpdateFrustum();

    //float3 eulerAngles(m_rotation.GetEulerAngles());
    //Log_DevPrintf("new rotation eular angles: %s", StringConverter::Float3ToString(float3(Math::RadiansToDegrees(eulerAngles.x), Math::RadiansToDegrees(eulerAngles.y), Math::DegreesToRadians(eulerAngles.z))).GetCharArray());
}

void DemoCamera::Update(const float &dt)
{
    float moveDistance = m_fCameraSpeed * dt * ((m_turbo) ? 5.0f : 1.0f);
    float3 moveVector((m_rotation * m_moveDirection) * moveDistance);

    if (m_clippingEnabled && m_pWorld != NULL)
    {
        static const float gravity = 9.8f;

        const float PLAYER_WIDTH = 1.0f;
        //const float PLAYER_HEIGHT = 2.0f;

        const Physics::PhysicsProxy *pContactObject;
        float3 contactNormal;
        float3 contactPoint;
        float contactHitFraction;

        // new position
        float3 newPosition(m_position);
        if (moveVector.SquaredLength() > 0.0f)
        {
            // push a sphere
            if (m_pWorld->GetPhysicsWorld()->SweepSphere(PLAYER_WIDTH * 0.5f, newPosition, newPosition + moveVector, &pContactObject, &contactNormal, &contactPoint, &contactHitFraction))
            {
                // alter position
                Log_DevPrintf("hit fraction %f", contactHitFraction);
                newPosition = newPosition - moveVector * contactHitFraction;
            }
            else
            {
                // no hit
                newPosition = newPosition + moveVector;
            }
        }

//         // apply gravity
//         moveVector = float3(0.0f, 0.0f, -9.8f) * dt;
// 
//         // push a sphere
//         if (m_pWorld->GetPhysicsWorld()->SweepSphere(PLAYER_WIDTH * 0.5f, newPosition, newPosition + moveVector, &pContactObject, &contactNormal, &contactPoint, &contactHitFraction))
//         {
//             // alter position
//             Log_DevPrintf("gravity hit fraction %f", contactHitFraction);
//             newPosition = newPosition - moveVector * contactHitFraction;
//         }
//         else
//         {
//             // no hit
//             newPosition = newPosition + moveVector;
//         }

        // update position
        SetPosition(newPosition);
    }
    else
    {
        if (moveVector.SquaredLength() > Y_FLT_EPSILON)
            SetPosition(m_position + moveVector);
    }
}

bool DemoCamera::HandleSDLEvent(const union SDL_Event *pEvent)
{
    if ((pEvent->type == SDL_KEYDOWN || pEvent->type == SDL_KEYUP) && !pEvent->key.repeat)
    {
        switch (pEvent->key.keysym.sym)
        {
        case SDLK_w:
            m_moveDirection += (pEvent->type == SDL_KEYDOWN) ? float3::UnitY : float3::NegativeUnitY;
            return true;

        case SDLK_s:
            m_moveDirection -= (pEvent->type == SDL_KEYDOWN) ? float3::UnitY : float3::NegativeUnitY;
            return true;

        case SDLK_d:
            m_moveDirection += (pEvent->type == SDL_KEYDOWN) ? float3::UnitX : float3::NegativeUnitX;
            return true;

        case SDLK_a:
            m_moveDirection -= (pEvent->type == SDL_KEYDOWN) ? float3::UnitX : float3::NegativeUnitX;
            return true;

        case SDLK_q:
            m_moveDirection += (pEvent->type == SDL_KEYDOWN) ? float3::UnitZ : float3::NegativeUnitZ;
            return true;

        case SDLK_e:
            m_moveDirection -= (pEvent->type == SDL_KEYDOWN) ? float3::UnitZ : float3::NegativeUnitZ;
            return true;

        case SDLK_LSHIFT:
            m_turbo = (pEvent->type == SDL_KEYDOWN);
            return true;
        }
    }
    else if (pEvent->type == SDL_MOUSEMOTION)
    {
        if (pEvent->motion.xrel != 0)
            ModYaw((float)-pEvent->motion.xrel);
        if (pEvent->motion.yrel != 0)
            ModPitch((float)-pEvent->motion.yrel);
        
        return true;
    }

    return false;
}
