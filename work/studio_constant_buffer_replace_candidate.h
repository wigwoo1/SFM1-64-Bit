#pragma once
#include <cstdint>
#include <limits>
// Recovered low-level replacement policy; owned failure recovery is separate.
inline uint32_t studioConstantBufferAllocationBytesCandidate(uint32_t count,uint32_t stride){
 const uint64_t product=uint64_t{count}*stride;
 return product>std::numeric_limits<uint32_t>::max()?0xffffffffu:static_cast<uint32_t>(product);
}
template<class Pointer,class Services>
void studioConstantBufferReplaceCandidate(Pointer& slot,unsigned countId,uint32_t stride,Services& services){
 if(slot)services.free(slot);
 const uint32_t count=services.count(countId);
 slot=services.allocate(studioConstantBufferAllocationBytesCandidate(count,stride));
}
