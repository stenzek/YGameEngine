#pragma once
#include "DemoGame/BaseDemoWorldGameState.h"
#include "GameFramework/DirectionalLightEntity.h"
#include "Renderer/RenderProxies/SkeletalMeshRenderProxy.h"
#include "Engine/SkeletalAnimationPlayer.h"

class SkeletalAnimationDemo : public BaseDemoWorldGameState
{
public:
    SkeletalAnimationDemo(DemoGame *pDemoGame);
    virtual ~SkeletalAnimationDemo();

    virtual bool Initialize() override final;
    virtual void Shutdown() override final;

protected:
    virtual bool OnWorldCreated(World *pWorld) override final;
    virtual void OnWorldDeleted(World *pWorld) override final;

    virtual void DrawOverlays(float deltaTime) override final;
    virtual void DrawUI(float deltaTime) override final;

    virtual bool OnWindowEvent(const union SDL_Event *event) override final;
    virtual void OnMainThreadTick(float deltaTime) override final;

private:
    bool SetModelIndex(uint32 index);
    bool SetAnimationIndex(uint32 index);
    void ResetCamera();

    DirectionalLightEntity *m_pSunLightEntity;
    float3 m_sunLightRotation;

    SkeletalMeshRenderProxy *m_pSkeletalMeshRenderProxy;
    SkeletalAnimationPlayer m_skeletalAnimationPlayer;

    PODArray<const char *> m_animationNames;
    uint32 m_skeletalMeshIndex;
    uint32 m_skeletalAnimationIndex;
};

