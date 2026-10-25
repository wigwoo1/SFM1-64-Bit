#include "sfm64-generic-ambient-candidate.hlsli"
struct AmbientRecord {float4 worldNormal;float4 cube[6];};
[[vk::binding(0,0)]] StructuredBuffer<AmbientRecord> records;
[[vk::binding(1,0)]] RWStructuredBuffer<float4> irradiance;
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint count,stride;records.GetDimensions(count,stride);if(id.x>=count)return;
    AmbientRecord record=records[id.x];
    // Domain validated on CPU. Actual native NRM/GPU precision is separate.
    float3 normal=normalize(record.worldNormal.xyz);
    irradiance[id.x]=float4(sfm64GenericAmbientCandidate(normal,record.cube),1);
}
