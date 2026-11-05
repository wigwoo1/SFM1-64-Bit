// Explicit opaque plain profile, linear UNORM output in the controlled draw.
// No inferred live Source exposure/gamma/atlas policy.
[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
[[vk::binding(0,1)]] Texture2D<float4> lightTexture;
[[vk::binding(1,1)]] SamplerState lightSampler;
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
    float3 light=lightTexture.Sample(lightSampler,lightUv).rgb;
    return float4(((base*light)*state.modulationExposure.rgb)*state.modulationExposure.w,1);
}
