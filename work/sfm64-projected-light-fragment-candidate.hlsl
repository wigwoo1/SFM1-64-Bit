// Direct diffuse light contribution only. Source shader parity and shadow/
// volumetric/uberlight passes are separate unfinished requirements.
[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState baseSampler;
#include "sfm64-projected-light-layout.hlsli"
[[vk::binding(3,0)]] StructuredBuffer<LightInput> lights;
[[vk::binding(4,0)]] Texture2D<float4> goboTexture;
struct MaterialPush {[[vk::offset(96)]]float alpha;[[vk::offset(100)]]float alphaCutoff;[[vk::offset(104)]]uint lightIndex;[[vk::offset(108)]]float diagnosticGain;};
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
float4 main(float2 uv:TEXCOORD0,float3 normal:TEXCOORD1,float3 position:TEXCOORD2):SV_Target0{
 uint count,stride;lights.GetDimensions(count,stride);if(material.lightIndex>=count)discard;
 LightInput light=lights[material.lightIndex];float4 p=float4(position,1);
 float4 projected=float4(dot(light.row0,p),dot(light.row1,p),dot(light.row2,p),dot(light.row3,p));
 if(projected.w<=0)discard;float3 coordinate=projected.xyz/projected.w;
 if(any(coordinate<0)||any(coordinate>1))discard;
 float4 base=baseTexture.Sample(baseSampler,uv);if(base.a<material.alphaCutoff)discard;
 float3 delta=light.originNear.xyz-position;float d2=dot(delta,delta),n2=dot(normal,normal);
 if(d2<=1e-20||n2<=1e-20)discard;float distance=sqrt(d2);
 float lambert=max(dot(normal*rsqrt(n2),delta*rsqrt(d2)),0);
 // SDK flashlight coefficients are summed reciprocal-distance contributions.
 float attenuation=saturate(dot(light.attenuationAmbient.xyz,float3(1,1/distance,1/d2)));
 float endFalloff=saturate((light.colorFar.w-distance)/(light.colorFar.w*.4f));
 float3 gobo=goboTexture.Sample(baseSampler,coordinate.xy).rgb;
 return float4(base.rgb*light.colorFar.rgb*gobo*(lambert*attenuation*endFalloff)*material.diagnosticGain,base.a*material.alpha);
}
