#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>
struct JiggleParametersCandidate {uint32_t flags=0;std::array<float,29>values{};};
// SFM120-byte candidate prefix. Boing extensions and simulation unfinished.
inline JiggleParametersCandidate readJiggleParametersCandidate(const std::vector<uint8_t>&bytes,size_t offset){
 if(offset>bytes.size()||bytes.size()-offset<120)throw std::runtime_error("jiggle parameter span");JiggleParametersCandidate result;std::memcpy(&result.flags,bytes.data()+offset,4);if(result.flags&~uint32_t(127))throw std::runtime_error("unsupported jiggle flags or extension");if((result.flags&3)!=1&&(result.flags&3)!=2)throw std::runtime_error("jiggle mode");std::memcpy(result.values.data(),bytes.data()+offset+4,116);for(auto value:result.values)if(!std::isfinite(value))throw std::runtime_error("nonfinite jiggle parameter");if(!(result.values[0]>0))throw std::runtime_error("jiggle length");return result;
}
