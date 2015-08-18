
//! End User Code

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Factory -> Material Adapter
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MaterialGSInterpolants
{
#if MATERIAL_NEEDS_GS_INPUT_WORLD_POSITION || MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    float3 WorldPosition        : MaterialWorldPosition;
#endif
#if MATERIAL_NEEDS_GS_INPUT_WORLD_NORMAL || MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    float3 WorldNormal          : MaterialWorldNormal;
#endif
#if MATERIAL_NEEDS_GS_INPUT_TANGENTTOWORLD || MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    float3 TangentToWorld0      : MaterialTangentToWorld0;
    float3 TangentToWorld1      : MaterialTangentToWorld1;
    float3 TangentToWorld2      : MaterialTangentToWorld2;
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD || MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    float4 TexCoord             : MaterialTexCoord;
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD_2 || MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    float4 TexCoord2            : MaterialTexCoord2;
#endif
#if MATERIAL_NEEDS_GS_INPUT_VERTEX_COLOR || MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    float4 VertexColor          : MaterialVertexColor;
#endif
};

struct MaterialPSInterpolants
{
#if MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    float3 WorldPosition        : MaterialWorldPosition;
#endif
#if MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    float3 WorldNormal          : MaterialWorldNormal;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    float3 TangentToWorld0      : MaterialTangentToWorld0;
    float3 TangentToWorld1      : MaterialTangentToWorld1;
    float3 TangentToWorld2      : MaterialTangentToWorld2;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    float4 TexCoord             : MaterialTexCoord;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    float4 TexCoord2            : MaterialTexCoord2;
#endif
#if MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    float4 VertexColor          : MaterialVertexColor;
#endif
#if MATERIAL_NEEDS_PS_INPUT_SCREEN_POSITION && PIXEL_SHADER
    // Only read not written.
    float4 ScreenPosition       : SV_Position;
#endif
};

//#if VERTEX_SHADER

#if WITH_VERTEX_FACTORY

MaterialVSInputParameters GetMaterialVSInputParametersFromVertexFactory(VertexFactoryInput input)
{
    MaterialVSInputParameters params;

#if MATERIAL_NEEDS_VS_INPUT_WORLD_POSITION
    params.WorldPosition = VertexFactoryGetWorldPosition(input);
#endif
#if MATERIAL_NEEDS_VS_INPUT_WORLD_NORMAL
    params.WorldNormal = VertexFactoryGetWorldNormal(input);
#endif
#if MATERIAL_NEEDS_VS_INPUT_TANGENTTOWORLD
    float3x3 vTangentBasis = VertexFactoryGetTangentBasis(input);
    float3x3 vTangentToWorld = VertexFactoryGetTangentToWorld(input, vTangentBasis);
    params.TangentToWorld0 = vTangentToWorld[0];
    params.TangentToWorld1 = vTangentToWorld[1];
    params.TangentToWorld2 = vTangentToWorld[2];
#endif
#if MATERIAL_NEEDS_VS_INPUT_TEXCOORD
    params.TexCoord0 = VertexFactoryGetTexCoord(input);
#endif
#if MATERIAL_NEEDS_VS_INPUT_TEXCOORD_2
    params.TexCoord1 = VertexFactoryGetTexCoord2(input);
#endif
#if MATERIAL_NEEDS_VS_INPUT_VERTEX_COLOR
    params.VertexColor = VertexFactoryGetVertexColor(input);
#endif

    return params;
}

MaterialGSInterpolants GetMaterialGSInterpolants(VertexFactoryInput input)
{
    MaterialGSInterpolants interpolants;

#if MATERIAL_NEEDS_GS_INPUT_WORLD_POSITION || MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    interpolants.WorldPosition = VertexFactoryGetWorldPosition(input);
#endif
#if MATERIAL_NEEDS_GS_INPUT_WORLD_NORMAL || MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    interpolants.WorldNormal = VertexFactoryGetWorldNormal(input);
#endif
#if MATERIAL_NEEDS_GS_INPUT_TANGENTTOWORLD || MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    float3x3 vTangentBasis = VertexFactoryGetTangentBasis(input);
    float3x3 vTangentToWorld = VertexFactoryGetTangentToWorld(input, vTangentBasis);
    interpolants.TangentToWorld0 = vTangentToWorld[0];
    interpolants.TangentToWorld1 = vTangentToWorld[1];
    interpolants.TangentToWorld2 = vTangentToWorld[2];
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD || MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    interpolants.TexCoord = VertexFactoryGetTexCoord(input);
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD_2 || MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    interpolants.TexCoord2 = VertexFactoryGetTexCoord2(input);
#endif
#if MATERIAL_NEEDS_GS_INPUT_VERTEX_COLOR || MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    interpolants.VertexColor = VertexFactoryGetVertexColor(input);
#endif

    return interpolants;
}

