#pragma once
#include "ResourceCompiler/Common.h"

class MaterialShader;
class Skeleton;

struct ResourceCompilerCallbacks
{
    // Get the contents of a specific file
    virtual BinaryBlob *GetFileContents(const char *name) = 0;

    // Get the blob of a MaterialShader (needed because Material depends on it)
    virtual const MaterialShader *GetCompiledMaterialShader(const char *name) = 0;

    // Get the blob of a skeleton (needed because SkeletalMesh/SkeletalAnimation depend on it)
    virtual const Skeleton *GetCompiledSkeleton(const char *name) = 0;
};

