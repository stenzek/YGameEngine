struct VertexFactoryInput
{
    float2 Position         : POSITION0;
};

float3 TerrainPatchOffset;						// in world units
float3 TerrainPatchSize;						// in world units
float2 TerrainPatchOffsetInTextureSpace;		// in texture space
float2 TerrainPatchSizeInTextureSpace;			// in texture space
float2 TerrainHeightMapDimensions;				// in units, y = 1.0 / x
float3 TerrainGridDimensions;
float4 TerrainMorphConstants;

Texture2D TerrainHeightMapTexture;
SamplerState TerrainHeightMapTexture_SamplerState;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float2 TerrainGetHeightMapTextureCoordinates(float2 patchTexCoord)
{
    float2 heightmapTextureCoordinates = TerrainPatchOffsetInTextureSpace + (TerrainPatchSizeInTextureSpace * patchTexCoord);
    return heightmapTextureCoordinates;
}

float TerrainSampleHeight(float2 heightMapTexCoord)
{
	return TerrainHeightMapTexture.SampleLevel(TerrainHeightMapTexture_SamplerState, heightMapTexCoord, 0).r * TerrainPatchSize.z + TerrainPatchOffset.z;
}

float3 TerrainSampleNormal(float2 heightMapTexCoord)
{
    float val = TerrainHeightMapTexture.SampleLevel(TerrainHeightMapTexture_SamplerState, heightMapTexCoord, 0).r;
    float valU = TerrainHeightMapTexture.SampleLevel(TerrainHeightMapTexture_SamplerState, heightMapTexCoord + float2(TerrainHeightMapDimensions.y, 0), 0).r;
    float valV = TerrainHeightMapTexture.SampleLevel(TerrainHeightMapTexture_SamplerState, heightMapTexCoord + float2(0, TerrainHeightMapDimensions.y), 0).r;
    return normalize(float3(val - valU, val - valV, 0.5));
}

float3x3 TerrainGetTangentBasisFromNormal(float3 normal)
{
	float3 tangentX = normalize(float3(normal.z, 0, -normal.x));
	float3 tangentY = cross(normal, tangentX);
	return float3x3(tangentX, tangentY, normal);
}

float3x3 TerrainGetTangentToWorldFromTangentBasis(float3x3 tangentBasis)
{
	return transpose(tangentBasis);
}

float2 TerrainMorphVertex(float2 position, float morphLerpK)
{
    float2 fracPart = frac(position * float2(TerrainGridDimensions.y, TerrainGridDimensions.y)) * float2(TerrainGridDimensions.z, TerrainGridDimensions.z);
    return position - fracPart * morphLerpK;
}

float2 TerrainPatchCoordinatesToWorldCoordinatesXY(float2 patchCoordinates)
{
    float2 worldPosition = float2(TerrainPatchOffset.xy + patchCoordinates.xy * TerrainPatchSize.xy);                                  
    return worldPosition;
}

float3 TerrainPatchCoordinatesToWorldCoordinates(float2 patchCoordinates, float height)
{
    float3 worldPosition = float3(TerrainPatchOffset.xy + patchCoordinates.xy * TerrainPatchSize.xy, height);                                  
    return worldPosition;
}

float2 TerrainPatchCoordinatesToWorldTextureCoordinates(float2 patchCoordinates)
{
    float2 textureCoordinates = float2(TerrainPatchOffset.xy + patchCoordinates.xy * TerrainPatchSize.xy);                                       
    return textureCoordinates;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0

float2 TerrainGetPatchTextureCoordinates(VertexFactoryInput input)
{
    return input.Position;
}

#else

float2 TerrainGetPatchTextureCoordinates(VertexFactoryInput input)
{
    // calculate unmorphed position
    float2 unmorphedHeightmapTextureCoordinates = TerrainPatchOffsetInTextureSpace + (TerrainPatchSizeInTextureSpace * input.Position);
    float unmorphedHeight = TerrainSampleHeight(unmorphedHeightmapTextureCoordinates);
    float3 unmorphedPosition = TerrainPatchCoordinatesToWorldCoordinates(input.Position, unmorphedHeight);
    
    // handle morphing
    float eyeDistance = distance(TerrainPatchCoordinatesToWorldCoordinatesXY(input.Position), ViewConstants.EyePosition.xy);
    //float eyeDistance = distance(unmorphedPosition, ViewConstants.EyePosition);
    float morphLerpK = 1.0f - clamp(TerrainMorphConstants.z - eyeDistance * TerrainMorphConstants.w, 0.0f, 1.0f);
    float2 morphedPatchTextureCoordinates = TerrainMorphVertex(input.Position, morphLerpK);
    return morphedPatchTextureCoordinates;
}

#endif

float3 TerrainGetWorldPosition(VertexFactoryInput input)
{
    float2 patchTextureCoordinates = TerrainGetPatchTextureCoordinates(input);
    float2 heightmapTextureCoordinates = TerrainGetHeightMapTextureCoordinates(patchTextureCoordinates);   
    float height = TerrainSampleHeight(heightmapTextureCoordinates);

    // convert to world space
    return TerrainPatchCoordinatesToWorldCoordinates(patchTextureCoordinates, height);
}

float3 TerrainGetWorldNormal(VertexFactoryInput input)
{
    float2 patchTextureCoordinates = TerrainGetPatchTextureCoordinates(input);
    float2 heightmapTextureCoordinates = TerrainGetHeightMapTextureCoordinates(patchTextureCoordinates);   
    
    return TerrainSampleNormal(heightmapTextureCoordinates);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float3 VertexFactoryGetLocalPosition(VertexFactoryInput input) { return TerrainGetWorldPosition(input); }

#if TERRAIN_VERTEX_FACTORY_ENABLE_LOCAL_TO_WORLD
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return mul(ObjectConstants.WorldMatrix, float4(TerrainGetWorldPosition(input), 1)).xyz; }
#else
float3 VertexFactoryGetWorldPosition(VertexFactoryInput input) { return TerrainGetWorldPosition(input); }
#endif

float4 VertexFactoryGetTexCoord(VertexFactoryInput input) { return float4(TerrainPatchCoordinatesToWorldTextureCoordinates(TerrainGetPatchTextureCoordinates(input)), 0, 0); }
float4 VertexFactoryGetTexCoord2(VertexFactoryInput input) { return float4(TerrainGetHeightMapTextureCoordinates(TerrainGetPatchTextureCoordinates(input)), 0, 0); }
float4 VertexFactoryGetVertexColor(VertexFactoryInput input) { return float4(1, 1, 1, 1); }
float3 VertexFactoryGetLocalNormal(VertexFactoryInput input) { return TerrainGetWorldNormal(input); }
float3 VertexFactoryGetWorldNormal(VertexFactoryInput input) { return TerrainGetWorldNormal(input); }
float3x3 VertexFactoryGetTangentBasis(VertexFactoryInput input) { return TerrainGetTangentBasisFromNormal(TerrainGetWorldNormal(input)); }
float3x3 VertexFactoryGetTangentToWorld(VertexFactoryInput input, float3x3 tangentBasis) { return TerrainGetTangentToWorldFromTangentBasis(tangentBasis); }
float3 VertexFactoryTransformWorldToTangentSpace(float3x3 tangentBasis, float3 worldVector) { return mul(tangentBasis, worldVector.xyz); }
