#pragma once
#include "element_owner_registration_candidate.h"
template<class Enqueue>
inline void releaseElementOwnerCandidate(std::uint32_t handle,
    ElementOwnerPoolEntryCandidate* entries,std::uint32_t count,
    std::uint32_t& liveCount,Enqueue&& enqueue) {
    const auto index=handle&0xfffffu;
    if(index>=count)return;
    auto& entry=entries[index];
    const auto old=entry.generationAndFlags;
    auto updated=(old&0x80000000u)|((old+1u)&0x7fffffffu);
    if(!(updated&0x80000000u)) {updated|=0x80000000u;--liveCount;}
    entry.generationAndFlags=updated;entry.object=nullptr;
    if((updated&0x7fffffffu)<0x7ffu)enqueue(index);
}
