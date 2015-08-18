//------------------------------------------------------------------------------------------------------------
// DeferredCommon.hlsl
// 
//------------------------------------------------------------------------------------------------------------

// Shading model defines
#define DEFERRED_SHADING_MODEL_NONE (0)
#define DEFERRED_SHADING_MODEL_PHONG (1)
#define DEFERRED_SHADING_MODEL_BLINN_PHONG (2)
#define DEFERRED_SHADING_MODEL_PHYSICALLY_BASED (3)
#define DEFERRED_SHADING_MODEL_COUNT (4)

// Encoding of shading models
float EncodeShadingModel(uint shadingModel) { return float(shadingModel) / float(DEFERRED_SHADING_MODEL_COUNT); }
uint DecodeShadingModel(float encodedShadingModel) { return uint(encodedShadingModel * float(DEFERRED_SHADING_MODEL_COUNT)); }

// Selection of shading model from material defines
uint SelectShadingModelFromMaterialDefines()
{
#if MATERIAL_LIGHTING_MODEL_PHONG
    return DEFERRED_SHADING_MODEL_PHONG;
#elif MATERIAL_LIGHTING_MODEL_BLINN_PHONG
    return DEFERRED_SHADING_MODEL_BLINN_PHONG;
#elif MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED
    return DEFERRED_SHADING_MODEL_PHYSICALLY_BASED;
#else
    return DEFERRED_SHADING_MODEL_NONE;
#endif
}

// For phong shading, the maximum value of a specular power.
#define DEFERRED_SHADING_PHONG_SPECULAR_POWER_MAX (255.0f)

// Encoding specular powers.
float EncodeSpecularPower(float specularPower) { return saturate(specularPower / DEFERRED_SHADING_PHONG_SPECULAR_POWER_MAX); }
float DecodeSpecularPower(float encodedSpecularPower) { return encodedSpecularPower * DEFERRED_SHADING_PHONG_SPECULAR_POWER_MAX; }

// Buffers
Texture2D<float> DepthBuffer;
SamplerState DepthBuffer_SamplerState;
Texture2D<float4> GBuffer0;
SamplerState GBuffer0_SamplerState;
Texture2D<float4> GBuffer1;
SamplerState GBuffer1_SamplerState;
Texture2D<float4> GBuffer2;
SamplerState GBuffer2_SamplerState;

// GBuffer information
struct GBufferData
{
    float DepthBufferValue;
    float LinearDepth;
    float3 ViewSpacePosition;
    float3 ViewSpaceNormal;
    float3 BaseColor;
    float SpecularFactorOrMetallic;
    float SpecularPowerOrSpecular;
    float Roughness;
    float ShadowMask;    
    uint ShadingModel;
    bool TwoSidedLighting;
};

struct GBufferWriteData
{
    float3 BaseColor;
    float3 ViewNormal;
    float SpecularFactorOrMetallic;
    float SpecularPowerOrSpecular;
    float Roughness;
    float ShadowMask;    
    uint ShadingModel;
    bool TwoSidedLighting;
};    

// Write gbuffer outputs
void WriteGBufferData(in GBufferWriteData data,
                      out float4 out_GBuffer0Value,
                      out float4 out_GBuffer1Value,
                      out float4 out_GBuffer2Value)
{
    out_GBuffer0Value = float4(data.BaseColor.rgb, data.ShadowMask);
    out_GBuffer1Value = float4(PackToColorRange3(data.ViewNormal.xyz), data.Roughness);
    out_GBuffer2Value = float4(data.SpecularFactorOrMetallic, data.SpecularPowerOrSpecular, EncodeShadingModel(data.ShadingModel), float(data.TwoSidedLighting));
}

// Retrieve gbuffer data from resources using texel coordinates, skipping the depth and position
void _GetGBufferData(in int3 pixelCoordinates, inout GBufferData data)
{
    // sample the gbuffers
    float4 GBuffer0Value = GBuffer0.Load(pixelCoordinates);
    float4 GBuffer1Value = GBuffer1.Load(pixelCoordinates);
    float4 GBuffer2Value = GBuffer2.Load(pixelCoordinates);
    
    // store values
    data.ViewSpaceNormal = UnpackFromColorRange3(GBuffer1Value.xyz);
    data.BaseColor = GBuffer0Value.rgb;
    data.SpecularFactorOrMetallic = GBuffer2Value.r;
    data.SpecularPowerOrSpecular = GBuffer2Value.g;
    data.Roughness = GBuffer1Value.a;
    data.ShadowMask = GBuffer0Value.a;
    data.ShadingModel = DecodeShadingModel(GBuffer2Value.b);
    data.TwoSidedLighting = any(GBuffer2Value.a);
}

// Retrieve gbuffer data from resources using a view ray
void GetGBufferDataFromViewRay(in float3 viewRay, in float2 screenPosition, out GBufferData data)
{
    // reconstruct the view-space position of the pixel
    // https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    int3 pixelCoordinates = int3(screenPosition.xy, 0);
    float depthBufferValue = DepthBuffer.Load(pixelCoordinates);
    float linearDepth = LinearizeDepth(depthBufferValue);
    float3 positionVS = viewRay * linearDepth;
    
    // sample the gbuffers
    float4 GBuffer0Value = GBuffer0.Load(pixelCoordinates);
    float4 GBuffer1Value = GBuffer1.Load(pixelCoordinates);
    float4 GBuffer2Value = GBuffer2.Load(pixelCoordinates);
    
    // get data
    data.DepthBufferValue = depthBufferValue;
    data.LinearDepth = linearDepth;
    data.ViewSpacePosition = positionVS;
    _GetGBufferData(pixelCoordinates, data);
}

// Lighting function
float3 CalculateDeferredLighting(in GBufferData data, in float3 lightVector, float3 lightColor, in float3 viewVector)
{
    float3 outColor;
    switch (data.ShadingModel)
    {
    case DEFERRED_SHADING_MODEL_PHONG:
        {
            float diffuse = CalculateLambertianDiffuse(data.ViewSpaceNormal, lightVector);
            float specular = CalculatePhongSpecular(data.ViewSpaceNormal, DecodeSpecularPower(data.SpecularPowerOrSpecular), lightVector, viewVector) * data.SpecularFactorOrMetallic;
            
            if (data.TwoSidedLighting)
            {
                diffuse += CalculateLambertianDiffuse(-data.ViewSpaceNormal, lightVector);
            }
            
            outColor = data.BaseColor * lightColor * (diffuse + specular);
        }
        break;
    
    case DEFERRED_SHADING_MODEL_BLINN_PHONG:
        {
            float diffuse = CalculateLambertianDiffuse(data.ViewSpaceNormal, lightVector);
            float specular = CalculateBlinnPhongSpecular(data.ViewSpaceNormal, DecodeSpecularPower(data.SpecularPowerOrSpecular), lightVector, viewVector) * data.SpecularFactorOrMetallic;
            
            if (data.TwoSidedLighting)
            {
                diffuse += CalculateLambertianDiffuse(-data.ViewSpaceNormal, lightVector);
            }
            
            outColor = data.BaseColor * lightColor * (diffuse + specular);        
        }
        break;
        
    case DEFERRED_SHADING_MODEL_PHYSICALLY_BASED:
        {
            outColor = float3(0.0f, 0.0f, 0.0f);
        }
        break;
        
    default:
        outColor = float3(0.0f, 0.0f, 0.0f);
        break;
    }
    
    return outColor;   
}
