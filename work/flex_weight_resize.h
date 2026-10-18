#pragma once
#include <vector>
#include <cstdint>
// Native105C2D10: preserve prefix, append float +0, or remove from the tail.
inline void recoveredFlexWeightResizeCandidate(std::vector<uint32_t>& weights,uint32_t count){weights.resize(count,0u);}
