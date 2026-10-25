#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Decimal integer stage only. Scaling and final double rounding are separate.
// The installed parser calls this stage with 1..24 digits and a nonzero value.
// Produce a normalized 80-bit significand and biased 16-bit exponent, explicitly
// encoded little-endian; no host floating point or original runtime is required.
inline bool materialDecimalDigitsCandidate(std::string_view digits,std::array<uint8_t,12>&out){
    if(digits.empty()||digits.size()>24)return false;
    std::array<uint32_t,3> words{};
    for(char ch:digits){
        if(ch<'0'||ch>'9')return false;
        uint64_t carry=uint32_t(ch-'0');
        for(auto&word:words){const uint64_t n=uint64_t(word)*10+carry;word=uint32_t(n);carry=n>>32;}
    }
    if(!(words[0]|words[1]|words[2]))return false;
    uint16_t exponent=0x404e;
    while(!(words[2]&0x8000)){
        words[2]=(words[2]<<1)|(words[1]>>31);
        words[1]=(words[1]<<1)|(words[0]>>31);words[0]<<=1;--exponent;
    }
    std::array<uint8_t,12> result{};
    for(unsigned i=0;i<10;++i)result[i]=uint8_t(words[i/4]>>((i%4)*8));
    result[10]=uint8_t(exponent);result[11]=uint8_t(exponent>>8);out=result;return true;
}
