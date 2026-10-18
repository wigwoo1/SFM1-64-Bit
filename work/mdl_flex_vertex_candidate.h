#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>
inline float finiteFlexHalfCandidate(uint16_t bits){
 unsigned exponent=(bits>>10)&31,mantissa=bits&1023;if(exponent==31)throw std::runtime_error("nonfinite flex delta");
 float value=exponent?std::ldexp(float(1024+mantissa),int(exponent)-25):std::ldexp(float(mantissa),-24);return bits&0x8000?-value:value;
}
struct MdlFlexVertexCandidate {uint16_t index;uint8_t speed,side;std::array<float,3>positionDelta,normalDelta;};
// Explicit unconverted normal disk record; caller chooses encoding and stride.
// Fixed-point runtime records and wrinkle records are separate formats.
inline MdlFlexVertexCandidate readMdlFlexHalfVertexCandidate(const std::vector<uint8_t>&bytes,size_t offset,size_t meshVertices){
 if(offset>bytes.size()||bytes.size()-offset<16)throw std::runtime_error("truncated flex vertex");MdlFlexVertexCandidate result{};std::memcpy(&result.index,bytes.data()+offset,2);if(result.index>=meshVertices)throw std::runtime_error("flex vertex outside mesh");result.speed=bytes[offset+2];result.side=bytes[offset+3];
 for(size_t i=0;i<6;++i){uint16_t bits;std::memcpy(&bits,bytes.data()+offset+4+i*2,2);(i<3?result.positionDelta[i]:result.normalDelta[i-3])=finiteFlexHalfCandidate(bits);}return result;
}
