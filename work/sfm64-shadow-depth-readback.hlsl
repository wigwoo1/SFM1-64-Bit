[[vk::binding(6,0)]] Texture2DArray<float> shadowDepth;
[[vk::binding(7,0)]] SamplerComparisonState shadowComparison;
[[vk::binding(5,0)]] RWStructuredBuffer<uint> results;
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint width,height,layers;shadowDepth.GetDimensions(width,height,layers);
 if(id.x>=width||id.y>=height||id.z>=layers)return;
 uint count,stride;results.GetDimensions(count,stride);
 uint index=((id.z*height+id.y)*width+id.x)*4;if(index+3>=count)return;
 float depth=shadowDepth.Load(int4(id,0));
 float3 uv=float3((float2(id.xy)+0.5)/float2(width,height),float(id.z));
 results[index]=asuint(depth);
 results[index+1]=asuint(shadowDepth.SampleCmpLevelZero(shadowComparison,uv,depth-1.0/1024.0));
 results[index+2]=asuint(shadowDepth.SampleCmpLevelZero(shadowComparison,uv,depth));
 results[index+3]=asuint(shadowDepth.SampleCmpLevelZero(shadowComparison,uv,depth+1.0/1024.0));
}
