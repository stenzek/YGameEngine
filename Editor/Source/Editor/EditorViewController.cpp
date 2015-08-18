#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/EditorViewController.h"
#include "Editor/EditorCVars.h"
Log_SetChannel(EditorViewController);

EditorViewController::EditorViewController()
    : m_mode(EDITOR_CAMERA_MODE_PERSPECTIVE),
      m_perspectiveAcceleration(60.0f),
      m_perspectiveMaxSpeed(5.0f),
      m_orthographicScale(1.0f),
      m_yaw(0.0f),
      m_pitch(0.0f),
      m_roll(0.0f),
      m_currentPerspectiveMoveVector(float3::Zero),
      m_leftKeyState(false),
      m_rightKeyState(false),
      m_forwardKeyState(false),
      m_backKeyState(false),
      m_upKeyState(false),
      m_downKeyState(false),
      m_turboEnabled(false),
      m_internalChanged(false),
      m_changed(false)
{
    // initialize view
    m_viewParameters.ViewCamera.SetPerspectiveFieldOfView(65.0f);
    m_viewParameters.ViewCamera.SetNearFarPlaneDistances(0.1f, 1000.0f);
    m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_PERSPECTIVE);
    m_viewParameters.ViewCamera.SetObjectCullDistance((m_viewParameters.ViewCamera.GetFarPlaneDistance() - m_viewParameters.ViewCamera.GetNearPlaneDistance()) * 1.2f);
    m_viewParameters.MaximumShadowViewDistance = m_viewParameters.ViewCamera.GetObjectCullDistance();
    m_viewParameters.Viewport.Set(0, 0, 1, 1, 0.0f, 1.0f);
}

EditorViewController::~EditorViewController()
{

}

void EditorViewController::SetCameraMode(EDITOR_CAMERA_MODE mode)
{
    DebugAssert(mode < EDITOR_CAMERA_MODE_COUNT);
    if (m_mode == mode)
        return;

    m_mode = mode;
    Reset();
}

void EditorViewController::Reset()
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT:
        {
            m_viewParameters.ViewCamera.SetPosition(float3::Zero);
            m_viewParameters.ViewCamera.SetRotation(Quaternion::Identity);
            m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
        }
        break;

    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_SIDE:
        {
            m_viewParameters.ViewCamera.SetPosition(float3::Zero);
            m_viewParameters.ViewCamera.SetRotation(Quaternion::FromNormalizedAxisAngle(float3::UnitY, 90.0f));
            m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
        }
        break;

    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP:
        {
            m_viewParameters.ViewCamera.SetPosition(float3::Zero);
            m_viewParameters.ViewCamera.SetRotation(Quaternion::FromNormalizedAxisAngle(float3::UnitX, 90.0f));
            m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
        }
        break;

    case EDITOR_CAMERA_MODE_ISOMETRIC:
        {
            m_viewParameters.ViewCamera.SetPosition(float3::Zero);
            m_viewParameters.ViewCamera.SetRotation(Quaternion::Identity);
            m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
        }
        break;

    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            m_viewParameters.ViewCamera.SetPosition(float3::Zero);
            m_viewParameters.ViewCamera.SetRotation(Quaternion::Identity);
            m_viewParameters.ViewCamera.SetProjectionType(CAMERA_PROJECTION_TYPE_PERSPECTIVE);
        }
        break;
    }

    m_internalChanged = true;
}

void EditorViewController::LookAt(const float3 &lookAtPosition)
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            m_viewParameters.ViewCamera.LookAt(m_viewParameters.ViewCamera.GetPosition(), lookAtPosition, float3::UnitY);
            m_internalChanged = true;
        }
        break;
    }
}

void EditorViewController::SetCameraPosition(const float3 &position)
{
    m_viewParameters.ViewCamera.SetPosition(position);
    m_internalChanged = true;
}

void EditorViewController::SetCameraRotation(const Quaternion &rotation)
{
    m_viewParameters.ViewCamera.SetRotation(rotation);
    m_internalChanged = true;
}

void EditorViewController::Move(const float3 &direction, float distance)
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_ISOMETRIC:
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            float3 D(direction * distance);
            if (D.SquaredLength() < Y_FLT_EPSILON)
                return;

            m_viewParameters.ViewCamera.SetPosition(m_viewParameters.ViewCamera.GetPosition() + D);
            m_internalChanged = true;
        }
        break;
    }
}

