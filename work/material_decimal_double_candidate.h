#pragma once
#include <array>
#include <cstdint>
struct MaterialDecimalDoubleCandidate{uint64_t bits=0;int status=0;};
inline MaterialDecimalDoubleCandidate materialDecimalDoubleCandidate(const std::array<uint8_t,12>&wide){
    std::array<uint16_t,6> original{};for(unsigned i=0;i<5;++i)original[i+1]=uint16_t(wide[2*i]|(unsigned(wide[2*i+1])<<8));
    const unsigned exponent=wide[10]|(unsigned(wide[11])<<8);
    const uint64_t sign=uint64_t(exponent>>15)<<63;const int initial=int(exponent&32767)-16383;
    if(initial==-16383){bool nonzero=false;for(auto w:original)nonzero|=w!=0;return {sign,nonzero?2:0};}
    auto round=[](std::array<uint16_t,6>&w){
        unsigned carry=0;
        if((w[2]&0x400)&&(w[0]||w[1]||(w[2]&0x3ff))){
            carry=0x800;for(unsigned i=2;i<6&&carry;++i){unsigned n=w[i]+carry;w[i]=uint16_t(n);carry=n>>16;}
        }
        w[0]=w[1]=0;w[2]&=0xfc00;return carry;
    };
    auto top=[](const std::array<uint16_t,6>&w){uint64_t v=0;for(unsigned i=2;i<6;++i)v|=uint64_t(w[i])<<(16*(i-2));return v;};
    auto value=original;const int e=initial+int(round(value));
    if(e< -1076)return {sign,2};
    if(e<=-1023){
        value=original;
        for(int shift=-1023-initial;shift>0;--shift){unsigned carry=0;for(unsigned i=6;i-->0;){unsigned next=value[i]&1;value[i]=uint16_t((value[i]>>1)|(carry<<15));carry=next;}}
        round(value);return {sign|(top(value)>>12),2};
    }
    if(e>=1024)return {sign|0x7ff0000000000000ull,1};
    return {sign|(uint64_t(e+1023)<<52)|((top(value)&0x7fffffffffffffffull)>>11),0};
}
