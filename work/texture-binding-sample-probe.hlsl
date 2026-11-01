[[vk::binding(0,0)]] Texture2D<float4> image;
[[vk::binding(1,0)]] SamplerState sampling;
[[vk::binding(0,1)]] RWStructuredBuffer<float4> output;
struct Request {uint index;float u;float v;float lod;};
[[vk::push_constant]] ConstantBuffer<Request> request;
[numthreads(1,1,1)]
void main(){output[request.index]=image.SampleLevel(sampling,float2(request.u,request.v),request.lod);}
