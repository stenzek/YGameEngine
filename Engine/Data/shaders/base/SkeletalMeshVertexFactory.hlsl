struct VertexFactoryInput
{
    float3 Position         : POSITION0;
    float3 TangentX         : TANGENT0;
    float3 TangentY         : BINORMAL0;
    float3 TangentZ         : NORMAL0;

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS
    float2 TexCoord         : TEXCOORD0;
#endif
#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS
	float4 VertexColor		: COLOR0;
#endif
#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING
    #if FEATURE_LEVEL == FEATURE_LEVEL_SM2
        float4 BoneIndices    : BLENDINDICES0;
    #else
        uint4 BoneIndices   : BLENDINDICES0;
    #endif
    float4 BoneWeights      : BLENDWEIGHTS0;
#endif
};

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING

// Ughly as hell hack, fix me properly.
#if FEATURE_LEVEL == FEATURE_LEVEL_SM2
    float4x4 BoneMatrices4x4[200];
    float4x4 GetBoneMatrix(float index) { return BoneMatrices4x4[int(index)]; }
#else
    float3x4 BoneMatrices[200];
    float3x4 GetBoneMatrix(uint index) { return BoneMatrices[index]; }
#endif

struct SkeletalMeshVertexFactorySkinnedVertex
{
    float3 Position;
    float3 TangentX;
    float3 TangentY;
    float3 TangentZ;
};

SkeletalMeshVertexFactorySkinnedVertex VertexFactoryGetSkinnedVertex(VertexFactoryInput input)
{
    SkeletalMeshVertexFactorySkinnedVertex output;
    
#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_0_ENABLED
    output.Position = mul(GetBoneMatrix(input.BoneIndices.x), float4(input.Position, 1)).xyz * input.BoneWeights.x;
    output.TangentX = mul((float3x3)GetBoneMatrix(input.BoneIndices.x), input.TangentX) * input.BoneWeights.x;
    output.TangentY = mul((float3x3)GetBoneMatrix(input.BoneIndices.x), input.TangentY) * input.BoneWeights.x;
    output.TangentZ = mul((float3x3)GetBoneMatrix(input.BoneIndices.x), input.TangentZ) * input.BoneWeights.x;
#else
    output.Position = float4(0, 0, 0, 1);
    output.TangentX = float3(0, 0, 0);
    output.TangentY = float3(0, 0, 0);
    output.TangentZ = float3(0, 0, 0);
#endif

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_1_ENABLED
    output.Position += mul(GetBoneMatrix(input.BoneIndices.y), float4(input.Position, 1)).xyz * input.BoneWeights.y;
    output.TangentX += mul((float3x3)GetBoneMatrix(input.BoneIndices.y), input.TangentX) * input.BoneWeights.y;
    output.TangentY += mul((float3x3)GetBoneMatrix(input.BoneIndices.y), input.TangentY) * input.BoneWeights.y;
    output.TangentZ += mul((float3x3)GetBoneMatrix(input.BoneIndices.y), input.TangentZ) * input.BoneWeights.y;
#endif

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_2_ENABLED
    output.Position += mul(GetBoneMatrix(input.BoneIndices.z), float4(input.Position, 1)).xyz * input.BoneWeights.z;
    output.TangentX += mul((float3x3)GetBoneMatrix(input.BoneIndices.z), input.TangentX) * input.BoneWeights.z;
    output.TangentY += mul((float3x3)GetBoneMatrix(input.BoneIndices.z), input.TangentY) * input.BoneWeights.z;
    output.TangentZ += mul((float3x3)GetBoneMatrix(input.BoneIndices.z), input.TangentZ) * input.BoneWeights.z;
#endif

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_3_ENABLED
    output.Position += mul(GetBoneMatrix(input.BoneIndices.w), float4(input.Position, 1)).xyz * input.BoneWeights.w;
    output.TangentX += mul((float3x3)GetBoneMatrix(input.BoneIndices.w), input.TangentX) * input.BoneWeights.w;
    output.TangentY += mul((float3x3)GetBoneMatrix(input.BoneIndices.w), input.TangentY) * input.BoneWeights.w;
    output.TangentZ += mul((float3x3)GetBoneMatrix(input.BoneIndices.w), input.TangentZ) * input.BoneWeights.w;
#endif

    // normalize tangent frame
    output.TangentX = normalize(output.TangentX);
    output.TangentY = normalize(output.TangentY);
    output.TangentZ = normalize(output.TangentZ);

    return output;
}

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return VertexFactoryGetSkinnedVertex(input).Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(VertexFactoryGetSkinnedVertex(input).Position, 1)).xyz; }

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
#else
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
#endif

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.VertexColor; }
#else
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
#endif

float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return VertexFactoryGetSkinnedVertex(input).TangentZ; }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)ObjectConstants.WorldMatrix, VertexFactoryGetSkinnedVertex(input).TangentZ)); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(VertexFactoryGetSkinnedVertex(input).TangentX, VertexFactoryGetSkinnedVertex(input).TangentY, VertexFactoryGetSkinnedVertex(input).TangentZ); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }

#else       // SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
#else
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
#endif

float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }

#if SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.VertexColor; }
#else
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
#endif

float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return input.TangentZ; }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)ObjectConstants.WorldMatrix, input.TangentZ)); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(input.TangentX, input.TangentY, input.TangentZ); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }

#endif      // SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING
