#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
// Original10361790 writes x86 pointer sentinel. Native CUtlSymbolLarge
// pointer storage widens; serialized DMX strings remain separate.
inline void initializeStringAttributeDataCandidate(void* data) {
    const std::uintptr_t invalid=std::numeric_limits<std::uintptr_t>::max();
    std::memcpy(data,&invalid,sizeof(invalid));
}
