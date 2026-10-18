#pragma once
#include <cstdint>
#include <cstring>
// Logical10006d20: signed divisions truncate toward0, subtraction wraps32.
inline uint32_t studioMeshPrimitiveCountCandidate(uint32_t type,uint32_t vertices,uint32_t indices){
 int32_t signedIndices;std::memcpy(&signedIndices,&indices,4);
 switch(type){case 1:return vertices;case 2:return uint32_t(signedIndices/2);case 4:return uint32_t(signedIndices/3);case 5:return indices-uint32_t(2);case 6:return uint32_t(signedIndices/4);default:return 0;}
}
