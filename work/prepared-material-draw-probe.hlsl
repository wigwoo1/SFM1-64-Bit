[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState sampling;
[[vk::binding(2,0)]] Texture2D<float4> maskTexture;
[[vk::binding(3,0)]] Texture2D<float4> secondTexture;
struct Request {[[vk::offset(112)]] float4 selected;};
[[vk::push_constant]] ConstantBuffer<Request> request;
float4 vertexMain(uint id:SV_VertexID):SV_Position {
    return float4(id==1?3:-1,id==2?3:-1,0,1);
}
float4 pixelMain():SV_Target0 {
    float2 uv=request.selected.yz;
    if(request.selected.x<0.5)return baseTexture.SampleLevel(sampling,uv,request.selected.w);
    if(request.selected.x<1.5)return maskTexture.SampleLevel(sampling,uv,request.selected.w);
    return secondTexture.SampleLevel(sampling,uv,request.selected.w);
}