void EditorViewController::MoveFromMousePosition(const int2 &mousePositionDiff)
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT:
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_SIDE:
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP:
        {
            const float SCALE = 1.0f;
            float3 moveVector(float3::Zero);

            if (mousePositionDiff.x != 0)
                moveVector += float3::UnitX * ((float)-mousePositionDiff.x * SCALE);

            if (mousePositionDiff.y != 0)
                moveVector += float3::UnitY * ((float)mousePositionDiff.y * SCALE);

            m_viewParameters.ViewCamera.SetPosition(m_viewParameters.ViewCamera.GetPosition() + moveVector);
            m_internalChanged = true;
        }
        break;

    case EDITOR_CAMERA_MODE_ISOMETRIC:
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            const float SCALE = 0.5f;

            float3 moveVector(float3::Zero);
            if (mousePositionDiff.x != 0)
                moveVector.x += (float)mousePositionDiff.x * SCALE;
            if (mousePositionDiff.y != 0)
                moveVector.y += (float)-mousePositionDiff.y * SCALE;

            m_viewParameters.ViewCamera.SetPosition(m_viewParameters.ViewCamera.GetPosition() + (m_viewParameters.ViewCamera.GetRotation() * moveVector));
            m_internalChanged = true;
        }
        break;
    }
}

void EditorViewController::ModPitch(float amount)
{
    // rotate around local x axis
    //Rotate(m_viewParameters.ViewCamera.GetRotation() * Vector3::UnitX, amount);
    m_pitch = Math::NormalizeAngleDegrees(m_pitch + amount);
    //m_pitch += amount;
    UpdateCameraRotation();
}

void EditorViewController::ModYaw(float amount)
{
    // rotate around fixed yaw axis
    //Rotate(m_viewParameters.ViewCamera.GetRotation() * Vector3::UnitZ, amount);
    m_yaw = Math::NormalizeAngleDegrees(m_yaw + amount);
    //m_yaw += amount;
    UpdateCameraRotation();
}

void EditorViewController::ModRoll(float amount)
{
    // rotate it
    //Rotate(m_viewParameters.ViewCamera.GetRotation() * Vector3::UnitY, amount);
    m_roll = Math::NormalizeAngleDegrees(m_roll + amount);
    //m_roll += amount;
    UpdateCameraRotation();
}

void EditorViewController::Rotate(const float3 &axis, float angle)
{
    Rotate(Quaternion::FromAxisAngle(axis, angle));
}

void EditorViewController::Rotate(const Quaternion &rotation)
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_ISOMETRIC:
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            Quaternion newRotation(m_viewParameters.ViewCamera.GetRotation() * rotation);
            newRotation.NormalizeInPlace();

            m_viewParameters.ViewCamera.SetRotation(newRotation);
            m_internalChanged = true;
        }
        break;
    }
}

void EditorViewController::RotateFromMousePosition(const int2 &mousePositionDiff)
{
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT:
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_SIDE:
    case EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP:
        {
            // ortho moves redirect to movement.
            MoveFromMousePosition(mousePositionDiff);
        }
        break;

    case EDITOR_CAMERA_MODE_ISOMETRIC:
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            if (mousePositionDiff.x != 0)
                ModYaw((float)-mousePositionDiff.x);
            if (mousePositionDiff.y != 0)
                ModPitch((float)-mousePositionDiff.y);
        }
        break;
    }
}

void EditorViewController::Update(float dt)
{
    uint32 i;
    float turboFactor = (m_turboEnabled) ? 10.0f : 1.0f;

    // no changes
    m_changed = m_internalChanged;
    m_internalChanged = false;

    // depending on mode
    switch (m_mode)
    {
    case EDITOR_CAMERA_MODE_PERSPECTIVE:
        {
            // read cvars
            const float speedGain = dt * turboFactor * m_perspectiveAcceleration;

            // find direction
            float3 moveDirection(float3::Zero);
            if (m_leftKeyState != m_rightKeyState)
                moveDirection.x = (m_leftKeyState) ? -1.0f : 1.0f;
            if (m_forwardKeyState != m_backKeyState)
                moveDirection.y = (m_backKeyState) ? -1.0f : 1.0f;
            if (m_upKeyState != m_downKeyState)
                moveDirection.z = (m_downKeyState) ? -1.0f : 1.0f;

            // update camera speed and direction
            float3 currentMoveVector(m_currentPerspectiveMoveVector);
            float3 targetMoveVector(moveDirection * m_perspectiveMaxSpeed);

            // steer to the vector
            for (i = 0; i < 3; i++)
            {
                if (currentMoveVector[i] > targetMoveVector[i])
                    currentMoveVector[i] = Max(targetMoveVector[i], currentMoveVector[i] - speedGain);
                else if (currentMoveVector[i] < targetMoveVector[i])
                    currentMoveVector[i] = Min(targetMoveVector[i], currentMoveVector[i] + speedGain);
            }

            // move camera
            if (currentMoveVector.SquaredLength() > Y_FLT_EPSILON)
            {
                m_viewParameters.ViewCamera.SetPosition(m_viewParameters.ViewCamera.GetPosition() + ((m_viewParameters.ViewCamera.GetRotation() * currentMoveVector) * turboFactor * dt));
                m_changed = true;
            }

            // store move vector
            m_currentPerspectiveMoveVector = currentMoveVector;
        }
        break;
    }
}

