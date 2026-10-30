#include "phong_ambient_pixel_candidate.hlsli"
[[vk::binding(0,0)]] StructuredBuffer<PhongAmbientPixelRecord> records;
[[vk::binding(1,0)]] RWStructuredBuffer<float4> colors;
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint count,stride;records.GetDimensions(count,stride);if(id.x>=count)return;
    colors[id.x]=phongAmbientPixelCandidate(records[id.x]);
}
