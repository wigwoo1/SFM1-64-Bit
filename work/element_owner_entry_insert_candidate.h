#pragma once
#include "element_owner_registration_candidate.h"
#include <cstring>
#include <type_traits>
#include <cassert>
// Adapter supplies production growth/failure contract. Valid insertion range
// and signed count+1 representability are caller preconditions.
template<class Adapter>
inline std::int32_t insertElementOwnerEntryCandidate(std::int32_t index,Adapter& a) {
    using Entry=ElementOwnerPoolEntryCandidate;
    static_assert(std::is_trivially_copyable<Entry>::value,"memmove entries");
    assert(index>=0 && index<=a.count());
    const auto required=a.count()+1;
    if(required>a.capacity())a.grow(required-a.capacity());
    a.setCount(required);
    auto* entries=a.entries();a.setCachedEntries(entries);
    const auto trailing=required-index-1;
    if(trailing>0)std::memmove(entries+index+1,entries+index,
        static_cast<std::size_t>(trailing)*sizeof(Entry));
    entries[index].generationAndFlags=0;entries[index].object=nullptr;
    return index;
}
