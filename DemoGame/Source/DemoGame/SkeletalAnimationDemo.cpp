#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/SkeletalAnimationDemo.h"
#include "DemoGame/DemoUtilities.h"
#include "Engine/World.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/ImGuiBridge.h"
Log_SetChannel(SkeletalAnimationDemo);

static const char *MODEL_LIST = "Ninja\0\0";

SkeletalAnimationDemo::SkeletalAnimationDemo(DemoGame *pDemoGame)
    : BaseDemoWorldGameState(pDemoGame)
    , m_pSunLightEntity(nullptr)
    , m_sunLightRotation(float3::Zero)
    , m_pSkeletalMeshRenderProxy(nullptr)
    , m_skeletalMeshIndex(0)
    , m_skeletalAnimationIndex(0)
{
    
}

SkeletalAnimationDemo::~SkeletalAnimationDemo()
{
    SAFE_RELEASE(m_pSkeletalMeshRenderProxy);
    SAFE_RELEASE(m_pSunLightEntity);
}

bool SkeletalAnimationDemo::Initialize()
{
    if (!BaseDemoWorldGameState::Initialize())
        return false;

    if (!CreateDynamicWorld())
        return false;

    // init view
    m_viewParameters.MaximumShadowViewDistance = 500.0f;
    return true;
}

bool SkeletalAnimationDemo::OnWorldCreated(World *pWorld)
{
    if (!BaseDemoWorldGameState::OnWorldCreated(pWorld))
        return false;

    // sun entity
    //m_pSunLightEntity = DemoUtilities::CreateSunLight(m_pWorld, float3(1.0f, 1.0f, 1.0f), 1.2f, 0.4f);
    m_pSunLightEntity = DemoUtilities::CreateSunLight(m_pWorld, float3(1.0f, 0.992f, 0.8f), 1.2f, 0.4f);

    // ground plane
    AutoReleasePtr<const Material> pGroundPlaneMaterial = g_pResourceManager->GetDefaultMaterial();
    AutoReleasePtr<Entity> pGroundPlaneEntity = DemoUtilities::CreatePlaneShape(m_pWorld, float3::UnitZ, float3(0.0f, 0.0f, 0.0f), 100.0f, pGroundPlaneMaterial, 10.0f);

    // set model, this creates the renderable too
    if (!SetModelIndex(0))
        return false;

    // done
    return true;
}

void SkeletalAnimationDemo::Shutdown()
{
    BaseDemoWorldGameState::Shutdown();
}

void SkeletalAnimationDemo::OnWorldDeleted(World *pWorld)
{
    BaseDemoWorldGameState::OnWorldDeleted(pWorld);
}

void SkeletalAnimationDemo::OnMainThreadTick(float deltaTime)
{
    BaseDemoWorldGameState::OnMainThreadTick(deltaTime);

    if (m_sunLightRotation.SquaredLength() > 0.0f)
        m_pSunLightEntity->SetRotation((m_pSunLightEntity->GetRotation() * Quaternion::FromEulerAngles(m_sunLightRotation.x * deltaTime, m_sunLightRotation.y * deltaTime, 0.0f)).Normalize());

    // no thread safety here captain :(
    m_skeletalAnimationPlayer.Update(deltaTime);
    m_skeletalAnimationPlayer.UpdateMeshState();
    m_skeletalAnimationPlayer.UpdateSkeletalMeshRenderProxy(m_pSkeletalMeshRenderProxy);
    //for (uint32 i = 0; i < m_pSkeletalMeshRenderProxy->GetSkeletalMesh()->GetSkeleton()->GetBoneCount(); i++)
        //m_pSkeletalMeshRenderProxy->SetBoneTransforms(i, 1, &float3x4::Identity);
}

void SkeletalAnimationDemo::DrawOverlays(float deltaTime)
{
    BaseDemoWorldGameState::DrawOverlays(deltaTime);
}

