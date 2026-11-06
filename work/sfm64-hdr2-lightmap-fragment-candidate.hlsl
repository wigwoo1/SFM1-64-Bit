// Explicit opaque plain profile, linear UNORM output in the controlled draw.
// No inferred live Source exposure/gamma/atlas policy.
[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
[[vk::binding(0,1)]] Texture2D<float4> lightTexture;
[[vk::binding(1,1)]] SamplerState lightSampler;
#ifdef SELF_SHADOWED_BUMP
[[vk::binding(6,0)]] Texture2D<float4> bumpTexture;
[[vk::binding(9,0)]] SamplerState bumpSampler;
#endif
struct Push {
    [[vk::offset(112)]] float4 baseTransform0;
    [[vk::offset(128)]] float4 baseTransform1;
    [[vk::offset(144)]] float4 modulationExposure;
};
[[vk::push_constant]] ConstantBuffer<Push> state;
float4 main([[vk::location(0)]] float2 storedUv:TEXCOORD0,
            [[vk::location(3)]] float2 lightUv:TEXCOORD3):SV_Target0 {
    float3 sourceUv=float3(storedUv.x,1-storedUv.y,1);
    float2 uv=float2(dot(sourceUv,state.baseTransform0.xyz),dot(sourceUv,state.baseTransform1.xyz));
    float3 base=baseTexture.Sample(baseSampler,uv).rgb;
#ifdef SELF_SHADOWED_BUMP
    // lightUv addresses the base plane. Successive planes use identical
    // face-local coordinates and are spaced one face width apart in the page.
    float2 planeStep=float2(state.baseTransform0.w,0);
    float3 weights=bumpTexture.Sample(bumpSampler,sourceUv.xy).rgb;
    float3 light=weights.r*lightTexture.Sample(lightSampler,lightUv+planeStep).rgb
                +weights.g*lightTexture.Sample(lightSampler,lightUv+2*planeStep).rgb
                +weights.b*lightTexture.Sample(lightSampler,lightUv+3*planeStep).rgb;
#else
    float3 light=lightTexture.Sample(lightSampler,lightUv).rgb;
#endif
    return float4(((base*light)*state.modulationExposure.rgb)*state.modulationExposure.w,1);
}
