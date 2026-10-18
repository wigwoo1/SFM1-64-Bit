#pragma once
#include <array>
#include <cstdint>
//1035a2e0. Adapter captures initial table storage; resolveOwner must use current
// manager pool. Symbols/normalization/storage lifetime remain explicit bindings.
template<class Adapter>
std::uint32_t lookupElementOwnerCandidate(const char* name,Adapter& adapter) {
    std::array<char,0x104> normalized;
    adapter.normalize(normalized.data(),normalized.size(),name);
    const auto symbol=adapter.findSymbol(normalized.data());
    if (symbol==0xffffffffu) return 0xffffffffu;
    const std::int32_t count=adapter.poolCount();
    if (count<=0) return 0xffffffffu;
    auto* initialTable=adapter.poolTable();
    for (std::int32_t i=0;i<count;++i) {
        const auto& entry=initialTable[i];
        if (!entry.object) continue;
        const auto index=static_cast<std::uint32_t>(i);
        if (index!=(index&0xfffffu)) continue;
        const auto handle=((entry.generationAndFlags&0x7ffu)<<20)|index;
        if (handle==0xffffffffu) continue;
        auto* owner=adapter.resolveOwner(handle);
        if (owner && adapter.nameSymbol(owner)==symbol) return handle;
    }
    return 0xffffffffu;
}
