#include "Defines.hlsl"

// Constant buffer wrapper, used when compiling for OpenGL ES
#if __OPENGL_ES__ && __GLSL__ < 300
    #define BEGIN_CONSTANT_BUFFER(BufferName, InstanceName, Register) struct BufferName
    #define END_CONSTANT_BUFFER(BufferName, InstanceName) ; uniform BufferName InstanceName;
    #else
    //#define BEGIN_CONSTANT_BUFFER(BufferName, InstanceName, Register) cbuffer BufferName : register(Register) { struct BufferName##_s
    #define BEGIN_CONSTANT_BUFFER(BufferName, InstanceName, Register) cbuffer BufferName { struct BufferName##_s
    #define END_CONSTANT_BUFFER(BufferName, InstanceName) InstanceName; }
#endif

#if !PLATFORM_D3D12
// Constant buffers
BEGIN_CONSTANT_BUFFER(ObjectConstantsBuffer, ObjectConstants, b1)
{
    float4x4 WorldMatrix;
    float4x4 InverseWorldMatrix;
    float4 MaterialTintColor;
}
END_CONSTANT_BUFFER(ObjectConstantsBuffer, ObjectConstants)
#else
cbuffer ObjectConstantsBuffer : register(b4) { struct ObjectConstants_s 
{
    float4x4 WorldMatrix;
    float4x4 InverseWorldMatrix;
    float4 MaterialTintColor;
} ObjectConstants; }
#endif

// Camera Constants
BEGIN_CONSTANT_BUFFER(ViewConstantsBuffer, ViewConstants, b2)
{
    float4x4 ViewMatrix;
    float4x4 InverseViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 InverseProjectionMatrix;
    float4x4 ViewProjectionMatrix;
    float4x4 InverseViewProjectionMatrix;
    float4x4 ScreenProjectionMatrix;
    float3 EyePosition;
    float WorldTime;
    float LogrithmicToLinearZDenominator;
    float LogrithmicToLinearZNumerator;
    float ZNear;
    float ZFar;
    float PerspectiveAspectRatio;
    float PerspectiveFOV;
    float2 ViewportOffset;
    float2 ViewportSize;
    float2 ViewportOffsetFraction;
    float2 InverseViewportSize;
}
END_CONSTANT_BUFFER(ViewConstantsBuffer, ViewConstants)

// Packs a vector in the range of [-1, 1] to [0, 1].
// Assumes the input vector has been normalized.
float PackToColorRange(float Input)
{
	return Input * 0.5 + 0.5;
}
float2 PackToColorRange2(float2 Input)
{
	return Input * 0.5 + 0.5;
}
float3 PackToColorRange3(float3 Input)
{
	return Input * 0.5 + 0.5;
}
float4 PackToColorRange4(float4 Input)
{
	return Input * 0.5 + 0.5;
}
float UnpackFromColorRange(float Input)
{
	//return (Input - 0.5) * 2.0;
    return Input * 2.0f + -1.0f;
}
float2 UnpackFromColorRange2(float2 Input)
{
	//return (Input - 0.5) * 2.0;
    return Input * 2.0f + -1.0f;
}
float3 UnpackFromColorRange3(float3 Input)
{
	//return (Input - 0.5) * 2.0;
    return Input * 2.0f + -1.0f;
}
float4 UnpackFromColorRange4(float4 Input)
{
	//return (Input - 0.5) * 2.0;
    return Input * 2.0f + -1.0f;
}

// calculate the lambertian diffuse reflectance
float CalculateLambertianDiffuse(float3 surfaceNormal, float3 lightVector)
{
    return saturate(dot(surfaceNormal, -lightVector));
}

// calculate phong specular highlights
float CalculatePhongSpecular(float3 surfaceNormal, float specularExponent, float3 lightDirection, float3 viewDirection)
{
    float3 reflectionVector = reflect(lightDirection, surfaceNormal);
    return pow(saturate(dot(reflectionVector, viewDirection)), specularExponent);
}

// calculate blinn-phong specular highlights
float CalculateBlinnPhongSpecular(float3 surfaceNormal, float specularExponent, float3 lightDirection, float3 viewDirection)
{
    // compute half angle
    float3 H = normalize(lightDirection + viewDirection);
    float NdotH = dot(surfaceNormal, H);
    return pow(saturate(NdotH), specularExponent);
}

// linearize a depth buffer value
float LinearizeDepth(float depthBufferValue)
{
    // https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/ with negated far plane distance
    return ViewConstants.LogrithmicToLinearZNumerator / (depthBufferValue - ViewConstants.LogrithmicToLinearZDenominator);
}

// reconstruct view-space position from a depth buffer sample, expects texture coordinates in [0..1]
float3 ReconstructViewSpacePosition(float2 textureCoords, float depth)
{
    float4 screenPositionW = float4(textureCoords.x * 2.0f - 1.0f, (1.0f - textureCoords.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 viewPositionW = mul(ViewConstants.InverseProjectionMatrix, screenPositionW);
    return viewPositionW.xyz / viewPositionW.w;
}

// reconstruct view-space position from a depth buffer sample, expects texture coordinates in [0..1]
float3 ReconstructWorldSpacePosition(float2 textureCoords, float depth)
{
    float4 screenPositionW = float4(textureCoords.x * 2.0f - 1.0f, (1.0f - textureCoords.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPositionW = mul(ViewConstants.InverseViewProjectionMatrix, screenPositionW);
    return worldPositionW.xyz / worldPositionW.w;
}

// calculate the luminance value of a color
float CalculateLuminance(float3 color)
{
    return dot(color, float3(0.212655f, 0.715158f, 0.072187f));
}

// gamma correction, implemented as a #define so to allow all data types @todo custom gamma value
//#define ApplyGammaCorrection(value) (pow((value), 1.0f / 2.2f))
#define ReverseGammaCorrection(value) (pow((value), 2.2f))
float3 ApplyGammaCorrection(float3 color)
{
    // temporary fix for hlsl-to-glsl
    //return pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
    return pow(color, 1.0f / 2.2f);
}

float4 ApplyGammaCorrection(float4 color)
{
    // temporary fix for hlsl-to-glsl
    return pow(color, float4(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f, 1.0f));
}