MaterialPSInterpolants GetMaterialPSInterpolantsVS(VertexFactoryInput input)
{
    MaterialPSInterpolants interpolants;

#if MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    interpolants.WorldPosition = VertexFactoryGetWorldPosition(input);
#endif
#if MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    interpolants.WorldNormal = VertexFactoryGetWorldNormal(input);
#endif
#if MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    float3x3 vTangentBasis = VertexFactoryGetTangentBasis(input);
    float3x3 vTangentToWorld = VertexFactoryGetTangentToWorld(input, vTangentBasis);
    interpolants.TangentToWorld0 = vTangentToWorld[0];
    interpolants.TangentToWorld1 = vTangentToWorld[1];
    interpolants.TangentToWorld2 = vTangentToWorld[2];
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    interpolants.TexCoord = VertexFactoryGetTexCoord(input);
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    interpolants.TexCoord2 = VertexFactoryGetTexCoord2(input);
#endif
#if MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    interpolants.VertexColor = VertexFactoryGetVertexColor(input);
#endif

    return interpolants;
}

#endif                          // WITH_VERTEX_FACTORY

//#elif GEOMETRY_SHADER           // VERTEX_SHADER

MaterialGSInputParameters GetMaterialGSInputParameters(MaterialGSInterpolants interpolants)
{
    MaterialGSInputParameters params;

#if MATERIAL_NEEDS_GS_INPUT_WORLD_POSITION
    params.WorldPosition = interpolants.WorldPosition;
#endif
#if MATERIAL_NEEDS_GS_INPUT_WORLD_NORMAL
    params.WorldNormal = interpolants.WorldNormal;
#endif
#if MATERIAL_NEEDS_GS_INPUT_TANGENTTOWORLD
    params.TangentToWorld = float3x4(interpolants.TangentToWorld0, interpolants.TangentToWorld1, interpolants.TangentToWorld2);
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD
    params.TexCoord0 = interpolants.TexCoord;
#endif
#if MATERIAL_NEEDS_GS_INPUT_TEXCOORD_2
    params.TexCoord1 = interpolants.TexCoord2;
#endif
#if MATERIAL_NEEDS_GS_INPUT_VERTEX_COLOR
    params.VertexColor = interpolants.VertexColor;
#endif

    return params;
}

MaterialPSInterpolants GetMaterialPSInterpolantsGS(MaterialGSInterpolants in_interpolants)
{
    MaterialPSInterpolants out_interpolants;

#if MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    out_interpolants.WorldPosition = in_interpolants.WorldPosition;
#endif
#if MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    out_interpolants.WorldNormal = in_interpolants.WorldNormal;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    out_interpolants.TangentToWorld0 = in_interpolants.TangentToWorld0;
    out_interpolants.TangentToWorld1 = in_interpolants.TangentToWorld1;
    out_interpolants.TangentToWorld2 = in_interpolants.TangentToWorld2;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    out_interpolants.TexCoord = in_interpolants.TexCoord;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    out_interpolants.TexCoord2 = in_interpolants.TexCoord2;
#endif
#if MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    out_interpolants.VertexColor = in_interpolants.VertexColor;
#endif

    return out_interpolants;
}

//#elif PIXEL_SHADER          // GEOMETRY_SHADER

MaterialPSInputParameters GetMaterialPSInputParameters(MaterialPSInterpolants interpolants)
{
    MaterialPSInputParameters params;
    

#if MATERIAL_NEEDS_PS_INPUT_WORLD_POSITION
    params.WorldPosition = interpolants.WorldPosition;
#endif
#if MATERIAL_NEEDS_PS_INPUT_WORLD_NORMAL
    params.WorldNormal = interpolants.WorldNormal;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TANGENTTOWORLD
    params.TangentToWorld = float3x3(interpolants.TangentToWorld0, interpolants.TangentToWorld1, interpolants.TangentToWorld2);
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD
    params.TexCoord0 = interpolants.TexCoord;
#endif
#if MATERIAL_NEEDS_PS_INPUT_TEXCOORD_2
    params.TexCoord1 = interpolants.TexCoord2;
#endif
#if MATERIAL_NEEDS_PS_INPUT_VERTEX_COLOR
    params.VertexColor = interpolants.VertexColor;
#endif
#if MATERIAL_NEEDS_PS_INPUT_SCREEN_POSITION && PIXEL_SHADER
    params.ScreenPosition = interpolants.ScreenPosition;
#endif

    return params;
}

