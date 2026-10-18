#pragma once
#include "attribute_write_gate_candidate.h"
// 10365ba0 +10366100. Adapter supplies native service bindings; the source
// native symbol pointer stays live across optional change-record construction.
template<class Attribute,class Adapter>
inline void updateStringAttributeCandidate(Attribute* p,const std::uintptr_t* source,Adapter& a) {
    if((a.flags(p)&31)!=5)return;
    a.initializeStringCompareDefault();
    const auto old=(a.flags(p)&31)==5?a.stringSymbol(p):a.stringCompareDefault();
    if(old==*source || !attributeWriteGateCandidate(p,a))return;
    if(a.shouldRecordChange(p))a.submitStringChangeRecord(a.createStringChangeRecord(p,source));
    a.setStringSymbol(p,*source);
    if((a.flags(p)&0x100) && a.ownerNotificationReady(p))a.notifyOwner(p);
    const auto type=a.flags(p)&31;
    const bool reference=type==1 || type==15;
    a.propagateOwner(p,reference?3u:2u);
    a.markModelChange(reference?0x1000u:0x2000u);
}
