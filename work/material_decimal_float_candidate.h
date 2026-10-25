#pragma once
#include <array>
#include <cstdint>
// Installed formatted %f converts the wide intermediate directly to binary32.
// It uses precision24, exponent range -127..127 and the legacy guard/sticky rule.
inline uint32_t materialDecimalFloatCandidate(const std::array<uint8_t,12>&wide){
    std::array<uint16_t,6>original{};for(unsigned i=0;i<5;++i)original[i+1]=uint16_t(wide[2*i]|(unsigned(wide[2*i+1])<<8));
    const unsigned exponent=wide[10]|(unsigned(wide[11])<<8);const uint32_t sign=uint32_t(exponent>>15)<<31;
    const int initial=int(exponent&32767)-16383;if(initial==-16383)return sign;
    auto round=[](std::array<uint16_t,6>&w){
        unsigned carry=0;
        if((w[4]&0x80)&&(w[0]||w[1]||w[2]||w[3]||(w[4]&0x7f))){
            carry=0x100;for(unsigned i=4;i<6&&carry;++i){unsigned n=w[i]+carry;w[i]=uint16_t(n);carry=n>>16;}
        }
        w[0]=w[1]=w[2]=w[3]=0;w[4]&=0xff80;return carry;
    };
    auto top=[](const auto&w){return uint32_t(w[4])|(uint32_t(w[5])<<16);};
    auto value=original;const int e=initial+int(round(value));
    if(e< -151)return sign;
    if(e<=-127){
        value=original;for(int shift=-127-initial;shift>0;--shift){unsigned carry=0;for(unsigned i=6;i-->0;){unsigned next=value[i]&1;value[i]=uint16_t((value[i]>>1)|(carry<<15));carry=next;}}
        round(value);return sign|(top(value)>>9);
    }
    if(e>=128)return sign|0x7f800000u;
    return sign|(uint32_t(e+127)<<23)|((top(value)&0x7fffffffu)>>8);
}
