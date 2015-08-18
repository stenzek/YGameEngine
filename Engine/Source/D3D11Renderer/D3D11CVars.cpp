#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11Common.h"

namespace CVars
{
    // D3D11 cvars
    CVar r_d3d11_force_ref("r_d3d11_force_ref", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "force reference renderer for Direct3D 11", "bool");
    CVar r_d3d11_force_warp("r_d3d11_force_warp", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "force WARP renderer for Direct3D 11", "bool");
    CVar r_d3d11_use_11_1("r_d3d11_use_11_1", CVAR_FLAG_REQUIRE_APP_RESTART, "1", "use Direct3D 11.1 enhancements if available", "bool");
}

