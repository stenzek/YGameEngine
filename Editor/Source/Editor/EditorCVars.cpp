#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorCVars.h"

namespace CVars
{
    CVar e_camera_max_speed("e_camera_max_speed", 0, "15", "Maximum editor camera speed", "float:1-100");
    CVar e_camera_acceleration("e_camera_acceleration", 0, "60", "Velocity gained with camera movement per second", "float:1-100");
    CVar e_max_fps("e_max_fps", 0, "60", "Maximum FPS that the editor will run at", "float:1-9999");
    CVar e_max_fps_unfocused("e_max_fps_unfocused", 0, "15", "Maximum FPS that the editor will run at when alt-tabbed to another window", "float:1-9999");
}
