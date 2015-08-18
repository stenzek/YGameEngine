//------------------------------------------------------------------------------------------------------------
// ScreenQuadVertexShader.hlsl
// Renders a full-screen quad, providing texture coordinates to the pixel shader corresponding
// to the pixel currently being shaded.
//------------------------------------------------------------------------------------------------------------

#if GENERATE_VIEW_RAY
#include "Common.hlsl"

float3 GetViewRay(float4 position)
{
    //float tanHalfFOV = tan(ViewConstants.PerspectiveFOV * 0.5f);
    //return float3(position.x * tanHalfFOV * ViewConstants.PerspectiveAspectRatio, position.y * tanHalfFOV, 1.0f);
    float4 positionVS = mul(ViewConstants.InverseProjectionMatrix, position);
    return float3(positionVS.xy / positionVS.z, 1.0f);
}

#endif

#if GENERATE_ON_GPU

void Main(in uint vertexID : SV_VertexID,
          out float2 out_texCoord : TEXCOORD0,
#if GENERATE_VIEW_RAY
          out float3 out_viewRay : VIEWRAY,
#endif
          out float4 out_position : SV_Position)
{
    float2 texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    float4 clipSpacePosition = float4(texCoord * float2(2, -2) + float2(-1, 1), 0, 1);
    
    out_texCoord = texCoord;
#if GENERATE_VIEW_RAY    
    out_viewRay = GetViewRay(clipSpacePosition);
#endif
    out_position = clipSpacePosition;
}

#else

void Main(in float2 in_position : POSITION,
          in float2 in_texCoord : TEXCOORD,
          out float2 out_texCoord : TEXCOORD0,
#if GENERATE_VIEW_RAY
          out float3 out_viewRay : VIEWRAY,
#endif
          out float4 out_position : SV_Position)
{
    //float2 texCoord = (in_position.xy - float2(-1.0f, 1.0f)) * float2(1.0f / 2.0f, 1.0f / -2.0f);
    
    float4 clip_position = float4(in_position, 0.0f, 1.0f);
    out_texCoord = in_texCoord;
#if GENERATE_VIEW_RAY
    out_viewRay = GetViewRay(clip_position);
#endif
    out_position = clip_position;
}

#endif
