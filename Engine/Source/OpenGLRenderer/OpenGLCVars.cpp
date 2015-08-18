#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLCommon.h"

namespace CVars
{
    // OpenGL CVars
    CVar r_opengl_disable_vertex_attrib_binding("r_opengl_disable_vertex_attrib_binding", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "Disable ARB_vertex_attrib_binding even if detected", "bool");
    CVar r_opengl_disable_direct_state_access("r_opengl_disable_direct_state_access", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "Disable EXT_direct_state_access even if detected", "bool");
    CVar r_opengl_disable_multi_bind("r_opengl_disable_multi_bind", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "Disable ARB_multi_bind even if detected", "bool");
}

