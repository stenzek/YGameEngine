struct VertexFactoryInput
{
    float3 Position                 : POSITION;
	float3 TexCoord                 : TEXCOORD;
    float4 Color                    : COLOR;
    float4 TangentAndBinormalSign   : TANGENT;
    float4 Normal                   : NORMAL;
    //float3 Tangent                  : TANGENT;
    //float3 Binormal                 : BINORMAL;
    //float3 Normal                   : NORMAL;
};

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0); }
float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(0, 0, 0, 0); }
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(input.Color.xyz, 1.0f); }
float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return input.Normal.xyz; }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return normalize(mul((float3x3)ObjectConstants.WorldMatrix, input.Normal.xyz)); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) 
{ 
    float3 tangent = /*UnpackFromColorRange3*/(input.TangentAndBinormalSign.xyz);
    float3 normal = /*UnpackFromColorRange3*/(input.Normal.xyz);
    float3 binormal = cross(normal, tangent) * (input.TangentAndBinormalSign.w);// * 2.0f + -1.0f);
    return float3x3(tangent, binormal, normal);
    //return float3x3(input.Tangent, input.Binormal, input.Normal);
}

float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return mul((float3x3)ObjectConstants.WorldMatrix, transpose(tangentBasis)); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }

struct VertexFactoryLightMapInterpolants
{
    float LightMapValue : LIGHTMAPVALUE;
};

VertexFactoryLightMapInterpolants VertexFactoryGetLightMapInterpolants(VertexFactoryInput input)
{
    VertexFactoryLightMapInterpolants interpolants;
    interpolants.LightMapValue = input.Color.a;
    return interpolants;
}

float3 VertexFactoryGetLightMapColor(VertexFactoryLightMapInterpolants interpolants)
{
    return interpolants.LightMapValue.rrr;
}
