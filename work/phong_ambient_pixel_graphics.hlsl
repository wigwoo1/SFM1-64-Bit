// Test-only graphics wrapper: actual texture sampling and float target output.
#include "phong_ambient_pixel_candidate.hlsli"
[[vk::binding(0,0)]] StructuredBuffer<PhongAmbientPixelRecord> records;
[[vk::binding(1,0)]] Texture2D<float4> baseTexture;
[[vk::binding(2,0)]] Texture2D<float4> normalTexture;
[[vk::binding(3,0)]] SamplerState pointClamp;
struct Interpolants {
    float4 clip:SV_Position;
    [[vk::location(0)]] float2 uv:TEXCOORD0;
    [[vk::location(1)]] float3 tangent:TEXCOORD1;
    [[vk::location(2)]] float3 binormal:TEXCOORD2;
    [[vk::location(3)]] float3 normal:TEXCOORD3;
    [[vk::location(4)]] float3 position:TEXCOORD4;
};
Interpolants vertexMain(uint id:SV_VertexID){
    const float2 corners[6]={float2(-1,-1),float2(-1,1),float2(1,-1),float2(1,-1),float2(-1,1),float2(1,1)};
    PhongAmbientPixelRecord r=records[0];Interpolants o;
    o.clip=float4(corners[id],.5,1);o.uv=float2(.5,.5);
    o.tangent=r.tangent.xyz;o.binormal=r.binormal.xyz;o.normal=r.normal.xyz;o.position=r.position.xyz;
    return o;
}
float4 pixelMain(Interpolants i):SV_Target0{
    PhongAmbientPixelRecord r=records[0];
    r.base=baseTexture.Sample(pointClamp,i.uv);r.normalSample=normalTexture.Sample(pointClamp,i.uv);
    r.tangent.xyz=i.tangent;r.binormal.xyz=i.binormal;r.normal.xyz=i.normal;r.position.xyz=i.position;
    return phongAmbientPixelCandidate(r);
}
