#include "sfm64-projected-light-layout.hlsli"
[[vk::binding(3,0)]] StructuredBuffer<LightInput> lights;
[[vk::binding(5,0)]] RWStructuredBuffer<uint4> results;
[numthreads(1,1,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint count,stride;lights.GetDimensions(count,stride);if(id.x>=count)return;
 LightInput l=lights[id.x];uint base=id.x*7;
 results[base]=asuint(l.row0);results[base+1]=asuint(l.row1);results[base+2]=asuint(l.row2);results[base+3]=asuint(l.row3);
 results[base+4]=asuint(l.originNear);results[base+5]=asuint(l.colorFar);results[base+6]=asuint(l.attenuationAmbient);
}
