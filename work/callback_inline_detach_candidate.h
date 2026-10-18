#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <cassert>
template<class Pointer,class Allocator>
inline void detachCallbackInlineStorageCandidate(Pointer*& storage,
    std::int32_t capacity,std::int32_t& growSize,std::int32_t newGrowSize,Allocator& a) {
    static_assert(std::is_pointer<Pointer>::value,"pointer storage required");
    if(growSize>=0)return;
    assert(capacity>=0);
    growSize=newGrowSize;
    if(capacity==0) {storage=nullptr;return;}
    const auto bytes=static_cast<std::size_t>(capacity)*sizeof(Pointer);
    auto* owned=a.allocate(bytes);
    std::memcpy(owned,storage,bytes);
    storage=owned;
}
