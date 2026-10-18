#pragma once
#include <cstdint>
#include <cstring>
struct OwnerSymbolArenaBlockCandidate {std::uint32_t capacity=0,used=0;};
//103543b0 signed comparison after native32-bit subtraction; no reservation.
inline std::int32_t findOwnerSymbolArenaFitCandidate(
    const OwnerSymbolArenaBlockCandidate* const* blocks,std::int32_t count,
    std::int32_t requested) {
    for (std::int32_t i=0;i<count;++i) {
        const std::uint32_t bits=blocks[i]->capacity-blocks[i]->used;
        std::int32_t remaining;std::memcpy(&remaining,&bits,sizeof(remaining));
        if (remaining>=requested) return i;
    }
    return -1;
}
