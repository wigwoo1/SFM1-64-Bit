#pragma once
#include <cstdint>
// Payload subset10365770; native symbols are pointer-width; base undo metadata/vtable/allocation stay external.
struct StringAttributeUndoPayloadCandidate {
    std::uint32_t ownerHandle;
    std::uintptr_t attributeName,oldSymbol,newSymbol;
};
template<class Attribute,class Adapter>
inline StringAttributeUndoPayloadCandidate captureStringUndoPayloadCandidate(Attribute* p,const std::uintptr_t* source,Adapter& a) {
    StringAttributeUndoPayloadCandidate result{};
    result.ownerHandle=a.ownerHandle(p);
    a.initializeStringCompareDefault();
    result.oldSymbol=(a.flags(p)&31)==5?a.stringSymbol(p):a.stringCompareDefault();
    result.newSymbol=*source;
    result.attributeName=a.attributeName(p);
    return result;
}
