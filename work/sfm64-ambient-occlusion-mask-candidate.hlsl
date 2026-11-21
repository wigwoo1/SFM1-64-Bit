// Mode 0 screen-space occlusion. The front buffer stores signed world normals
// in RGB and positive linear view depth in alpha. Background depth is zero.
// The mask, where one is unoccluded, belongs to the linear scene-lighting pass.
[[vk::binding(0, 0)]] Texture2D<float4> frontNormalDepth;
[[vk::binding(1, 0)]] Texture2D<float4> reflectionNoise;
[[vk::binding(2, 0)]] SamplerState depthSampler;
[[vk::binding(3, 0)]] SamplerState noiseSampler;
[[vk::binding(4, 0)]] RWTexture2D<float> occlusionMask;

[[vk::binding(5, 0)]] cbuffer OcclusionView {
    // At each pixel center, interpolate the view ray in world coordinates.
    float4 topLeftRay;
    float4 horizontalRaySpan;
    float4 verticalRaySpan;
    float3 eyePosition;
    float normalBias;
    float4 eyeForward;
    float4 worldToClipX;
    float4 worldToClipY;
    float4 worldToClipW;
    float2 noiseScale;
    float2 noiseOffset;
    float2 projectionCenter;
    float sampleRadius;
    float inverseFalloff;
    float4 sampleDirections[16];
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint2 extent;
    occlusionMask.GetDimensions(extent.x, extent.y);
    if (any(id.xy >= extent))
        return;

    float2 uv = (id.xy + 0.5) / extent;
    float4 surface = frontNormalDepth.SampleLevel(depthSampler, uv, 0);
    if (surface.w <= 0) {
        occlusionMask[id.xy] = 1;
        return;
    }

    float3 noise = reflectionNoise.SampleLevel(noiseSampler, id.xy * noiseScale + noiseOffset, 0).xyz;
    float3 reflectionAxis = noise * 2 - 1;
    float3 viewRay = topLeftRay.xyz + uv.x * horizontalRaySpan.xyz + uv.y * verticalRaySpan.xyz;
    float3 origin = eyePosition + surface.w * (viewRay + surface.xyz * normalBias);
    float weightedOcclusion = 0;
    const float distanceBias = 0.618;
    [loop]
    for (uint sampleIndex = 0; sampleIndex < 16; ++sampleIndex) {
        float3 direction = reflect(sampleDirections[sampleIndex].xyz, reflectionAxis);
        if (dot(direction, surface.xyz) < 0)
            direction = reflect(direction, surface.xyz);
        float3 samplePosition = origin + direction * (sampleRadius * 0.25);
        float4 world = float4(samplePosition, 1);
        float clipW = dot(world, worldToClipW);
        if (clipW == 0)
            continue;
        float2 projected = float2(dot(world, worldToClipX), dot(world, worldToClipY)) / clipW;
        float2 sampleUv = projectionCenter + projected * float2(0.5, -0.5);
        float sceneDepth = frontNormalDepth.SampleLevel(depthSampler, sampleUv, 0).w;
        precise float gap = (dot(samplePosition - eyePosition, eyeForward.xyz) - sceneDepth) * inverseFalloff;
        float weight = saturate(rcp(gap + distanceBias) - distanceBias);
        weightedOcclusion += weight * saturate(gap * 10000000.0);
    }
    float visibility = 1 - weightedOcclusion / 16;
    occlusionMask[id.xy] = visibility * visibility;
}
