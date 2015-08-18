#pragma once
#include "BaseGame/BaseGame.h"
#include "DemoGame/LaunchpadGameState.h"

// game class
class DemoGame : public BaseGame
{
public:
    DemoGame();
    virtual ~DemoGame();

protected:
    // events
    virtual void OnRegisterTypes() override final;
    virtual bool OnStart() override final;
    virtual void OnExit() override final;

    // bind input events
    void BindInputEvents();

private:
    LaunchpadGameState *m_pLaunchpadGameState;
};
