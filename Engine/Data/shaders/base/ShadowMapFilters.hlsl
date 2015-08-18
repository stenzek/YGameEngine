float PoissonDiscFilter(Texture2D textureObject, SamplerState samplerState, float2 textureCoordinates, float2 texelSize, float comparisonValue)
{	
    const float2 poissonDisk[24] = { 
        float2(0.5713538f, 0.7814451f),
        float2(0.2306823f, 0.6228884f),
        float2(0.1000122f, 0.9680607f),
        float2(0.947788f, 0.2773731f),
        float2(0.2837818f, 0.303393f),
        float2(0.6001099f, 0.4147638f),
        float2(-0.2314563f, 0.5434746f),
        float2(-0.08173513f, 0.0796717f),
        float2(-0.4692954f, 0.8651238f),
        float2(0.2768489f, -0.3682062f),
        float2(-0.5900795f, 0.3607553f),
        float2(-0.1010569f, -0.5284956f),
        float2(-0.4741178f, -0.2713854f),
        float2(0.4067073f, -0.00782522f),
        float2(-0.4603044f, 0.0511527f),
        float2(0.9820454f, -0.1295522f),
        float2(0.8187376f, -0.4105208f),
        float2(-0.8115796f, -0.106716f),
        float2(-0.4698426f, -0.6179109f),
        float2(-0.8402727f, -0.4400948f),
        float2(-0.2302377f, -0.879307f),
        float2(0.2748472f, -0.708988f),
        float2(-0.7874522f, 0.6162704f),
        float2(-0.9310728f, 0.3289311f)
    };
    
    // Number of samples
    #define TOTAL_SAMPLES 20

	float shadowAmount = 0;
	float sampleDiscSize = 1.7f;
	float2 pixelSize = texelSize * sampleDiscSize;

	// Sample the texture at various offsets

	[unroll]
	for (int i = 0; i < TOTAL_SAMPLES; i++)
		shadowAmount += (textureObject.Sample(samplerState, textureCoordinates + poissonDisk[i] * pixelSize).r >= comparisonValue);

	shadowAmount /= (TOTAL_SAMPLES + 1);
	return shadowAmount;
    
    #undef TOTAL_SAMPLES
}

float PCFFilter(Texture2D textureObject, SamplerState samplerState, float2 textureCoordinates, float2 texelSize, float comparisonValue)
{
    float value;
    float2 offset = texelSize * 0.5;
    //float2 offset = texelSize;
    
    value = (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, -offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(-offset.x, offset.y)).r >= comparisonValue);
    value += (textureObject.Sample(samplerState, textureCoordinates + float2(offset.x, offset.y)).r >= comparisonValue);
    
    return value / 4;
}
