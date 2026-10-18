#pragma once
#include "mdl_flex_vertex_candidate.h"
inline float readMdlFlexFixedScaleCandidate(const std::vector<uint8_t>&bytes){
 if(bytes.size()<396)throw std::runtime_error("truncated flex scale header");uint32_t flags;std::memcpy(&flags,bytes.data()+152,4);float scale=1.0f/4096.0f;if(flags&0x200000)std::memcpy(&scale,bytes.data()+392,4);if(!std::isfinite(scale)||scale<=0)throw std::runtime_error("invalid flex fixed scale");return scale;
}
// SDK conversion candidate. Reject overflow instead of narrowing out-of-range
// floating values; native conversion call site still needs verification.
inline int16_t quantizeFlexDeltaCandidate(float value,float scale){
 if(!std::isfinite(value)||!std::isfinite(scale)||scale<=0)throw std::runtime_error("invalid flex quantization input");float divided=value/scale;if(!std::isfinite(divided))throw std::runtime_error("flex quantization overflow");float truncated=std::trunc(divided);if(truncated< -32768||truncated>32767)throw std::runtime_error("flex quantization overflow");return int16_t(truncated);
}
