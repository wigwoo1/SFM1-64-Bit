#pragma once
#include <cstdint>
inline unsigned studioMultisampleSelectorCandidate(int32_t request){
 const auto value=static_cast<uint32_t>(request);
 return value-2u<=14u?value:0u;
}
