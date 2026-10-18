#pragma once
#include <cstdint>
#include <limits>
template<class Adapter>
const char* modelTitleStringHandleCandidate(std::uint32_t handle,Adapter& a){
    if(handle==0xffffffffu)return nullptr;
    const auto index=handle&0xfffffu;
    if(index>=a.slotCount())return nullptr;
    auto& slot=a.slot(index);
    const auto generation=(handle>>20)&0x7ffu;
    if(((generation^slot.word)&0x7fffffffu)!=0 || (slot.word&0x80000000u))return nullptr;
    if(!slot.payload)return nullptr;
    const auto symbol=a.payloadSymbol(slot.payload);
    if(symbol==std::numeric_limits<std::uintptr_t>::max())return a.emptyString();
    return reinterpret_cast<const char*>(symbol);
}
