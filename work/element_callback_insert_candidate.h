#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>
template<class Pointer,class Adapter>
inline std::int32_t insertElementCallbackCandidate(std::int32_t index,
    const Pointer* source,Adapter& a) {
    assert(index>=0 && index<=a.count());
    const auto required=a.count()+1;
    if(required>a.capacity()) {
        const auto delta=required-a.capacity();
        if(a.external())a.detach(a.inlineCapacity());
        a.grow(delta);
    }
    a.setCount(required);
    auto* data=a.data();a.setCached(data);
    const auto trailing=required-index-1;
    if(trailing>0)std::memmove(data+index+1,data+index,
        static_cast<std::size_t>(trailing)*sizeof(Pointer));
    data[index]=*source;
    return index;
}
