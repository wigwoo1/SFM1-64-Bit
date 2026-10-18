#pragma once
#include <array>
#include <cstdint>
// Default x87 path assumes masked FP exceptions. Alternate integer path retains
// signaling NaN bits. This returns the first three rows of the full transpose.
inline std::array<uint32_t,12> studioShaderInstanceMatrixCandidate(const std::array<uint32_t,16>& source,bool integerPath=false){
 std::array<uint32_t,12> result{};
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){
  auto bits=source[c*4+r];unsigned dst=r*4+c;
  bool x87=dst==1||dst==4||dst==8||dst==9;
  if(!integerPath&&x87&&(bits&0x7f800000u)==0x7f800000u&&(bits&0x007fffffu))bits|=0x00400000u;
  result[dst]=bits;
 }
 return result;
}
