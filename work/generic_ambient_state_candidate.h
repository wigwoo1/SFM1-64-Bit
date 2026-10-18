#pragma once
#include <array>
#include <cstdint>
struct GenericAmbientStateCandidate {
    std::array<uint32_t,24> source{};
    std::array<uint32_t,18> packed{};
    uint8_t control=0;
    bool set(const std::array<uint32_t,24>& input) {
        if(source==input)return false;
        source=input;
        for(unsigned group=0;group<6;++group)for(unsigned channel=0;channel<3;++channel){
            uint32_t bits=input[group*4+channel];
            // Masked x87 FLD/FSTP quiets signaling NaNs; retain sign/payload.
            if((bits&0x7f800000u)==0x7f800000u && (bits&0x007fffffu)!=0)bits|=0x00400000u;
            packed[group*3+channel]=bits;
        }
        control|=1;return true;
    }
};
