[[vk::binding(0,0)]] Texture2D<float4> sourceTexture;
// No input conversion for the floating-point scene. The attachment view encodes
// RGB once; alpha remains linear. No tone-map or exposure policy is inferred.
float4 main(float4 position : SV_Position) : SV_Target0 {
    return sourceTexture.Load(int3(int2(position.xy), 0));
}
