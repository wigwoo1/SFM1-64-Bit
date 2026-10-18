// Isolated reconstruction of installed normal UnlitTwoTexture RGB/UV contract.
// No claim of complete fog, cloak, proxy, lighting or material compatibility.
struct Parameters { float4 uv0x; float4 uv0y; float4 uv1x; float4 uv1y; float4 control; };
[[vk::push_constant]] ConstantBuffer<Parameters> parameters;
struct Varying { float4 position:SV_Position; float2 uv0:TEXCOORD0; float2 uv1:TEXCOORD1; };
Varying vertexMain(uint id:SV_VertexID) {
    float2 positions[3]={float2(-1,-1),float2(-1,3),float2(3,-1)};
    float4 uv=float4(.25,.25,0,1);
    Varying output;
    output.position=float4(positions[id],parameters.control.x,1);
    output.uv0=float2(dot(uv,parameters.uv0x),dot(uv,parameters.uv0y));
    output.uv1=float2(dot(uv,parameters.uv1x),dot(uv,parameters.uv1y));
    return output;
}
// Pass raw UVs to the production fragment shader. The shared recorder supplies
// its independent material transforms at the production offsets.
Varying rawVertexMain(uint id:SV_VertexID) {
    Varying output=vertexMain(id);
    output.uv0=float2(.25,.25);
    output.uv1=0;
    return output;
}
[[vk::binding(0,0)]] Texture2D<float4> texture0;
[[vk::binding(1,0)]] Texture2D<float4> texture1;
[[vk::binding(2,0)]] SamplerState pointSampler;
float4 fragmentMain(Varying input):SV_Target0 {
    return float4(texture0.Sample(pointSampler,input.uv0).rgb*texture1.Sample(pointSampler,input.uv1).rgb,1);
}
