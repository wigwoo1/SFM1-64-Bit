#pragma once
#include <array>
#include <cstdint>
#include <cstring>
// Upload tail1001694c..10016a78 only; caller computes local float4.
// Borrowed caches need at least30 registers. Ops implements direct/queue submit.
// Recovered comparison cache is published SECOND, after current-cache copy.
// Shared with100144a0 for valid caller-selected registers. Owner supplies
// index domain and sufficient cache capacity; native32 overflow not emulated.
template<class Ops>bool studioPixelConstantBlockCandidate(uint32_t index,const std::array<uint32_t,4>& local,Ops& ops){
 if(!std::memcmp(ops.comparisonCache()+index*4,local.data(),16))return false;
 ops.submitPixel(index,1,local.data());
 std::memcpy(ops.currentCache()+index*4,local.data(),16);
 std::memcpy(ops.comparisonCache()+index*4,local.data(),16);
 return true;
}
template<class Ops>bool studioPixelConstantTailCandidate(const std::array<uint32_t,4>& local,Ops& ops){
 return studioPixelConstantBlockCandidate(29,local,ops);
}
