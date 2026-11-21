[[vk::binding(0, 0)]] Texture2D<float4> baseTexture;
[[vk::binding(1, 0)]] SamplerState baseSampler;
struct Coverage {
    [[vk::offset(64)]] float4 uvRow0;
    [[vk::offset(80)]] float4 uvRow1;
    [[vk::offset(96)]] float alphaCutoff;
};
[[vk::push_constant]] ConstantBuffer<Coverage> coverage;

float4 main(float4 position : SV_Position,
            [[vk::location(0)]] float3 worldNormal : TEXCOORD0,
            [[vk::location(1)]] float2 uv : TEXCOORD1) : SV_Target0 {
    if (coverage.alphaCutoff > 0) {
        float3 sourceUv = float3(uv, 1);
        float2 transformed = float2(dot(sourceUv, coverage.uvRow0.xyz),
                                    dot(sourceUv, coverage.uvRow1.xyz));
        if (baseTexture.Sample(baseSampler, transformed).a < coverage.alphaCutoff)
            discard;
    }
    float normalSquared = dot(worldNormal, worldNormal);
    if (normalSquared == 0)
        discard;
    // The viewport's perspective projection puts linear view depth in clip.w.
    return float4(worldNormal * rsqrt(normalSquared), rcp(position.w));
}
