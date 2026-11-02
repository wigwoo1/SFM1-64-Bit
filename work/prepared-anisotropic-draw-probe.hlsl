[[vk::binding(0,0)]] Texture2D<float4> baseTexture;
[[vk::binding(1,0)]] SamplerState sampling;
[[vk::binding(2,0)]] Texture2D<float4> maskTexture;
[[vk::binding(3,0)]] Texture2D<float4> secondTexture;
[[vk::binding(4,0)]] SamplerState maskSampling;
[[vk::binding(5,0)]] SamplerState secondSampling;
struct Request {[[vk::offset(112)]] float4 selected;};
[[vk::push_constant]] ConstantBuffer<Request> request;
float4 vertexMain(uint id:SV_VertexID):SV_Position {
    return float4(id==1?3:-1,id==2?3:-1,0,1);
}
float4 pixelMain():SV_Target0 {
    float2 uv=request.selected.yz;
    const float2 dx[8]={float2(1./64,0),float2(.125,0),float2(.5,0),float2(0,.5),float2(.5,.5),float2(.25,.125),float2(1,0),float2(.125,.125)};
    const float2 dy[8]={float2(0,1./64),float2(0,.125),float2(0,.03125),float2(.03125,0),float2(-.03125,.03125),float2(-.015625,.03125),float2(0,.0625),float2(-.125,.125)};
    uint pattern=min(7u,(uint)request.selected.w);
    if(request.selected.x<0.5)return baseTexture.SampleGrad(sampling,uv,dx[pattern],dy[pattern]);
    if(request.selected.x<1.5)return maskTexture.SampleGrad(maskSampling,uv,dx[pattern],dy[pattern]);
    if(request.selected.x<2.5)return secondTexture.SampleGrad(secondSampling,uv,dx[pattern],dy[pattern]);
    return baseTexture.SampleGrad(sampling,uv,dx[pattern],dy[pattern])*secondTexture.SampleGrad(secondSampling,uv,dx[pattern],dy[pattern])+0.1*maskTexture.SampleGrad(maskSampling,uv,dx[pattern],dy[pattern]);
}
