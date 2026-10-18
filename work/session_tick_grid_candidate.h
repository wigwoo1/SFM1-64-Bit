#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>
// Units are serialized session ticks. Seconds/frame-rate parity is unverified.
inline std::vector<int32_t> sessionTickGridCandidate(int32_t start,int32_t step,size_t count){
 if(!count||count>10000||!step)throw std::runtime_error("invalid session tick grid");std::vector<int32_t>ticks;ticks.reserve(count);
 for(size_t frame=0;frame<count;++frame){int64_t value=int64_t(start)+int64_t(step)*int64_t(frame);if(value<INT32_MIN||value>INT32_MAX)throw std::runtime_error("session tick grid overflow");ticks.push_back(int32_t(value));}return ticks;
}
