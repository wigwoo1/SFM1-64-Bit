#pragma once
#include <cstdint>
// 1035e400. Full supplied mask participates in early/propagation gates;
// only its low two bits are accumulated locally. Caller supplies reference ABI.
template<class Element,class Adapter>
inline void propagateElementChangeCandidate(Element* e,std::uint32_t mask,Adapter& a) {
    if(a.suppressed())return;
    const bool already=(a.state(e)&0x10u)!=0;
    if(already) {
        const auto active=a.packed(e)&3u;
        if((active|mask)==active)return;
    }
    if(!already)a.enqueueReset(e);
    if(a.hasFilteredCallbacks(e)) {
        const auto packed=a.packed(e);
        const auto required=(packed>>2)&3u;
        const bool satisfied=((packed&required)&3u)==required && (a.state(e)&0x10u);
        if((required&mask)==required && !satisfied)a.enqueuePost(e);
    }
    const bool wasMarked=(a.state(e)&0x10u)!=0;
    a.setPacked(e,a.packed(e)|(mask&3u));
    a.setState(e,a.state(e)|0x10u);
    const auto propagated=mask&~2u;
    const auto active=a.packed(e)&3u;
    if(wasMarked && (active|propagated)==active)return;
    for(auto reference=a.firstIncoming(e);reference;reference=a.nextIncoming(reference)) {
        auto* attribute=a.resolveAttribute(reference);
        if(attribute && !(a.attributeFlags(attribute)&0x1000u))
            if(auto* owner=a.attributeOwner(attribute))
                propagateElementChangeCandidate(owner,propagated,a);
    }
}
