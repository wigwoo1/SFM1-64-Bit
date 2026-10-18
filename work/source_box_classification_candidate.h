#pragma once
#include <array>
#include <cstdint>
// Exact recovered runtime predicate, including the zero-side equality case.
// This does not certify safe box construction or validate plane geometry.
inline bool sourceBoxClassificationCandidate(uint16_t count,const std::array<uint8_t,6>&types){
 unsigned axial=0;if(count==6)for(uint8_t type:types){if(type>2)break;++axial;}return axial==count;
}
