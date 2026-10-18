#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
// Logical1000fc90 policy. Ops supplies live services/scales and recovered lookup.
// SSE truncation retains native indefinite-result behavior for out-of-range
// negative channels; FP exception/control-state parity remains unverified.
template<class Ops>uint32_t studioShaderColorConversionCandidate(std::array<uint8_t,3> rgb,uint32_t flag,Ops& ops){
 bool convert=ops.flag5aa()&1;
 if(!convert&&uint8_t(flag))convert=(ops.flag5a9()&128)||ops.mode()==2;
 const bool scale=ops.mode()==1;
 uint32_t bits=0x3b808081;float normalize;std::memcpy(&normalize,&bits,4);
 std::array<float,3> values{float(rgb[0])*normalize,float(rgb[1])*normalize,float(rgb[2])*normalize};
 if(convert){for(float& value:values)value=ops.lookup(value);}
 if(scale){const float factor=convert?ops.convertedScale():ops.rawScale();for(float& value:values)value*=factor;}
 std::array<uint8_t,3> packed{};
 for(unsigned i=0;i<3;++i){
  // Native COMISS followed by JA leaves values only when strictly below1;
  // unordered values take the1 branch.
  if(!(values[i]<1.f))values[i]=1.f;
  packed[i]=uint8_t(_mm_cvtt_ss2si(_mm_set_ss(values[i]*255.f)));
 }
 return 0xff000000u|(uint32_t(packed[0])<<16)|(uint32_t(packed[1])<<8)|packed[2];
}
