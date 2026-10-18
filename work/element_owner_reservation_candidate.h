#pragma once
#include "element_owner_registration_candidate.h"
//1035abe0 normal ring invariants. Adapter supplies ring/pop/new-entry storage.
// highBit explicitly carries original pool address bit31; x64 pointer bits are
// not silently converted into a new semantic handle flag.
template<class Adapter>
inline std::uint32_t reserveElementOwnerCandidate(std::uint32_t highBit,Adapter& a) {
    const auto index=a.hasRecycled()?a.popRecycled():a.appendEntry();
    auto& entry=a.entry(index);
    entry.generationAndFlags&=0x7fffffffu;entry.object=nullptr;
    a.incrementLiveCount();
    auto handle=((entry.generationAndFlags&0x7ffu)<<20)|
        (highBit&0x80000000u)|(index&0xfffffu);
    if(index!=(handle&0xfffffu))handle=0xffffffffu;
    return handle;
}
