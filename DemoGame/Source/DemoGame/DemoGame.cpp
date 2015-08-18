#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/DemoGame.h"
#include "Engine/InputManager.h"
Log_SetChannel(BlockGame);

DemoGame::DemoGame()
    : BaseGame(),
      m_pLaunchpadGameState(nullptr)
{

}

DemoGame::~DemoGame()
{
    
}

void DemoGame::OnRegisterTypes()
{
    BaseGame::OnRegisterTypes();
}

bool DemoGame::OnStart()
{
    if (!BaseGame::OnStart())
        return false;

    m_pLaunchpadGameState = new LaunchpadGameState(this);
    SetNextGameState(m_pLaunchpadGameState);
    return true;
}

void DemoGame::OnExit()
{
    BaseGame::OnExit();

    //delete m_pLaunchpadGameState;
    m_pLaunchpadGameState = nullptr;
}
