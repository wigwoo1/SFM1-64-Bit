#pragma once
#include <array>
#include <cstdint>
// 10012140: x87 store on first RGB component; MOVSS on the other two.
// Assumes masked floating-point exceptions, as in the execution oracle.
inline std::array<uint32_t,24> genericAmbientExpansionCandidate(const std::array<uint32_t,18>& rgb){
    std::array<uint32_t,24> result{};
    for(unsigned face=0;face<6;++face){
        auto first=rgb[face*3];
        if((first&0x7f800000u)==0x7f800000u&&(first&0x007fffffu))first|=0x00400000u;
        result[face*4]=first;
        result[face*4+1]=rgb[face*3+1];
        result[face*4+2]=rgb[face*3+2];
        result[face*4+3]=0x3f800000u;
    }
    return result;
}
