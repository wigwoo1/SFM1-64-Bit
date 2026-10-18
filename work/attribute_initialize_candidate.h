#pragma once
#include <cstdint>
#include <limits>
// Original constructor10363350. Adapter owns native layout and typed data
// initialization; no inferred ownership or valid-range policy.
template<class Attribute,class Owner,class Adapter>
inline Attribute* initializeAttributeCandidate(Attribute* p,Owner* owner,std::uint32_t type,const char* name,void* data,Adapter& a) {
    a.setData(p,data);a.setNameSymbol(p,std::numeric_limits<std::uintptr_t>::max());a.setOwner(p,owner);
    a.setNameSymbol(p,a.internName(name));
    a.setFlags(p,static_cast<std::uint16_t>(type));
    a.setHandle(p,0xffffffffu);a.setNext(p,nullptr);
    if(type==1 || type==15)a.setFlags(p,static_cast<std::uint16_t>(a.flags(p)|0x400));
    a.initializeTypedData(a.flags(p)&31,a.data(p));
    a.setFlags(p,static_cast<std::uint16_t>(a.flags(p)|0x200));return p;
}
