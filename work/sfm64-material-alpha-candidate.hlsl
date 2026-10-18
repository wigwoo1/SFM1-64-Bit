// Textured preview with optional BSP vertex-lightmap modulation and per-draw alpha.
// Full native Source gamma/lightmap interpolation and material parity remain unverified.
[[vk::binding(0, 0)]] Texture2D<float4> baseTexture;
[[vk::binding(1, 0)]] SamplerState baseSampler;
[[vk::binding(3, 0)]] Texture2D<float4> secondBaseTexture;
struct MaterialPush {
    [[vk::offset(96)]] float alpha;
    [[vk::offset(100)]] float alphaCutoff;
    [[vk::offset(104)]] float vertexLighting;
    [[vk::offset(108)]] float vertexTransition;
    [[vk::offset(112)]] float4 baseTextureTransform0;
    [[vk::offset(128)]] float4 baseTextureTransform1;
};
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
float4 main(float2 uv : TEXCOORD0, float3 vertexLight : TEXCOORD1, float displacementAlpha : TEXCOORD2) : SV_Target0 {
    float2 transformedUv = float2(dot(float3(uv,1),material.baseTextureTransform0.xyz),dot(float3(uv,1),material.baseTextureTransform1.xyz));
    float4 sampled = baseTexture.Sample(baseSampler, transformedUv);
    if (material.vertexTransition != 0) {
        float4 secondSample = secondBaseTexture.Sample(baseSampler, transformedUv);
        sampled = lerp(sampled, secondSample, saturate(displacementAlpha / 255.0));
    }
    if (sampled.a < material.alphaCutoff) discard;
    if (material.vertexLighting != 0) sampled.rgb *= vertexLight;
    sampled.a *= material.alpha;
    return sampled;
}
