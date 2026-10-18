#pragma once
#include <cstdint>
// 103623b0: retain live flag reads across phase query and callback preparation.
template<class Attribute,class Adapter>
inline bool attributeWriteGateCandidate(Attribute* p,Adapter& a) {
    if(a.flags(p)&0x20)return false;
    const auto phase=a.phase();
    if(phase!=0 && phase!=1 && phase!=2) {
        if(phase!=4 || (a.flags(p)&0x400))return false;
    }
    const auto flags=a.flags(p);
    if(!(flags&0x80) && (flags&0x2000))a.prepareCallbacks(p);
    a.setFlags(p,static_cast<std::uint16_t>(a.flags(p)|0x4080));
    a.markOwner(p);return true;
}
