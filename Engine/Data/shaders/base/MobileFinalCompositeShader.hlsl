//------------------------------------------------------------------------------------------------------------
// ToneMapShader.hlsl
// 
//------------------------------------------------------------------------------------------------------------
#include "Common.hlsl"

Texture2D SceneTexture;
SamplerState SceneTexture_SamplerState;

void PSMain(in float2 screenTexCoord : TEXCOORD0,
            out float3 out_target : SV_Target)
{
#ifdef __GLSL__
    // need to flip to opengl convention
    float2 fixedTexCoord = float2(screenTexCoord.x, 1.0f - screenTexCoord.y);
#else
    // need to flip to opengl convention
    float2 fixedTexCoord = screenTexCoord;
#endif
    
    //float3 sceneColor = SceneTexture.SampleLevel(SceneTexture_SamplerState, fixedTexCoord, 0).rgb;
    float3 sceneColor = SceneTexture.Sample(SceneTexture_SamplerState, fixedTexCoord).rgb;  
    out_target = ApplyGammaCorrection(sceneColor);
}
