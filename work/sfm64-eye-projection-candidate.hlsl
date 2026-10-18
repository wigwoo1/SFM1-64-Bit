// Candidate consumes explicit world-space iris projection vectors.
// Native vector generation, lighting and eye composition are unfinished.
struct EyeProjectionRows {
 float4 camera0; float4 camera1; float4 camera2; float4 camera3;
 float4 irisU; float4 irisV;
};
[[vk::push_constant]] ConstantBuffer<EyeProjectionRows> projection;
struct Input {[[vk::location(0)]] float3 position:POSITION;[[vk::location(2)]] float2 uv:TEXCOORD0;};
struct Output {float4 position:SV_Position;float2 baseUV:TEXCOORD0;float2 irisUV:TEXCOORD1;};
Output main(Input input){
 Output output;float4 world=float4(input.position,1.0);
 output.position=float4(dot(projection.camera0,world),dot(projection.camera1,world),dot(projection.camera2,world),dot(projection.camera3,world));
 output.baseUV=input.uv;output.irisUV=float2(dot(projection.irisU,world),dot(projection.irisV,world));return output;
}
