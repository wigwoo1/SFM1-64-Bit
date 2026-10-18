[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
struct MaterialPush {[[vk::offset(96)]]float alpha;[[vk::offset(100)]]float alphaCutoff;};
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
float4 main(float2 uv:TEXCOORD0,float3 worldNormal:TEXCOORD1):SV_Target0{
 float opacity=baseTexture.Sample(baseSampler,uv).a;if(opacity<material.alphaCutoff)discard;
 float lengthSquared=dot(worldNormal,worldNormal);
 float3 normal=lengthSquared>1e-20?worldNormal*rsqrt(lengthSquared):float3(0,0,0);
 return float4(normal*.5+.5,opacity*material.alpha);
}
