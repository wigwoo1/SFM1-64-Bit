#pragma once
#include <cstdint>
template<class Element,class Adapter>
inline void markElementStateCandidate(Element* element,Adapter& a) {
    if(!(a.state(element)&1u))a.enqueue(a.handle(element));
    a.setState(element,static_cast<std::uint8_t>(a.state(element)|3u));
}
template<class Element,class Adapter>
inline void clearElementAttributeMarkCandidate(Element* element,Adapter& a) {
    auto* attribute=a.firstAttribute(element);
    a.setState(element,static_cast<std::uint8_t>(a.state(element)&0xfeu));
    while(attribute) {
        a.setFlags(attribute,static_cast<std::uint16_t>(a.flags(attribute)&0xff7fu));
        attribute=a.nextAttribute(attribute);
    }
}
