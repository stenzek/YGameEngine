#include "Engine/PrecompiledHeader.h"
#include "Engine/EngineCVars.h"

#if Y_BUILD_CONFIG_DEBUG && !Y_BUILD_CONFIG_DEBUGFAST
    static const char *ENABLED_BOOL_ON_DEBUG_BUILD = "true";
#else
    static const char *ENABLED_BOOL_ON_DEBUG_BUILD = "false";
#endif

namespace CVars
{
    // Engine cvars
    CVar e_worker_threads("e_worker_threads", CVAR_FLAG_REQUIRE_APP_RESTART, "-1", "number of worker threads, -1 to automatically decide, or 0 for none", "int");

    // Resource manager cvars
    CVar rm_enable_resource_compilation("rm_enable_resource_compilation", 0, "1", "Load uncompiled resources, if the modification time is newer than the compiled version.", "bool");
    CVar rm_maintenance_interval("rm_maintenance_interval", 0, "1", "Delay in seconds between resource manager maintenance calls");
    CVar rm_remote_resource_compiler_close_delay("rm_remote_resource_compiler_close_delay", 0, "15", "Delay in seconds between a resource compiler being created and then closed", "uint");

    // Physics cvars
    CVar physics_fps("physics_fps", 0, "60.0", "The (fixed) frame rate that physics simulates at.", "float:0-999");

    // Renderer cvars
    CVar r_platform("r_platform", CVAR_FLAG_REQUIRE_APP_RESTART, "", "Rendering API to use, empty is default for platform", "string:D3D9|D3D11|OPENGL|OPENGL_ES");
    CVar r_use_render_thread("r_use_render_thread", CVAR_FLAG_REQUIRE_APP_RESTART, "true", "Enable multithreaded rendering", "bool");
    CVar r_fullscreen("r_fullscreen", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Renderer uses fullscreen mode", "bool");
    CVar r_fullscreen_exclusive("r_fullscreen_exclusive", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Use exclusive fullscreen instead of borderless window.", "bool");
    CVar r_fullscreen_width("r_fullscreen_width", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1280", "Width of backbuffer", "uint");
    CVar r_fullscreen_height("r_fullscreen_height", CVAR_FLAG_REQUIRE_RENDER_RESTART, "720", "Height of backbuffer", "uint");
    CVar r_windowed_width("r_windowed_width", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1280", "Width of backbuffer", "uint");
    CVar r_windowed_height("r_windowed_height", CVAR_FLAG_REQUIRE_RENDER_RESTART, "720", "Height of backbuffer", "uint");
    CVar r_vsync("r_vsync", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Renderer uses vsync mode", "bool");
    CVar r_triple_buffering("r_triple_buffering", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Renderer uses triple buffering mode", "bool");
    CVar r_show_fps("r_show_fps", 0, "1", "Show current FPS on screen", "bool");
    CVar r_show_debug_info("r_show_debug_info", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1", "Show debug info", "bool");
    CVar r_show_frame_times("r_show_frame_times", 0, "1", "Show frame times on screen", "bool");
    CVar r_render_profiler("r_render_profiler", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Enable render profiler", "bool");
    CVar r_render_profiler_gpu_time("r_render_profiler_gpu_time", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Enable render profiler GPU time capturing", "bool");
    CVar r_texture_filtering("r_texture_filtering", CVAR_FLAG_REQUIRE_APP_RESTART, "3", "Default texture filter, 0 - point, 1 - bilinear, 2 - trilinear, 3 - anistropic", "uint:0-3");
    CVar r_max_anisotropy("r_max_anistropy", CVAR_FLAG_REQUIRE_APP_RESTART, "16", "Maximum anisotropy when using anisotropic filtering", "uint:1-16");
    CVar r_material_debug_mode("r_material_debug_mode", 0, "0", "Material debug mode", "uint");
    CVar r_wireframe("r_wireframe", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Draw world in wireframe", "bool");
    CVar r_fullbright("r_fullbright", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Draw world in fullbright", "bool");
    CVar r_debug_normals("r_debug_normals", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Render world-space normals", "bool");
    CVar r_deferred_shading("r_deferred_shading", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Enable deferred rendering path", "bool");
    CVar r_depth_prepass("r_depth_prepass", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Draw depth prepass", "bool");
    CVar r_show_skeletons("r_show_skeletons", CVAR_FLAG_PAUSE_RENDER_THREAD, "0", "Show skeletons for debugging", "bool");
    CVar r_gpu_skinning("r_gpu_skinning", CVAR_FLAG_PAUSE_RENDER_THREAD, "1", "Use gpu-accelerated skinning", "bool");
    CVar r_show_buffers("r_show_buffers", CVAR_FLAG_PAUSE_RENDER_THREAD, "0", "Show buffers used to construct the scene", "bool");
    CVar r_use_light_scissor_rect("r_use_light_scissor_rect", CVAR_FLAG_PAUSE_RENDER_THREAD, "false", "Use scissor rectangle for rendering point lights", "bool");
    CVar r_occlusion_culling("r_occlusion_culling", CVAR_FLAG_REQUIRE_RENDER_RESTART, "false", "Enable use of occlusion culling", "bool");
    CVar r_occlusion_culling_objects_per_buffer("r_occlusion_culling_objects_per_buffer", CVAR_FLAG_REQUIRE_RENDER_RESTART, "500", "Number of objects rendered per buffer for occlusion culling", "uint:16-1024");
    CVar r_occlusion_culling_wait_for_results("r_occlusion_culling_wait_for_results", CVAR_FLAG_PAUSE_RENDER_THREAD, "false", "Block until results come in before drawing next frame", "bool");
    CVar r_occlusion_prediction("r_occlusion_prediction", CVAR_FLAG_REQUIRE_RENDER_RESTART, "false", "Use occlusion queries for non-blocking predicated drawing", "bool");
    CVar r_shadows("r_shadows", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1", "Enable dynamic shadows, 0 - none, 1 - all, 2 - directional only", "uint:0-2");
    CVar r_shadow_map_bits("r_shadow_map_bits", CVAR_FLAG_REQUIRE_RENDER_RESTART, "16", "Number of bits in shadow map textures", "uint:16,24,32");
    CVar r_directional_shadow_map_resolution("r_directional_shadow_map_resolution", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1024", "Directional shadow map resolution", "uint:16-8192");
    CVar r_point_shadow_map_resolution("r_point_shadow_map_resolution", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1024", "Point shadow map resolution", "uint:16-8192");
    CVar r_spot_shadow_map_resolution("r_spot_shadow_map_resolution", CVAR_FLAG_REQUIRE_RENDER_RESTART, "1024", "Spot shadow map resolution", "uint:16-8192");
    CVar r_shadow_use_hardware_pcf("r_shadow_use_hardware_pcf", CVAR_FLAG_REQUIRE_RENDER_RESTART, "false", "Use hardware pcf where supported for shadow map filtering", "bool");
    CVar r_shadow_filtering("r_shadow_filtering", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Shadow filtering quality, 0 - 1x1 filter, 2 - 3x3 filter, 3 - 5x5 filter", "bool");
    CVar r_post_processing("r_post_processing", CVAR_FLAG_REQUIRE_RENDER_RESTART, "true", "Allow post processing effects", "bool");
    CVar r_ssao("r_ssao", CVAR_FLAG_REQUIRE_RENDER_RESTART, "true", "Enable screen-space ambient occlusion", "bool");
    CVar r_use_shader_cache("r_use_shader_cache", CVAR_FLAG_REQUIRE_APP_RESTART, "1", "Enables/disables overall use of the shader cache.", "bool");
    CVar r_allow_shader_cache_writes("r_allow_shader_cache_writes", CVAR_FLAG_REQUIRE_APP_RESTART, "1", "Enables/disables reads/writes of new shaders to user cache.", "bool");
    CVar r_use_debug_device("r_use_debug_device", CVAR_FLAG_REQUIRE_APP_RESTART, ENABLED_BOOL_ON_DEBUG_BUILD, "If set, a debug device is created for the renderer (huge performance cost).", "bool");
    CVar r_use_debug_shaders("r_use_debug_shaders", CVAR_FLAG_REQUIRE_APP_RESTART, ENABLED_BOOL_ON_DEBUG_BUILD, "If set, debug shaders are used instead of release (optimized) shaders.", "bool");
    CVar r_dump_shaders("r_dump_shaders", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "If set, shader source code will be dumped to a file.", "bool");
    CVar r_enable_multithreaded_resource_creation("r_enable_multithreaded_resource_creation", CVAR_FLAG_REQUIRE_APP_RESTART, "0", "Enabled multithreaded resource creation, if supported", "bool");
    CVar r_sprite_draw_instanced_quads("r_sprite_draw_instanced_quads", CVAR_FLAG_REQUIRE_APP_RESTART, "1", "Enable usage of instanced quads for sprite rendering", "bool");
    CVar r_emulate_mobile("r_emulate_mobile", CVAR_FLAG_REQUIRE_RENDER_RESTART, "0", "Emulate mobile rendering on desktop", "bool");

    // Renderer debug cvars
    CVar r_show_cascades("r_show_cascades", CVAR_FLAG_REQUIRE_RENDER_RESTART, "false", "Enable visualization of cascade selection", "bool");

    // Terrain renderer cvars
    CVar r_terrain_renderer("r_terrain_renderer", CVAR_FLAG_REQUIRE_MAP_RESTART, "cdlod", "Terrain renderer", "string:null|cdlod");
    CVar r_terrain_render_resolution_multiplier("r_terrain_render_resolution_multiplier", CVAR_FLAG_PAUSE_RENDER_THREAD, "1", "Terrain render resolution multiplier", "int32:1-10");
    CVar r_terrain_lod_distance_ratio("r_terrain_lod_distance_ratio", CVAR_FLAG_PAUSE_RENDER_THREAD, "2.0", "Terrain lod distance ratio", "float:1-10");
    CVar r_terrain_view_distance("r_terrain_view_distance", CVAR_FLAG_PAUSE_RENDER_THREAD, "2000", "Terrain render distance", "int:1-99999");
    CVar r_terrain_show_nodes("r_terrain_show_nodes", CVAR_FLAG_PAUSE_RENDER_THREAD, "0", "Show/hide terrain nodes", "bool");
}