void SkeletalAnimationDemo::DrawUI(float deltaTime)
{
    BaseDemoWorldGameState::DrawUI(deltaTime);

    ImGui::SetNextWindowPos(ImVec2(10, float(m_viewParameters.Viewport.Height - 250)), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiSetCond_FirstUseEver);

    // annoyingly, everything is drawn on the UI thread yet commands have to be invoked on the main thread..
    if (ImGui::Begin("Skeletal Animation Demo"))
    {
        int intValue;
        float floatValue;
        bool boolValue;

        ImGui::PushItemWidth(-140.0f);

        intValue = m_skeletalMeshIndex;
        if (ImGui::Combo("Mesh", &intValue, MODEL_LIST))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, intValue]() {
                SetModelIndex(intValue);
            });
        }

        // eww ugly, @TODO cache this?
        {
            char animationNamesList[512];
            uint32 animationNamesListPos = 0;
            for (const char *animationName : m_animationNames)
            {
                const char *partAnimationName = Y_strrchr(animationName, '/');
                if (partAnimationName != nullptr)
                    partAnimationName++;
                else
                    partAnimationName = animationName;

                uint32 len = Y_strlen(partAnimationName);
                if ((animationNamesListPos + len + 1) < countof(animationNamesList))
                {
                    Y_memcpy(animationNamesList + animationNamesListPos, partAnimationName, len + 1);
                    animationNamesListPos += len + 1;
                }
                else
                {
                    break;
                }
            }
            animationNamesListPos = Min(animationNamesListPos, uint32(countof(animationNamesList) - 7));
            Y_strncpy(animationNamesList + animationNamesListPos, 5, "None"); animationNamesListPos += 5;
            animationNamesList[animationNamesListPos++] = '\0';
            intValue = m_skeletalAnimationIndex;
            if (ImGui::Combo("Animation", &intValue, animationNamesList))
            {
                QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, intValue]() {
                    SetAnimationIndex(intValue);
                });
            }
        }

        floatValue = m_skeletalAnimationPlayer.GetChannelPlaybackSpeed(0);
        if (ImGui::InputFloat("Playback Speed", &floatValue, 0.1f, 0.5f, 2))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, floatValue]() {
                m_skeletalAnimationPlayer.SetPlaybackSpeed(floatValue, 0);
            });
        }

        boolValue = m_skeletalAnimationPlayer.GetChannelPlaying(0);
        if (ImGui::Button(boolValue ? "Pause" : "Resume"))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, boolValue]() {
                (m_skeletalAnimationPlayer.GetChannelPlaying(0)) ? m_skeletalAnimationPlayer.PauseAnimation(0) : m_skeletalAnimationPlayer.ResumeAnimation(0);
            });
        }

        // @TODO model rotation/scale/etc

        ImGui::PopItemWidth();
    }
    ImGui::End();
}

bool SkeletalAnimationDemo::OnWindowEvent(const union SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN)
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_PAGEUP:
            {
                if (m_skeletalAnimationIndex == 0)
                    SetAnimationIndex(m_animationNames.GetSize());
                else
                    SetAnimationIndex(m_skeletalAnimationIndex - 1);

                return true;
            }
            break;

        case SDLK_PAGEDOWN:
            {
                if (m_skeletalAnimationIndex == m_animationNames.GetSize())
                    SetAnimationIndex(0);
                else
                    SetAnimationIndex(m_skeletalAnimationIndex + 1);

                return true;
            }
            break;

        case SDLK_1:
            SetModelIndex(0);
            break;

        case SDLK_2:
            SetModelIndex(1);
            break;

        case SDLK_KP_PLUS:
            {
                m_skeletalAnimationPlayer.SetPlaybackSpeed(m_skeletalAnimationPlayer.GetChannelPlaybackSpeed(0) + 0.25f, 0);
                return true;
            }
            break;

        case SDLK_KP_MINUS:
            {
                m_skeletalAnimationPlayer.SetPlaybackSpeed(m_skeletalAnimationPlayer.GetChannelPlaybackSpeed(0) - 0.25f, 0);
                return true;
            }
            break;

        case SDLK_SPACE:
            {
                if (m_skeletalAnimationPlayer.GetChannelPlaying(0))
                    m_skeletalAnimationPlayer.PauseAnimation();
                else
                    m_skeletalAnimationPlayer.ResumeAnimation();

                return true;
            }
            break;
        }
    }

    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        bool isKeyDownEvent = (event->type == SDL_KEYDOWN);
        switch (event->key.keysym.sym)
        {
        case SDLK_LEFT:
            m_sunLightRotation.x += (isKeyDownEvent) ? -1.0f : 1.0f;
            return true;

        case SDLK_RIGHT:
            m_sunLightRotation.x += (isKeyDownEvent) ? 1.0f : -1.0f;
            return true;

        case SDLK_UP:
            m_sunLightRotation.y += (isKeyDownEvent) ? 1.0f : -1.0f;
            return true;

        case SDLK_DOWN:
            m_sunLightRotation.y += (isKeyDownEvent) ? -1.0f : 1.0f;
            return true;
        }
    }

    return BaseDemoWorldGameState::OnWindowEvent(event);
}

