// Lookup Tables
static const float3 CUBE_FACE_NORMALS[6] = {
    { 1, 0, 0 },        // RIGHT
    { -1, 0, 0 },       // LEFT
    { 0, 1, 0 },        // BACK
    { 0, -1, 0 },       // FRONT
    { 0, 0, 1 },        // TOP
    { 0, 0, -1 },       // BOTTOM
};

static const float3x3 CUBE_FACE_TANGENTBASIS[6] = {
    { 0, -1, 0, 0, 0, -1, 1, 0, 0 },        // RIGHT
    { 0, -1, 0, 0, 0, -1, -1, 0, 0 },       // LEFT
    { 1, 0, 0, 0, 0, -1, 0, 1, 0 },         // BACK
    { 1, 0, 0, 0, 0, -1, 0, -1, 0 },        // FRONT
    { 1, 0, 0, 0, -1, 0, 0, 0, 1 },         // TOP
    { 1, 0, 0, 0, -1, 0, 0, 0, -1 },        // BOTTOM
};

struct VertexFactoryInput
{
    float3 Position         : POSITION0;

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD && BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY
	float3 TexCoord         : TEXCOORD0;
#elif BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD
	float2 TexCoord         : TEXCOORD0;
#endif

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS
	float4 AtlasTexCoord    : TEXCOORD1;
#endif

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS
    float4 Color            : COLOR0;
#endif
#if BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX
    uint4 FaceIndex         : BLENDINDICES0;
#endif
};

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD && BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0); }
#elif BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
#else
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
#endif

float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.Color; }
#else
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
#endif

#if BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS
#error fixme!
#elif BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX
	float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return CUBE_FACE_NORMALS[input.FaceIndex.x]; }
    float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)ObjectConstants.WorldMatrix, CUBE_FACE_NORMALS[input.FaceIndex.x])); }
    float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return CUBE_FACE_TANGENTBASIS[input.FaceIndex.x]; }
	float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
	float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul(ObjectConstants.InverseWorldMatrix, float4(worldVector, 1.0)).xyz); }
#else
	float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
	float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
	float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1); }
	float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
	float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }
#endif

