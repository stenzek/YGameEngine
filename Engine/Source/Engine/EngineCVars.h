#pragma once
#include "Engine/Common.h"

namespace CVars
{
    // Engine cvars
    extern CVar e_worker_threads;

    // Resource manager cvars
    extern CVar rm_enable_resource_compilation;
    extern CVar rm_maintenance_interval;
    extern CVar rm_remote_resource_compiler_close_delay;

    // Physics cvars
    extern CVar physics_fps;

    // Renderer cvars
    extern CVar r_platform;
    extern CVar r_use_render_thread;
    extern CVar r_multithreaded_rendering;
    extern CVar r_gpu_latency;
    extern CVar r_fullscreen;
    extern CVar r_fullscreen_exclusive;
    extern CVar r_fullscreen_width;
    extern CVar r_fullscreen_height;
    extern CVar r_windowed_width;
    extern CVar r_windowed_height;
    extern CVar r_vsync;
    extern CVar r_triple_buffering;
    extern CVar r_show_fps;
    extern CVar r_show_debug_info;
    extern CVar r_show_frame_times;
    extern CVar r_texture_filtering;
    extern CVar r_max_anisotropy;
    extern CVar r_material_debug_mode;
    extern CVar r_wireframe;
    extern CVar r_fullbright;
    extern CVar r_debug_normals;
    extern CVar r_deferred_shading;
    extern CVar r_depth_prepass;
    extern CVar r_show_skeletons;
    extern CVar r_gpu_skinning;
    extern CVar r_show_buffers;
    extern CVar r_use_light_scissor_rect;
    extern CVar r_occlusion_culling;
    extern CVar r_occlusion_culling_objects_per_buffer;
    extern CVar r_occlusion_culling_wait_for_results;
    extern CVar r_occlusion_prediction;
    extern CVar r_shadows;
    extern CVar r_shadow_map_bits;
    extern CVar r_directional_shadow_map_resolution;
    extern CVar r_point_shadow_map_resolution;
    extern CVar r_spot_shadow_map_resolution;
    extern CVar r_shadow_use_hardware_pcf;
    extern CVar r_shadow_filtering;
    extern CVar r_post_processing;
    extern CVar r_ssao;
    extern CVar r_use_shader_cache;
    extern CVar r_allow_shader_cache_writes;
    extern CVar r_use_debug_device;
    extern CVar r_use_debug_shaders;
    extern CVar r_dump_shaders;
    extern CVar r_enable_multithreaded_resource_creation;
    extern CVar r_sprite_draw_instanced_quads;
    extern CVar r_emulate_mobile;

    // Renderer debug cvars
    extern CVar r_show_cascades;

    // Terrain renderer cvars
    extern CVar r_terrain_renderer;
    extern CVar r_terrain_render_resolution_multiplier;
    extern CVar r_terrain_lod_distance_ratio;
    extern CVar r_terrain_view_distance;
    extern CVar r_terrain_show_nodes;
}

