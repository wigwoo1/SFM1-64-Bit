#pragma once
#include <cmath>
#include <cfenv>
#include <cstdint>
#include <cstring>
#include <stdexcept>
struct ClientExpNormalTablesCandidate { double small[16], middle[32], large[32], reduction, correction; };
inline float clientExpNormalCandidate(float input,const ClientExpNormalTablesCandidate& table){
 if(!std::isfinite(input)||input < -10.0f||input > -0.00001f)
  throw std::runtime_error("input outside recovered exponential candidate range");
 if(std::fegetround()!=FE_TONEAREST)throw std::runtime_error("exponential candidate requires nearest rounding");
 const float scaled=input*23637.115234375f;
 const int32_t n=static_cast<int32_t>(std::nearbyint(scaled));
 const uint32_t u=static_cast<uint32_t>(n);
 const double first=table.small[u&15u];
 const double residual=table.reduction*double(input)-double(n);
 const double correction=(table.correction*first)*residual;
 const double product=table.middle[(u>>4)&31u]*table.large[(u>>9)&31u];
 uint64_t bits;std::memcpy(&bits,&product,sizeof(bits));
 // Floor division replaces the native arithmetic shift portably.
 const int32_t exponent=n>=0?n/16384:-static_cast<int32_t>((-int64_t(n)+16383)/16384);
 bits+=static_cast<uint64_t>(int64_t(exponent))* (uint64_t(1)<<52);
 double factor;std::memcpy(&factor,&bits,sizeof(factor));
 return static_cast<float>((first+correction)*factor);
}
