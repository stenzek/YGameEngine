struct VertexFactoryInput
{
    float3 Position             : POSITION;
    float3 Normal               : NORMAL;
    
#if LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS
    float3 Tangent              : TANGENT;
    float3 Binormal             : BINORMAL;
#endif

#if LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS
    float2 TexCoord             : TEXCOORD0;
#elif LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS
    float3 TexCoord             : TEXCOORD0;    
#endif

#if LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS
	float4 VertexColor		    : COLOR;
#endif

#if LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX
    float4 InstanceTransform0   : POSITION1;
    float4 InstanceTransform1   : POSITION2;
    float4 InstanceTransform2   : POSITION3;
#endif
};

#if LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX

float3x4 VertexFactoryGetInstanceTransform(VertexFactoryInput input) { return float3x4(input.InstanceTransform0, input.InstanceTransform1, input.InstanceTransform2); }

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(VertexFactoryGetInstanceTransform(input), float4(input.Position, 1.0f)).xyz; }
float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return input.Normal; }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)VertexFactoryGetInstanceTransform(input), input.Normal)); }

#else

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }
float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return input.Normal; }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)ObjectConstants.WorldMatrix, input.Normal)); }

#endif

#if LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
#elif LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0); }
#else
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
#endif

float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }

#if LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.VertexColor; }
#else
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
#endif

#if LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(input.Tangent, input.Binormal, input.Normal); }
#else
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), input.Normal); }
#endif
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }
