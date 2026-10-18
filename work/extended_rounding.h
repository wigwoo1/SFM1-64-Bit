#pragma once
#include <cstdint>
// Guard word and retained least significant bit at 0x10A1E458.
inline bool extendedScaleRoundUp(uint32_t lowWord){
 return (lowWord&0xffffu)>0x8000u||(lowWord&0x1ffffu)==0x18000u;
}
