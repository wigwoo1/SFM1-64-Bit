#pragma once
#include <array>
#include <cstdint>
// Native100036b0 distinct-ID count, normalized away from PE32 object offsets.
// Broader storage avoids original32-entry local-buffer overflow; equivalence
// outside constructor-valid input bounds has not been established.
inline uint32_t studioCpuIdCountCandidate(const uint8_t* ids,uint8_t count,uint8_t maskA,uint8_t maskB,uint8_t shift){
 std::array<bool,256> seen{};uint32_t result=0;const unsigned bits=shift&31u;
 for(unsigned i=0;i<count;++i){const uint8_t value=bits>=8?0:static_cast<uint8_t>((ids[i]&(maskA|maskB))>>bits);if(!seen[value]){seen[value]=true;++result;}}
 return result;
}
