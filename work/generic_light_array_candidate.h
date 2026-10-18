#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
struct GenericLightArrayCandidate {
    int32_t count=0;
    std::array<uint32_t,88> words{}; // Four 88-byte records.
    uint8_t control=0;
    template<class Flush> void set(int32_t requested,int32_t configured,
        const std::array<uint32_t,88>& supplied,Flush flush) {
        const int32_t effective=(std::min)(requested,(std::min)(configured,int32_t(4)));
        flush(); // Original setter flushes even when effective count is unchanged.
        count=effective;
        if(effective>0)std::copy_n(supplied.begin(),size_t(effective)*22,words.begin());
        control&=0xe9;
    }
    template<class Flush> void clear(Flush flush) {
        if(count!=0){flush();control&=0xe9;count=0;}
    }
};