Ray EditorViewController::GetPickRay(int32 x, int32 y) const
{
    return m_viewParameters.ViewCamera.GetPickRay(x, y, &m_viewParameters.Viewport);
}


float3 EditorViewController::Project(const float3 &worldCoordindates) const
{
    return m_viewParameters.ViewCamera.Project(worldCoordindates, &m_viewParameters.Viewport);
}

float3 EditorViewController::Unproject(const float3 &windowCoordinates) const
{
    return m_viewParameters.ViewCamera.Unproject(windowCoordinates, &m_viewParameters.Viewport);
}

void EditorViewController::SetDrawDistance(float v)
{
    m_viewParameters.ViewCamera.SetObjectCullDistance(v);
    m_viewParameters.ViewCamera.SetFarPlaneDistance(v * 1.2f);
    m_internalChanged = true;
}

void EditorViewController::SetShadowDistance(float v)
{
    m_viewParameters.MaximumShadowViewDistance = v;
    m_internalChanged = true;
}

void EditorViewController::SetNearPlaneDistance(float v)
{
    m_viewParameters.ViewCamera.SetNearPlaneDistance(v);
    m_internalChanged = true;
}

void EditorViewController::SetPerspectiveFieldOfView(float fov)
{
    m_viewParameters.ViewCamera.SetPerspectiveFieldOfView(fov);
    m_internalChanged = true;
}

void EditorViewController::SetPerspectiveAcceleration(float v)
{
    m_perspectiveAcceleration = v;
}

void EditorViewController::SetPerspectiveMaxSpeed(float v)
{
    m_perspectiveMaxSpeed = v;
}

void EditorViewController::SetOrthographicScale(float v)
{
    m_orthographicScale = v;
    UpdateViewportDependancies();
}

void EditorViewController::SetViewportDimensions(uint32 width, uint32 height)
{
    m_viewParameters.Viewport.Width = width;
    m_viewParameters.Viewport.Height = height;
    UpdateViewportDependancies();
}

void EditorViewController::UpdateCameraRotation()
{
    Quaternion newRotation(Quaternion::FromEulerAngles(m_pitch, m_roll, m_yaw));

//     Quaternion newRotation;
//     newRotation = Quaternion::FromAxisAngle(float3::UnitZ, Math::DegreesToRadians(m_yaw));
//     newRotation *= Quaternion::FromAxisAngle(float3::UnitX, Math::DegreesToRadians(m_pitch));
//     newRotation *= Quaternion::FromAxisAngle(float3::UnitY, Math::DegreesToRadians(m_roll));
//     Log_DevPrintf("pitch %f yaw %f roll %f", m_pitch, m_yaw, m_roll);

    m_viewParameters.ViewCamera.SetRotation(newRotation);
    m_internalChanged = true;
}

void EditorViewController::UpdateViewportDependancies()
{
    // update perspective aspect
    float perspectiveAspect = (float)m_viewParameters.Viewport.Width / (float)m_viewParameters.Viewport.Height;
    m_viewParameters.ViewCamera.SetPerspectiveAspect(perspectiveAspect);

    // update ortho window
    float orthoWindowWidth = (float)m_viewParameters.Viewport.Width * m_orthographicScale;
    float orthoWindowHeight = (float)m_viewParameters.Viewport.Height * m_orthographicScale;
    m_viewParameters.ViewCamera.SetOrthographicWindow(orthoWindowWidth, orthoWindowHeight);

    // internal state has changed
    m_internalChanged = true;
}

void EditorViewController::SetTurboEnabled(bool enabled)
{
    m_turboEnabled = enabled;
}

bool EditorViewController::HandleKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{
    // we handle camera movement via the keyboard in the viewport, not the edit mode.
    bool isKeyDownEvent = (pKeyboardEvent->type() == QEvent::KeyPress);
    bool isKeyUpEvent = (pKeyboardEvent->type() == QEvent::KeyRelease);
    if ((isKeyDownEvent | isKeyUpEvent))
    {
        // handle perspective camera movement
        if (!IsOrthoView())
        {
            switch (pKeyboardEvent->key())
            {
            case Qt::Key_W:
            case Qt::Key_Up:
                m_forwardKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_S:
            case Qt::Key_Down:
                m_backKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_A:
            case Qt::Key_Left:
                m_leftKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_D:
            case Qt::Key_Right:
                m_rightKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_Q:
                m_upKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_E:
                m_downKeyState = isKeyDownEvent;
                return true;

            case Qt::Key_Shift:
                m_turboEnabled = isKeyDownEvent;
                return true;
            }
        }
    }

    return false;
}
