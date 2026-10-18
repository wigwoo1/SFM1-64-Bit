#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
enum class StudioColorLookupRoundingCandidate {NearestEven,Down,Up,TowardZero};
inline float studioColorLookupCandidate(float input,const std::array<float,256>& table,StudioColorLookupRoundingCandidate mode){
 // Native NaN leads to an invalid FISTP index. Reject unsupported input in
 // this logical prototype; this is not a native exception-parity claim.
 if(std::isnan(input))throw std::invalid_argument("unverified color lookup NaN");
 if(input<0)return 0;
 uint32_t bits=0x3f733333u;float threshold;std::memcpy(&threshold,&bits,4);
 if(input>=threshold)return 1;
 const float scaled=input*255.0f;
 const double value=scaled,base=std::floor(value);
 unsigned index=0;
 switch(mode){
 case StudioColorLookupRoundingCandidate::Down:
 case StudioColorLookupRoundingCandidate::TowardZero:index=unsigned(base);break;
 case StudioColorLookupRoundingCandidate::Up:index=unsigned(std::ceil(value));break;
 case StudioColorLookupRoundingCandidate::NearestEven:
  index=unsigned(base);if(value-base>0.5||(value-base==0.5&&(index&1)))++index;break;
 }
 return table.at(index);
}
