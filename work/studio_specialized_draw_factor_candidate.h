#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>
// Finite native factor path: bound float to[1,16], round upward, convert.
// Nonfinite rejection is a candidate policy, not recovered native behavior.
inline uint32_t studioSpecializedDrawFiniteFactorCandidate(float factor){
 if(!std::isfinite(factor))throw std::invalid_argument("nonfinite specialized draw factor");
 const float bounded=factor<1.0f?1.0f:(factor>16.0f?16.0f:factor);
 return static_cast<uint32_t>(std::ceil(static_cast<double>(bounded)));
}
