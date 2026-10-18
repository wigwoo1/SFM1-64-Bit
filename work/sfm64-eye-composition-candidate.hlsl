// Candidate unlit eye composition; Source lighting, glint, dilation and
// native composition parity remain unfinished.
[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
[[vk::binding(0,1)]] Texture2D<float4> irisTexture;
[[vk::binding(1,1)]] SamplerState irisSampler;
float4 main(float2 baseUV:TEXCOORD0,float2 irisUV:TEXCOORD1):SV_Target0 {
 float4 base=baseTexture.Sample(baseSampler,baseUV);
 float4 iris=irisTexture.Sample(irisSampler,irisUV);
 return float4(lerp(base.rgb,iris.rgb,iris.a),base.a);
}
