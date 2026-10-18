#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
// Recovered finite/default-environment ceiling semantics from 0x10A0F010.
// NaN runtime error/quieting paths remain unresolved: report unsupported.
inline std::optional<float> expressionCeiling(float value){
 uint32_t bits;std::memcpy(&bits,&value,4);unsigned exponent=(bits>>23)&255;
 if(exponent==255&&(bits&0x7fffffu))return std::nullopt;
 if(exponent>=150)return value;
 if(exponent<127){bits=(bits&0x80000000u)?0x80000000u:((bits&0x7fffffffu)?0x3f800000u:0);}
 else {uint32_t mask=(uint32_t(1)<<(150-exponent))-1;
  if(!(bits&0x80000000u)&&(bits&mask))bits+=mask;bits&=~mask;
 }
 std::memcpy(&value,&bits,4);return value;
}
