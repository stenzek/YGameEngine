#pragma once

#define D3D12_PLACED_CONSTANT_BUFFER_ALIGNMENT (65536)          // 64KiB
#define D3D12_BUFFER_WRITE_SCRATCH_BUFFER_THRESHOLD (1024)      // 1KiB
#define D3D12_CONSTANT_BUFFER_ALIGNMENT (256)                   // 256B
#define D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS (4)
#define D3D12_LEGACY_GRAPHICS_ROOT_SHADER_RESOURCE_SLOTS (8)
#define D3D12_LEGACY_GRAPHICS_ROOT_SHADER_SAMPLER_SLOTS (8)
#define D3D12_LEGACY_GRAPHICS_ROOT_PER_DRAW_CONSTANT_BUFFER_SLOTS (1)

// Since map read/write ranges breaks the hell out of the debugger, this lets us compile it out
#ifdef Y_BUILD_CONFIG_DEBUG
    #define D3D12_MAP_RANGE_PARAM(value) nullptr
#else
    #define D3D12_MAP_RANGE_PARAM(value) value
#endif

namespace NameTables {
    // This nametable is actually located in the D3D11 library.
    Y_Declare_NameTable(D3DFeatureLevels);
}