//#endif      // PIXEL_SHADER

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if MATERIAL_NEEDS_WORLD_POSITION

float3 MaterialGetWorldPosition(MaterialPSInputParameters inputParameters)
{
    return inputParameters.WorldPosition;
}

#endif

#if MATERIAL_NEEDS_WORLD_NORMAL
    
float3 MaterialGetWorldNormal(MaterialPSInputParameters inputParameters)
{
    float3 normal = MaterialGetNormal(inputParameters);
    #if MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE
        normal = mul(inputParameters.TangentToWorld, normal);
    #endif
    return normal;
}

#endif      // MATERIAL_NEEDS_WORLD_NORMAL

#if MATERIAL_NEEDS_LIGHT_REFLECTION

float3 MaterialGetLightReflection(MaterialPSInputParameters inputParameters, float3 lightVector, float3 lightColor, float lightCoefficient, float3 viewVector)
{
#if MATERIAL_LIGHTING_MODEL_PHONG || MATERIAL_LIGHTING_MODEL_BLINN_PHONG

    // Get the surface normal
    float3 surfaceNormal = MaterialGetNormal(inputParameters);
    
    // Get base colour
    float3 baseColor = MaterialGetBaseColor(inputParameters);
    
    // Calculate lambertian diffuse
    float3 diffuseColor = baseColor * lightColor * CalculateLambertianDiffuse(surfaceNormal, lightVector);
    
    // Common to both models
    float specularCoefficient = MaterialGetSpecularCoefficient(inputParameters);
    float specularExponent = MaterialGetSpecularExponent(inputParameters);
    
    #if MATERIAL_LIGHTING_MODEL_PHONG
        // Compute phong specular
        float specularIntensity = CalculatePhongSpecular(surfaceNormal, specularExponent, lightVector, viewVector) * specularCoefficient;        
    #else
        // Calculate blinn-phong specular
        float specularIntensity = CalculateBlinnPhongSpecular(surfaceNormal, specularExponent, lightVector, viewVector) * specularCoefficient;        
    #endif
    
    // Calculate specular colour
    float3 specularColor = lightColor * specularIntensity;
    
    // Add these together and return
    return (diffuseColor + specularColor) * lightCoefficient;
    
#elif MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED

    // TODO
    return float3(1, 1, 1);
    
#elif MATERIAL_LIGHTING_MODEL_CUSTOM
    
    // Pass through
    return MaterialGetCustomLighting(inputParameters, lightVector, lightColor, lightCoefficient, viewVector);
    
#else

    // Shouldn't ever be here
    return float3(0, 0, 0);

#endif
}

#endif      // MATERIAL_NEEDS_LIGHT_REFLECTION

#if MATERIAL_NEEDS_OUTPUT_COLOR

float4 MaterialCalcOutputColor(MaterialPSInputParameters inputParameters, float3 sceneColor)
{
	float4 outColor;
	
#if MATERIAL_BLENDING_MODE_STRAIGHT
    float opacity = MaterialGetOpacity(inputParameters);
    outColor = float4(sceneColor.rgb * opacity, opacity);
#elif MATERIAL_BLENDING_MODE_PREMULTIPLIED
    outColor = float4(sceneColor.rgb, MaterialGetOpacity(inputParameters));
#elif MATERIAL_BLENDING_MODE_MASKED
    clip(MaterialGetOpacity(inputParameters) - 0.8);
    outColor = float4(sceneColor, 1.0);
#elif MATERIAL_BLENDING_MODE_SOFTMASKED
    outColor = float4(sceneColor.rgb, MaterialGetOpacity(inputParameters));
#else
    outColor = float4(sceneColor, 1.0);
#endif

#if MATERIAL_TINT
	// Premultiplied alpha expects premultiplied values coming in to the blender, so fix that up here.
	#if MATERIAL_BLENDING_MODE_PREMULTIPLIED
		outColor *= ObjectConstants.MaterialTintColor;
		outColor.rgb *= ObjectConstants.MaterialTintColor.a;
	#else
		outColor *= ObjectConstants.MaterialTintColor;
	#endif
#endif

	return outColor;
}

#endif      // MATERIAL_NEEDS_OUTPUT_COLOR
