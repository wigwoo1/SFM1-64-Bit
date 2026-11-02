[[vk::binding(0,0)]] Texture2D<float4> image;
[[vk::binding(1,0)]] SamplerState sampling;
struct Request {uint index;float u;float v;float unused;float2 dx;float2 dy;};
[[vk::push_constant]] ConstantBuffer<Request> request;
float4 vertexMain(uint id:SV_VertexID):SV_Position{return float4(id==1?3:-1,id==2?3:-1,0,1);}
float4 pixelMain():SV_Target0{return image.SampleGrad(sampling,float2(request.u,request.v),request.dx,request.dy);}
