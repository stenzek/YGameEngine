#pragma once

#define D3D12_CONSTANT_BUFFER_ALIGNMENT (65536)             // 64KiB
#define D3D12_BUFFER_WRITE_SCRATCH_BUFFER_THRESHOLD (1024)  // 1KiB

namespace NameTables {
    // This nametable is actually located in the D3D11 library.
    Y_Declare_NameTable(D3DFeatureLevels);
}