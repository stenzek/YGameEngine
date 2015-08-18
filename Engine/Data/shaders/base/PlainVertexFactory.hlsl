struct VertexFactoryInput
{
    float3 Position         : POSITION0;
#if PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD
    float2 TexCoord         : TEXCOORD0;
#endif
#if PLAIN_VERTEX_FACTORY_FLAG_COLOR
    float4 VertexColor      : COLOR0;
#endif
};

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }

#if PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
#else
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
#endif

float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }

#if PLAIN_VERTEX_FACTORY_FLAG_COLOR
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.VertexColor; }
#else
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
#endif

float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }
