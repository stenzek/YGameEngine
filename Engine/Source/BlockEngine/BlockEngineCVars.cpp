#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockEngineCVars.h"

namespace CVars
{
    CVar r_block_world_visible_radius("r_block_world_visible_radius", 0, "10", "Number of visible chunks around observer", "uint:4-256");
    CVar r_block_world_section_load_radius("r_block_world_section_load_radius", 0, "4", "Number of sections around player to load", "uint:1-256");
    CVar r_block_world_chunk_remove_delay("r_block_world_chunk_remove_delay", 0, "10", "Number of seconds to delay removing a previously visible chunk", "float:0-60");
    CVar r_block_world_parallel_chunk_build("r_block_world_parallel_chunk_build", CVAR_FLAG_REQUIRE_MAP_RESTART, "true", "Enable parallel chunk building", "bool");
    CVar r_block_world_max_chunks_per_frame("r_block_world_max_chunks_per_frame", 0, "100", "Maximum number of triangulation passes to perform each frame", "uint:1-256");
    CVar r_block_world_max_sections_per_frame("r_block_world_max_sections_per_frame", 0, "1", "Maximum number of sections to load/generate per frame", "uint:1-256");
    CVar r_block_world_occlusion("r_block_world_occlusion", 0, "1", "Use occlusion queries for block terrain chunks", "bool");
    CVar r_block_world_show_lods("r_block_world_show_lods", 0, "0", "Show lod via colours", "bool");
    CVar r_block_world_use_lightmaps("r_block_world_use_lightmaps", 0, "false", "Use lightmaps instead of dynamic lighting", "bool");
}

