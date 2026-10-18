#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <string>
#include <stdexcept>
// Recovered digit accumulator 0x10A320F8, positive nonzero retained digits.
inline std::array<uint32_t,3> decimalExtendedNumeric(std::string_view digits){
 if(digits.empty()||digits.size()>24)throw std::runtime_error("retained digit count outside verified domain");
 std::array<uint32_t,3> words{};
 for(unsigned char c:digits){if(c>10)throw std::runtime_error("native numeric digit required");
  uint64_t carry=c;
  for(auto&w:words){uint64_t v=uint64_t(w)*10+carry;w=uint32_t(v);carry=v>>32;}
  if(carry)throw std::runtime_error("digit accumulator overflow");
 }
 if(!(words[0]|words[1]|words[2]))throw std::runtime_error("zero normalization requires separate native path");
 uint16_t exponent=0x404e;
 while(!(words[2]&0x8000u)){
  words[2]=(words[2]<<1)|(words[1]>>31);
  words[1]=(words[1]<<1)|(words[0]>>31);words[0]<<=1;--exponent;
 }
 words[2]=(words[2]&0xffffu)|(uint32_t(exponent)<<16);return words;
}
inline std::array<uint32_t,3> decimalExtendedInteger(std::string_view digits){
 std::string numeric;for(char c:digits){if(c<'0'||c>'9')throw std::runtime_error("decimal digit required");numeric.push_back(char(c-'0'));}
 return decimalExtendedNumeric(numeric);
}