bool SkeletalAnimationDemo::SetModelIndex(uint32 index)
{
    AutoReleasePtr<const SkeletalMesh> pSkeletalMesh;
    Transform transform(Transform::Identity);

    if (index == 0)
    {
        // ninja
        if ((pSkeletalMesh = g_pResourceManager->GetSkeletalMesh("models/ninja/ninja")) == nullptr)
            return false;

        m_animationNames.Clear();
        m_animationNames.Add("models/ninja/ninja_walk");
        m_animationNames.Add("models/ninja/ninja_stealth_walk");
        m_animationNames.Add("models/ninja/ninja_punch_swipe");
        m_animationNames.Add("models/ninja/ninja_swipe_spin");
        m_animationNames.Add("models/ninja/ninja_twohand_swipe");
        m_animationNames.Add("models/ninja/ninja_block");
        m_animationNames.Add("models/ninja/ninja_kick");
        m_animationNames.Add("models/ninja/ninja_crouch_start");
        m_animationNames.Add("models/ninja/ninja_crouch_end");
        m_animationNames.Add("models/ninja/ninja_jump");
        m_animationNames.Add("models/ninja/ninja_jump_noheight");
        m_animationNames.Add("models/ninja/ninja_jump_attack");
        m_animationNames.Add("models/ninja/ninja_sidekick");
        m_animationNames.Add("models/ninja/ninja_spin_attack");
        m_animationNames.Add("models/ninja/ninja_backflip");
        m_animationNames.Add("models/ninja/ninja_climbwall");
        m_animationNames.Add("models/ninja/ninja_death_forwards");
        m_animationNames.Add("models/ninja/ninja_death_backwards");
        m_animationNames.Add("models/ninja/ninja_idle_1");
        m_animationNames.Add("models/ninja/ninja_idle_2");
        m_animationNames.Add("models/ninja/ninja_idle_3");
        transform.Set(float3(0.0f, 0.0f, 1.0f), Quaternion::Identity, float3::One);
    }
    else
    {
        return false;
    }

    // not initializing
    if (m_pSkeletalMeshRenderProxy == nullptr)
    {
        m_pSkeletalMeshRenderProxy = new SkeletalMeshRenderProxy(0, pSkeletalMesh, Transform::Identity, ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);
        m_pWorld->GetRenderWorld()->AddRenderable(m_pSkeletalMeshRenderProxy);
    }

    m_skeletalMeshIndex = index;
    m_pSkeletalMeshRenderProxy->SetSkeletalMesh(pSkeletalMesh);
    m_skeletalAnimationPlayer.SetSkeletalMesh(pSkeletalMesh);
    m_skeletalAnimationPlayer.ClearAllAnimations();
    m_skeletalAnimationIndex = m_animationNames.GetSize();

    // this needs to be fixed properly, ugh. because we're not being threadsafe main thread changes the animations..
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([](){});

    ResetCamera();
    return true;
}

bool SkeletalAnimationDemo::SetAnimationIndex(uint32 index)
{
    DebugAssert(index <= m_animationNames.GetSize());

    m_skeletalAnimationIndex = index;
    if (m_skeletalAnimationIndex == m_animationNames.GetSize())
    {
        m_skeletalAnimationPlayer.ClearAnimation();
        return true;
    }

    AutoReleasePtr<const SkeletalAnimation> pSkeletalAnimation = g_pResourceManager->GetSkeletalAnimation(m_animationNames[m_skeletalAnimationIndex]);
    if (pSkeletalAnimation == nullptr)
    {
        Log_ErrorPrintf("Failed to load skeletal animation '%s'", m_animationNames[m_skeletalAnimationIndex]);
        m_skeletalAnimationPlayer.ClearAnimation();
        return false;
    }

    m_skeletalAnimationPlayer.PlayAnimation(pSkeletalAnimation, 1.0f, -1, true);
    return true;
}

void SkeletalAnimationDemo::ResetCamera()
{
    const float3 moveVec(1.0f, -1.0f, 1.0f);
    const Sphere &boundingSphere = m_pSkeletalMeshRenderProxy->GetBoundingSphere();
    float3 cameraPos(boundingSphere.GetCenter() + moveVec * (boundingSphere.GetRadius() * 2.5f));
    m_camera.SetPosition(cameraPos);
    m_camera.SetRotation(Quaternion::FromEulerAngles(-30.0f, 0.0f, 45.0f));
}

bool LaunchpadGameState::InvokeSkeletalAnimationDemo()
{
    return RunDemo(new SkeletalAnimationDemo(m_pDemoGame));
}
