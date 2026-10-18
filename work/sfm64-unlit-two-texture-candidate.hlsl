// Normal uncloaked subset with white modulation and unit light scale only.
// sRGB sampling/output and blend/depth masks
// belong to the image views/render target/pipeline, not ad-hoc gamma here.
[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
[[vk::binding(3,0)]] Texture2D<float4> secondTexture;
struct MaterialPush {
    [[vk::offset(96)]] float4 header;
    [[vk::offset(112)]] float4 baseRow0;
    [[vk::offset(128)]] float4 baseRow1;
    [[vk::offset(144)]] float4 secondRow0;
    [[vk::offset(160)]] float4 secondRow1;
};
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
float4 main(float2 uv:TEXCOORD0):SV_Target0 {
    float3 source=float3(uv,1);
    float2 first=float2(dot(source,material.baseRow0.xyz),dot(source,material.baseRow1.xyz));
    float2 second=float2(dot(source,material.secondRow0.xyz),dot(source,material.secondRow1.xyz));
    return float4(baseTexture.Sample(baseSampler,first).rgb*secondTexture.Sample(baseSampler,second).rgb,1);
}
