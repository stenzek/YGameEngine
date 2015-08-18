#pragma once
#include "DemoGame/BaseDemoWorldGameState.h"
#include "GameFramework/DirectionalLightEntity.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Engine/StaticMesh.h"

class DrawCallStressDemo : public BaseDemoWorldGameState
{
public:
    DrawCallStressDemo(DemoGame *pDemoGame);
    virtual ~DrawCallStressDemo();

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
    void CreateObjects(uint32 count, bool useInstancing, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    DirectionalLightEntity *m_pSunLightEntity;
    float3 m_sunLightRotation;

    const StaticMesh *m_pMeshToRender;
    PODArray<StaticMeshRenderProxy *> m_meshProxies;
    uint32 m_newObjectCount;
};

