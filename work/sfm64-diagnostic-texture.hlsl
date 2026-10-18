// Diagnostic Vulkan sampling shader; not Source material shader parity.
[[vk::binding(0, 0)]] Texture2D<float4> baseTexture;
[[vk::binding(1, 0)]] SamplerState baseSampler;
float4 main(float2 uv : TEXCOORD0) : SV_Target0 {
    return baseTexture.Sample(baseSampler, uv);
}
