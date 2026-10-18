#pragma once
#include "decimal_accumulator.h"
#include "decimal_scaler.h"
#include "extended_double.h"
#include <cstring>
// Input is the scanner's retained digit sequence and combined decimal
// exponent, not an arbitrary source string. Scanner retention is separate.
inline PackedDouble convertRetainedDecimal(std::string_view digits,int exponent,bool negative=false){
 if(digits.empty()||digits.size()>24)throw std::runtime_error("retained digit count outside verified domain");
 if(exponent < -5200||exponent > 5200)throw std::runtime_error("decimal scale outside verified domain");
 bool nonzero=false;
 for(char c:digits){if(c<'0'||c>'9')throw std::runtime_error("decimal digit required");nonzero|=c!='0';}
 if(!nonzero)return {negative?0x8000000000000000ull:0ull,0};
 const auto words=decimalExtendedInteger(digits);
 NormalizedExtended extended{{uint16_t(words[0]),uint16_t(words[0]>>16),uint16_t(words[1]),uint16_t(words[1]>>16),uint16_t(words[2])},uint16_t(words[2]>>16)};
 extended=scaleDecimalExtended(extended,exponent);
 // Parser applies input sign after magnitude scaling at 0x10A1E55E.
 extended.exponent=uint16_t(extended.exponent|(negative?0x8000u:0));
 return packExtendedDouble(extended);
}
inline double retainedDecimalDouble(PackedDouble packed){double value;std::memcpy(&value,&packed.bits,sizeof value);return value;}
