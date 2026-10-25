#pragma once
#include <array>
#include <cstdint>
#include <cstring>
// Installed VS20 static0/dynamic5,7: D3D SHORT2 (not SHORT2N), then ADD1,
// MUL(1/32768). Every bit pattern is representable; no implicit clamp.
inline std::array<float,2>genericCompressedWeightInputCandidate(const std::array<uint8_t,4>&packed){
    std::array<int16_t,2>signedValues{};std::memcpy(signedValues.data(),packed.data(),4);
    std::array<float,2>result{};for(unsigned i=0;i<2;++i){const float biased=float(signedValues[i])+1.0f;result[i]=biased*(1.0f/32768.0f);}return result;
}
