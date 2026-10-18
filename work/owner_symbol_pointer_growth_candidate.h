#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
// Normal nonoverflow path of102bdbd0. Overflow correction is not translated.
// Adapter allocator must preserve prior contents and follow its failure contract.
template<class Entry,class Allocator>
inline void growRecoveredStorageCandidate(Entry*& storage,
    std::int32_t& capacity,std::int32_t growSize,std::int32_t delta,
    std::int32_t initialCapacity,Allocator& allocator) {
    static_assert(std::is_trivially_copyable<Entry>::value,"raw storage entries required");
    if(growSize<0)return;
    if(capacity<0 || delta<=0)throw std::invalid_argument("unsupported growth input");
    const std::int64_t requested=std::int64_t(capacity)+delta;
    std::int64_t next=capacity;
    if(growSize>0)next=((requested-1)/growSize+1)*growSize;
    else {
        if(next==0)next=initialCapacity;
        while(next<requested)next*=2;
    }
    if(next>std::numeric_limits<std::int32_t>::max() ||
       std::uint64_t(next)>std::numeric_limits<std::size_t>::max()/sizeof(Entry))
        throw std::overflow_error("untranslated native growth overflow path");
    capacity=static_cast<std::int32_t>(next); // Original updates before allocation.
    const auto bytes=static_cast<std::size_t>(next)*sizeof(Entry);
    storage=storage?allocator.reallocate(storage,bytes):allocator.allocate(bytes);
}
template<class Pointer,class Allocator>
inline void growOwnerSymbolPointersCandidate(Pointer*& storage,
    std::int32_t& capacity,std::int32_t growSize,std::int32_t delta,Allocator& allocator) {
    static_assert(std::is_pointer<Pointer>::value,"pointer storage required");
    growRecoveredStorageCandidate(storage,capacity,growSize,delta,8,allocator);
}
