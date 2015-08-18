struct VertexFactoryInput
{
    uint VertexID : SV_VertexID;
};

float2 VFGetTexCoord(VertexFactoryInput input)
{
    return float2((input.VertexID << 1) & 2, input.VertexID & 2);
}

float2 VFGetPosition(VertexFactoryInput input)
{
    float2 tc = VFGetTexCoord(input);
    return float4(tc * float2(2, -2) + float2(-1, 1), 0, 1);
}

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return VFGetPosition(input); }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return VFGetPosition(input); }
float4 VertexFactoryGetViewPosition(VertexFactoryInput input) { return VFGetPosition(input); }
float4 VertexFactoryGetProjectedPosition(VertexFactoryInput input) { return VFGetPosition(input); }
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(VFGetTexCoord(input), 0, 0); }
float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }

float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }
