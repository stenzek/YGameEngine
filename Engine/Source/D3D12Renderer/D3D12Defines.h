#pragma once

#define D3D12_CONSTANT_BUFFER_ALIGNMENT (65536)                 // 64KiB
#define D3D12_BUFFER_WRITE_SCRATCH_BUFFER_THRESHOLD (1024)      // 1KiB
#define D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS (4)
#define D3D12_LEGACY_GRAPHICS_ROOT_SHADER_RESOURCE_SLOTS (8)
#define D3D12_LEGACY_GRAPHICS_ROOT_SHADER_SAMPLER_SLOTS (8)

namespace NameTables {
    // This nametable is actually located in the D3D11 library.
    Y_Declare_NameTable(D3DFeatureLevels);
}