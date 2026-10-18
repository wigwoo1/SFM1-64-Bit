#pragma once
#include <cstdint>
template<class Attribute,class Adapter>
inline bool markAttributeReferenceCandidate(Attribute* attribute,Adapter& a) {
    if(a.flags(attribute)&0x20u)return false;
    const auto phase=a.phase();
    if(phase!=0 && phase!=1 && phase!=2 &&
       !(phase==4 && !(a.flags(attribute)&0x400u)))return false;
    const auto flags=a.flags(attribute);
    if(!(flags&0x80u) && (flags&0x2000u))a.prepare(a.owner(attribute),attribute);
    auto* owner=a.owner(attribute);
    a.setFlags(attribute,static_cast<std::uint16_t>(a.flags(attribute)|0x4080u));
    a.markOwner(owner);
    return true;
}
