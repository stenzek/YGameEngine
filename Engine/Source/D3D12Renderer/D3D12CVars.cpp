#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12Common.h"

namespace CVars
{
    // D3D11 cvars
    CVar r_d3d12_force_warp("r_d3d12_force_warp", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "force WARP renderer for Direct3D 12", "bool");
}

