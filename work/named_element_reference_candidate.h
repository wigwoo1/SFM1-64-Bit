#pragma once
#include <cstdint>
// 10002780/10002830. Adapter owns original distinct defaults and shared scratch;
// Type is read again on mismatch; stable mismatches return the default.
// Scratch helper copies a raw32 handle, with no coercion or parsing.
template<class Attribute,class Adapter>
inline std::uint32_t attributeElementHandleCandidate(Attribute* attribute,Adapter& a) {
    a.initializeAttributeDefault();
    if((a.attributeFlags(attribute)&31)==1)return a.directHandle(attribute);
    if(a.isElementHandleType(attribute)) {
        a.copyIntoSharedHandle(attribute);
        return a.sharedHandle();
    }
    return a.attributeDefaultHandle();
}
template<class Element,class Adapter>
inline Element* namedElementReferenceCandidate(Element* element,const char* name,Adapter& a) {
    a.initializeNamedDefault();
    auto* attribute=a.findAttribute(element,name);
    const auto handle=attribute?attributeElementHandleCandidate(attribute,a):a.namedDefaultHandle();
    auto* resolved=a.resolve(handle);
    if(!resolved || !a.acceptElementType(resolved))return nullptr;
    return resolved;
}
