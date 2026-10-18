#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
// Recovered101941d0 table formula only; not the full100d5020 generator.
// Fixed installed exponent is float32 2.2 widened to double.
inline std::array<float,256> studioColorTableCandidate(){
 uint32_t bits=0x3b808081;float normalize;std::memcpy(&normalize,&bits,4);
 std::array<float,256> table{};
 for(unsigned i=0;i<256;++i){
  const float normalized=float(i)*normalize;
  table[i]=float(std::pow(double(normalized),double(2.2f)));
 }
 return table;
}
