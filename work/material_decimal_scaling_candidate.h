#pragma once
#include "material_decimal_digits_candidate.h"
#include "material_decimal_powers_candidate.h"
#include <string>
struct MaterialDecimalWideCandidate{std::array<uint16_t,5> m{};int e=0;};
inline MaterialDecimalWideCandidate materialDecimalWideDecodeCandidate(const std::array<uint8_t,12>&v){
    MaterialDecimalWideCandidate w;for(unsigned i=0;i<5;++i)w.m[i]=uint16_t(v[2*i]|(unsigned(v[2*i+1])<<8));
    w.e=v[10]|(unsigned(v[11])<<8);return w;
}
inline MaterialDecimalWideCandidate materialDecimalMultiplyCandidate(MaterialDecimalWideCandidate a,MaterialDecimalWideCandidate b){
    const MaterialDecimalWideCandidate infinity{{0,0,0,0,0x8000},32767};
    int total=a.e+b.e;
    if(a.e>=32767||b.e>=32767||total>0xbffd)return infinity;
    if(total<=0x3fbf)return {};
    auto zero=[](const auto&w){for(auto x:w.m)if(x)return false;return true;};
    if(!a.e){++total;if(zero(a))return {};}
    if(!b.e){++total;if(zero(b))return {};}
    std::array<uint16_t,6> p{};
    for(unsigned i=0;i<5;++i)for(unsigned j=0;j<5;++j)if(i+j>=4){
        uint64_t carry=uint32_t(a.m[i])*uint32_t(b.m[j]);
        for(unsigned k=i+j-4;k<6&&carry;++k){carry+=p[k];p[k]=uint16_t(carry);carry>>=16;}
    }
    int e=total-16382;
    while(e>0&&!(p[5]&0x8000)){
        unsigned carry=0;for(auto&x:p){unsigned n=(unsigned(x)<<1)|carry;carry=n>>16;x=uint16_t(n);}--e;
    }
    if(e<=0){
        --e;bool lost=false;
        while(e<0){lost=lost||bool(p[0]&1);unsigned carry=0;for(unsigned i=6;i-->0;){unsigned next=p[i]&1;p[i]=uint16_t((p[i]>>1)|(carry<<15));carry=next;}++e;}
        if(lost)p[0]|=1;
    }
    if(p[0]>0x8000||(p[0]==0x8000&&(p[1]&1))){
        unsigned carry=1;for(unsigned i=1;i<6&&carry;++i){unsigned n=p[i]+carry;p[i]=uint16_t(n);carry=n>>16;}
        if(carry){p[5]=0x8000;++e;}
    }
    if(e>=32767)return infinity;
    MaterialDecimalWideCandidate w;w.e=e;for(unsigned i=0;i<5;++i)w.m[i]=p[i+1];return w;
}
inline bool materialDecimalScaleRawCandidate(const std::array<uint8_t,12>&raw,int power,std::array<uint8_t,12>&out){
    MaterialDecimalWideCandidate value;
    if(power>5200)value={{0,0,0,0,0x8000},32767};
    else if(power>=-5200){
        value=materialDecimalWideDecodeCandidate(raw);
        if(power){
            value.m[0]=0;const auto&table=power<0?materialDecimalNegativePowersCandidate:materialDecimalPositivePowersCandidate;
            unsigned remaining=unsigned(power<0?-power:power),group=0;
            while(remaining){unsigned digit=remaining&7;remaining>>=3;
                if(digit)value=materialDecimalMultiplyCandidate(value,materialDecimalWideDecodeCandidate(table[group*7+digit-1]));++group;}
        }
    }
    std::array<uint8_t,12> result{};for(unsigned i=0;i<5;++i){result[2*i]=uint8_t(value.m[i]);result[2*i+1]=uint8_t(value.m[i]>>8);}
    result[10]=uint8_t(value.e);result[11]=uint8_t(value.e>>8);out=result;return true;
}
// Canonical decimal coefficient/scaling stage, not a general text parser.
inline bool materialDecimalScaleCandidate(std::string digits,int power,std::array<uint8_t,12>&out){
    std::array<uint8_t,12> raw{};if(!materialDecimalDigitsCandidate(digits,raw))return false;
    if(power< -10000||power>10000)return false;
    while(digits.back()=='0'){digits.pop_back();++power;}
    materialDecimalDigitsCandidate(digits,raw);return materialDecimalScaleRawCandidate(raw,power,out);
}
