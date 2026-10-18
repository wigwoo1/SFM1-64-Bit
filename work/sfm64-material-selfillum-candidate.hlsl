// Linear-input selfillum candidate. Scene lighting, gamma and native parity pending.
[[vk::binding(0, 0)]] Texture2D<float4> baseTexture;
[[vk::binding(1, 0)]] SamplerState baseSampler;
[[vk::binding(2, 0)]] Texture2D<float4> selfIllumMask;
struct MaterialPush {
    [[vk::offset(96)]] float alpha;
    [[vk::offset(100)]] float alphaCutoff;
    [[vk::offset(104)]] float selfIllumEnabled;
    [[vk::offset(108)]] float separateMask;
    [[vk::offset(112)]] float3 linearTint;
    [[vk::offset(124)]] float vertexLighting;
    [[vk::offset(128)]] float4 baseTextureTransform0;
    [[vk::offset(144)]] float4 baseTextureTransform1;
};
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
float4 main(float2 uv : TEXCOORD0, float3 vertexLight : TEXCOORD1) : SV_Target0 {
    float2 transformedUv = float2(dot(float3(uv,1),material.baseTextureTransform0.xyz),dot(float3(uv,1),material.baseTextureTransform1.xyz));
    float4 sampled = baseTexture.Sample(baseSampler, transformedUv);
    if (sampled.a < material.alphaCutoff) discard;
    if (material.vertexLighting != 0) sampled.rgb *= vertexLight;
    if (material.selfIllumEnabled != 0) {
        float3 mask = sampled.aaa;
        if (material.separateMask != 0) mask = selfIllumMask.Sample(baseSampler, transformedUv).rgb;
        sampled.rgb = sampled.rgb + mask * (material.linearTint * sampled.rgb - sampled.rgb);
    }
    sampled.a *= material.alpha;
    return sampled;
}
