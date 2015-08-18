// Basic rendering
#if PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_RENDER_BASIC

struct VertexFactoryInput
{
    float3 Position         : POSITION0;
    float2 TexCoord         : TEXCOORD0;
    float4 VertexColor      : COLOR0;
};

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return input.Position; }
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(input.Position, 1)).xyz; }
float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(input.TexCoord, 0, 0); }
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return input.VertexColor; }

// Instanced quad rendering
#elif PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_RENDER_INSTANCED_QUADS

static const float2 LUT_PARTICLE_POSITION_MUL[4] =
{
    float2(-0.5f, 0.5f),
    float2(-0.5f, -0.5f),
    float2(0.5, 0.5),
    float2(0.5, -0.5f)
};

static const float2 LUT_PARTICLE_TEXCOORD_MUL[4] =
{
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

struct VertexFactoryInput
{
    float4 PositionRotation : POSITION0;
    float4 TexCoordRange    : TEXCOORD0;
    float2 Dimensions       : TEXCOORD1;
    float4 VertexColor      : COLOR0;
    uint VertexID           : SV_VertexID;
};

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input)
{ 
    // build the position based on vertex index
    float2 vertexPosition = LUT_PARTICLE_POSITION_MUL[input.VertexID] * input.Dimensions;
    
    // handle rotations
    float sinTheta, cosTheta;
    sincos(input.PositionRotation.w, sinTheta, cosTheta);
    vertexPosition = float2(vertexPosition.x * cosTheta - vertexPosition.y * sinTheta,
                            vertexPosition.x * sinTheta + vertexPosition.y * cosTheta);
    
    // rotate so it faces the camera, and add the offset
    return mul((float3x3)ViewConstants.InverseViewMatrix, float3(vertexPosition, 0.0f)) + input.PositionRotation.xyz;
}

float3 VertexFactoryGetWorldPosition(VertexFactoryInput input)
{ 
    float3 localPosition = VertexFactoryGetLocalPosition(input);
#if PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_USE_WORLD_TRANSFORM
    return mul(ObjectConstants.WorldMatrix, float4(localPosition, 1)).xyz;
#else
    return localPosition;
#endif
}

float4 VertexFactoryGetTexCoord(VertexFactoryInput input)
{
    // build texture coordinates
    float2 texCoord = float2(LUT_PARTICLE_TEXCOORD_MUL[input.VertexID] * input.TexCoordRange.zw + input.TexCoordRange.xy);
    return float4(texCoord, 0, 0);
}

float4 VertexFactoryGetVertexColor(VertexFactoryInput input)
{ 
    return input.VertexColor;
}

#endif  // PARTICLE_SYSTEM_SPRITE_VERTEX_FACTORY_RENDER_INSTANCED_QUADS

float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return float3(0, 0, 1); }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return -ViewConstants.ViewMatrix[2].xyz; }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return float3x3(-ViewConstants.ViewMatrix[0].xyz, ViewConstants.ViewMatrix[1].xyz, -ViewConstants.ViewMatrix[2].xyz); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return float3x3(-ViewConstants.ViewMatrix[0].xyz, ViewConstants.ViewMatrix[1].xyz, -ViewConstants.ViewMatrix[2].xyz); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, mul((float3x3)ObjectConstants.InverseWorldMatrix, worldVector)); }
