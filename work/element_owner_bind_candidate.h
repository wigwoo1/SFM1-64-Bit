#pragma once
#include "element_owner_registration_candidate.h"
//10354b70: binding permits metadata-highbit reactivation, unlike resolve.
inline void bindElementOwnerCandidate(std::uint32_t handle,
    ElementOwnerPoolEntryCandidate* entries,std::uint32_t count,
    std::uint32_t& liveCount,ElementOwnerCounterCandidate* object) {
    if(handle==0xffffffffu)return;
    const auto index=handle&0xfffffu;
    if(index>=count)return;
    auto& entry=entries[index];
    if((entry.generationAndFlags&0x7fffffffu)!=((handle>>20)&0x7ffu))return;
    if(entry.generationAndFlags&0x80000000u) {
        ++liveCount;entry.generationAndFlags&=0x7fffffffu;
    }
    entry.object=object;
}
