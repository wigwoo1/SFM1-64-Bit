#pragma once
#include <cstdint>
inline bool modelUndoRecordEnabledCandidate(std::uint8_t flagsC18c) {return (flagsC18c&8)!=0;}
template<class Owner,class Adapter>
inline bool modelAllowsUndoRecordCandidate(Owner* owner,Adapter& a) {
    if(!a.undoRecordEnabled())return false;
    return owner && a.ownerField38(owner)!=0xffffffffu;
}

// Model60 normalizes any nonzero input byte, unlike suppression setter bit0.
inline void setModelUndoRecordEnabledCandidate(std::uint8_t& flagsC18c,std::uint8_t incoming) {
    flagsC18c=static_cast<std::uint8_t>((flagsC18c&0xf7u)|(incoming?8u:0u));
}
