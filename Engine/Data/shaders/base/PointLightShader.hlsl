//------------------------------------------------------------------------------------------------------------
// PointLightShader.hlsl
// Shader for rendering an object with lighting contributions from a point light.
//------------------------------------------------------------------------------------------------------------

// From the material, we need the tangent normal, diffuse, and specular colors, as well as specular exponent.
#define MATERIAL_NEEDS_LIGHT_REFLECTION 1
#define MATERIAL_NEEDS_OUTPUT_COLOR 1

// Now, include the headers.
#include "Common.hlsl"
#include "VertexFactory.hlsl"
#include "Material.hlsl"

static const float SHADOW_MAP_DEPTH_BIAS = 0.001f;
static const float SHADOW_MAP_Z_NEAR = 0.1f;

// Input variables.
cbuffer PointLightParameters : register(b3) { struct
{
    float3 LightColor;
    float3 LightPosition;
    float LightRange;
    float LightInverseRange;
    float LightFalloffExponent;
} cbPointLightParameters; }

#if WITH_SHADOW_MAP
	TextureCube<float> ShadowMapTexture;
    #if USE_HARDWARE_PCF
        SamplerComparisonState ShadowMapTexture_SamplerState;
    #else
        SamplerState ShadowMapTexture_SamplerState;
    #endif
	
	float LightVectorToDepthValue(float3 lightVector)
	{
		float zNear = SHADOW_MAP_Z_NEAR;
		float zFar = 1.0f / cbPointLightParameters.LightInverseRange;
		
		float3 absVec = abs(lightVector);
		float maxDepth = max(absVec.x, max(absVec.y, absVec.z));
		
		//float rz = (zFar / (zNear - zFar)) * -maxDepth + (zNear * zFar / (zNear - zFar)) * 1.0f;
		//float rw = -1.0f * -maxDepth;
		//return rz / rw;
		
		return ((zFar / (zNear - zFar)) * -maxDepth + (zNear * zFar / (zNear - zFar))) / maxDepth;
	}
#endif

// VS to PS interpolants
struct VSToPSParameters
{
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    float3 TangentLightVector   : TangentLightVector;
    float3 TangentCameraVector  : TangentCameraVector;
	
	// The world space vector has to be included for shadow mapping.
	#if WITH_SHADOW_MAP
		float3 WorldLightVector     : WorldLightVector;
	#endif
#else
	float3 WorldCameraVector	: WorldCameraVector;
    float3 WorldLightVector     : WorldLightVector;
#endif
};

// Vertex shader.
void VSMain(in VertexFactoryInput in_input,
			out VSToPSParameters out_parameters,
            out MaterialPSInterpolants out_interpolants,
			out float4 out_screenPosition : SV_Position)
{
    float3 worldPosition = VertexFactoryGetWorldPosition(in_input);

#if MATERIAL_FEATURE_LEVEL >= FEATURE_LEVEL_ES3
    MaterialVSInputParameters MVIParameters = GetMaterialVSInputParametersFromVertexFactory(in_input);
    worldPosition += MaterialGetWorldPositionOffset(MVIParameters);
#endif
	
	// Calculate light and camera vectors (world space)
	float3 worldLightVector = worldPosition - cbPointLightParameters.LightPosition;
    float3 worldCameraVector = ViewConstants.EyePosition - worldPosition;
	
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE  
    // Retreive tangent basis vectors.
    float3x3 tangentBasis = VertexFactoryGetTangentBasis(in_input);   
    
    // Transform the light/camera vectors to tangent space, and store in output.
    out_parameters.TangentLightVector = VertexFactoryTransformWorldToTangentSpace(tangentBasis, worldLightVector);
    out_parameters.TangentCameraVector = VertexFactoryTransformWorldToTangentSpace(tangentBasis, worldCameraVector);
	
	// Sahdow mapping requires world light vector still.
	#if WITH_SHADOW_MAP
		out_parameters.WorldLightVector = worldLightVector;
	#endif
#else
	out_parameters.WorldLightVector = worldLightVector;
	out_parameters.WorldCameraVector = worldCameraVector; 
#endif
    
    out_interpolants = GetMaterialPSInterpolantsVS(in_input);
    out_screenPosition = mul(ViewConstants.ViewProjectionMatrix, float4(worldPosition, 1));    
}

void PSMain(in VSToPSParameters in_parameters,
			in MaterialPSInterpolants in_interpolants,            
            out float4 out_target : SV_Target)
{
    // Fill from vertex factory.
    MaterialPSInputParameters MPIParameters = GetMaterialPSInputParameters(in_interpolants);
    
#if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
    // Determine the direction and luminosity of the light at this pixel. [compiler should optimize the normalize/length to one sqrt]
	float lightDistance = length(in_parameters.TangentLightVector);
    float3 lightDirection = normalize(in_parameters.TangentLightVector);
    float3 viewDirection = normalize(in_parameters.TangentCameraVector);
#else
	// Determine the direction and luminosity of the light at this pixel.
	float lightDistance = length(in_parameters.WorldLightVector);
    float3 lightDirection = normalize(in_parameters.WorldLightVector);
    float3 viewDirection = normalize(in_parameters.WorldCameraVector);
#endif

    // determine light luminosity
	float lightCoefficient = pow(saturate(1.0f - lightDistance * cbPointLightParameters.LightInverseRange), cbPointLightParameters.LightFalloffExponent);
    
    // Shadows
#if WITH_SHADOW_MAP
    float shadowComparisonValue = LightVectorToDepthValue(in_parameters.WorldLightVector) - SHADOW_MAP_DEPTH_BIAS;
    #if USE_HARDWARE_PCF
        lightCoefficient *= ShadowMapTexture.SampleCmpLevelZero(ShadowMapTexture_SamplerState, in_parameters.WorldLightVector.xzy, shadowComparisonValue);
    #else
        float shadowValue = ShadowMapTexture.SampleLevel(ShadowMapTexture_SamplerState, in_parameters.WorldLightVector.xzy, 0.0f);
        lightCoefficient *= (shadowValue >= shadowComparisonValue);
    #endif
#endif

    // Calculate the lighting results, and return them.
    float3 sceneColor = MaterialGetLightReflection(MPIParameters, lightDirection, cbPointLightParameters.LightColor, lightCoefficient, viewDirection);

    // Calculate output colour    
    out_target = MaterialCalcOutputColor(MPIParameters, sceneColor);
}
