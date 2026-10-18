#pragma once
#include <cstdint>
// Recovered element IDs0..49; out-of-range yields zero.
inline uint32_t studioMaterialVertexElementSizeCandidate(uint32_t element,bool compressed){
 static constexpr uint8_t regular[50]={12,16,12,16,4,4,12,12,4,4,4,8,12,16,4,8,12,16,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,12,12,12,12,12,12,12,12,16,16,16,16,16,16,16,16};
 static constexpr uint8_t packed[50]={12,16,4,16,4,4,12,12,4,4,4,4,12,16,4,8,12,0,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,12,12,12,12,12,12,12,12,16,16,16,16,16,16,16,16};
 return element<50?(compressed?packed[element]:regular[element]):0;
}
