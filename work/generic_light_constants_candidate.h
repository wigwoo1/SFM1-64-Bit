#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
// Scoped bounded equivalent of 10012210. Destination tails remain unchanged.
// Original routine does not enforce this owned four-light capacity bound.
inline void genericLightConstantsCandidate(std::array<uint32_t,137>& destination,
    int32_t count,const std::array<uint32_t,88>& lights){
    if(count>4)throw std::invalid_argument("light constant capacity");
    auto store=[](uint32_t bits){
        if((bits&0x7f800000u)==0x7f800000u&&(bits&0x007fffffu))bits|=0x00400000u;
        return bits;
    };
    destination[24]=static_cast<uint32_t>(count);
    for(int32_t light=0;light<count;++light){
        const auto* s=lights.data()+light*22;auto* d=destination.data()+49+light*20;
        d[0]=store(s[1]);d[1]=s[2];d[2]=s[3];d[3]=s[0]==2?0x3f800000u:0;
        d[4]=store(s[7]);d[5]=s[8];d[6]=s[9];d[7]=s[0]==3?0x3f800000u:0;
        d[8]=store(s[4]);d[9]=s[5];d[10]=s[6];d[11]=0x3f800000u;
        if(s[0]==3){d[12]=store(s[11]);d[13]=s[17];d[14]=s[18];d[15]=s[19];}
        else{d[12]=0;d[13]=d[14]=d[15]=0x3f800000u;}
        d[16]=store(s[12]);d[17]=s[13];d[18]=s[14];d[19]=0;
    }
    destination[129]=static_cast<uint32_t>(count);
    destination[130]=destination[132]=0;destination[131]=1;
    for(unsigned n=0;n<4;++n)destination[133+n]=count>static_cast<int32_t>(n);
}
