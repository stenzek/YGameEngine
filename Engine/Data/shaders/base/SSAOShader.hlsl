//------------------------------------------------------------------------------------------------------------
// SSAOShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

// ssao settings
static const float SAMPLE_RADIUS = 0.5f;
static const float SAMPLE_POWER = 3.0f;

// ssao kernel
#define KERNEL_SIZE 10
static const float3 KERNEL_OFFSETS[KERNEL_SIZE] =
{
/*float3(-0.200819, 0.811046, 0.549433),
float3(0.336032, 0.010670, 0.941790),
float3(-0.356416, 0.905260, 0.231238),
float3(0.407996, 0.675948, 0.613704),
float3(0.400277, 0.676286, 0.618398),
float3(0.103376, -0.780724, 0.616266),
float3(-0.705150, 0.667362, 0.239566),
float3(0.777408, -0.424702, 0.463967),
float3(-0.076263, 0.743072, 0.664851),
float3(-0.028973, -0.520448, 0.853401),
float3(-0.860162, -0.502136, 0.089333),
float3(-0.508712, 0.760016, 0.404460),
float3(-0.574750, 0.663699, 0.478712),
float3(0.143266, 0.424575, 0.893986),
float3(0.433384, 0.142516, 0.889869),
float3(0.458326, 0.639491, 0.617243)*/
/*normalize(float3(-1, -1, -1)),
normalize(float3(-1, -1, 1)),
normalize(float3(-1, 1, -1)),
normalize(float3(-1, 1, 1)),
normalize(float3( 1, -1, -1)),
normalize(float3( 1, -1, 1)),
normalize(float3( 1, 1, -1)),
normalize(float3( 1, 1, 1))*/
float3(-0.010735935, 0.01647018, 0.0062425877),
float3(-0.06533369, 0.3647007, -0.13746321),
float3(-0.6539235, -0.016726388, -0.53000957),
float3(0.40958285, 0.0052428036, -0.5591124),
float3(-0.1465366, 0.09899267, 0.15571679),
float3(-0.44122112, -0.5458797, 0.04912532),
float3(0.03755566, -0.10961345, -0.33040273),
float3(0.019100213, 0.29652783, 0.066237666),
float3(0.8765323, 0.011236004, 0.28265962),
float3(0.29264435, -0.40794238, 0.15964167)
};

// Resources
uniform float2 RandomTextureScale;
Texture2D<float3> RandomTexture;
SamplerState RandomTexture_SamplerState;

// Frame data
Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;
Texture2D NormalsTexture;
SamplerState NormalsTexture_SamplerState;

float SSAO(float3x3 kernelBasis, float3 originPos, float radius)
{
    float occlusion = 0.0f;
    [loop] for (int i = 0; i < KERNEL_SIZE; i++)
    {
        // acquire sample position
        float3 samplePos = mul(kernelBasis, KERNEL_OFFSETS[i]);
        samplePos = samplePos * radius + originPos;
        
        // project to texture space
        float4 sampleCoords = mul(ViewConstants.ProjectionMatrix, float4(samplePos, 1.0f));
        sampleCoords.xy /= sampleCoords.w;
        sampleCoords.xy = sampleCoords.xy * 0.5f + 0.5f;
        sampleCoords.y = 1.0f - sampleCoords.y;
        
        // sample depth
        float sampleDepth = DepthBuffer.SampleLevel(DepthBuffer_SamplerState, sampleCoords.xy, 0.0f);
        sampleDepth = LinearizeDepth(sampleDepth);
        
        // range check and apply occlusion
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(originPos.z - sampleDepth));
        occlusion += rangeCheck * step(sampleDepth, samplePos.z);
        //occlusion += (sampleDepth <= samplePos.z) ? 1.0f : 0.0f;
    }
    
    // fixup
    occlusion = 1.0f - (occlusion / (float)KERNEL_SIZE);
    return pow(occlusion, SAMPLE_POWER);
}

// Entry point
void PSMain2(in float2 in_screenTexCoord : TEXCOORD0,
            out float out_target : SV_Target)
{
    // reconstruct the view-space position of the pixel, and retrieve the view-space normal.
    float3 position = ReconstructViewSpacePosition(in_screenTexCoord, DepthBuffer.SampleLevel(DepthBuffer_SamplerState, in_screenTexCoord, 0.0f));
    float3 normal = normalize(UnpackFromColorRange3(NormalsTexture.SampleLevel(NormalsTexture_SamplerState, in_screenTexCoord, 0.0f).xyz));
    
    // sample the random texture and reorientate the sample kernel along the view normal
    float3 rvec = normalize(UnpackFromColorRange3(RandomTexture.SampleLevel(RandomTexture_SamplerState, in_screenTexCoord * RandomTextureScale, 0.0f)));
    float3 tangent = normalize(rvec - normal * dot(rvec, normal));
    float3 bitangent = cross(tangent, normal);
    float3x3 kernelBasis = float3x3(tangent, bitangent, normal);
    
    // calculate occlusion
    float occlusion = SSAO(kernelBasis, position, SAMPLE_RADIUS);
    
    // return occlusion
    out_target = occlusion;
}

void PSMain(in float2 in_screenTexCoord : TEXCOORD0,
            out float out_target : SV_Target)
{
    static const float strength = 0.125f;
    static const float falloff = 0.0000002f;
    //static const float rad = 0.006f;
    //static const float rad = 0.05f;

    float3 fres = normalize(UnpackFromColorRange3(RandomTexture.SampleLevel(RandomTexture_SamplerState, in_screenTexCoord * RandomTextureScale, 0.0f)));
    float3 normal = normalize(UnpackFromColorRange3(NormalsTexture.SampleLevel(NormalsTexture_SamplerState, in_screenTexCoord, 0.0f).xyz));
    float depth = DepthBuffer.SampleLevel(DepthBuffer_SamplerState, in_screenTexCoord, 0.0f);
    
    float rad = 0.25f / LinearizeDepth(depth);
    float3 ep = float3(in_screenTexCoord, depth);
    float bl = 0.0f;
    float radD = rad / depth;
       
    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        float3 ray = radD * reflect(KERNEL_OFFSETS[i], fres);
        
        float2 stc = ep.xy + sign(dot(ray, normal)) * ray.xy;
        float3 occNorm = normalize(UnpackFromColorRange3(NormalsTexture.SampleLevel(NormalsTexture_SamplerState, stc, 0.0f).xyz));
        float occDepth = DepthBuffer.SampleLevel(DepthBuffer_SamplerState, stc, 0.0f);
        float depthDiff = depth - occDepth;
        
        bl += step(falloff, depthDiff) * (1.0f - dot(occNorm, normal)) * (1.0f - smoothstep(falloff, strength, depthDiff));
    }
    
    float ao = 1.0f - bl * (1.0f / KERNEL_SIZE);
    out_target = ao;
}
