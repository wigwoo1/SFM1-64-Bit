#pragma once
#include <cstdint>
inline std::uint32_t controlModelBaseFlagsCandidate(std::int32_t row,
    std::int32_t column,bool modelIdentityMatches) {
    return row>=0 && column>=0 && modelIdentityMatches ? 0x21u : 0u;
}
// Base flags supplied by UI framework; callbacks implement original node lookup.
template<class ResolveGroup, class IsTransform>
std::uint32_t controlModelItemFlagsCandidate(std::uint32_t base, bool nodePresent,
    std::int32_t column, std::int32_t kind, ResolveGroup resolveGroup, IsTransform isTransform) {
    if (!nodePresent) return base;
    if (column == 2) return (base & ~1u) | ((kind == 1 || kind == 2) ? 0x10u : 0u);
    if (column == 3) {
        if (resolveGroup()) return (base & ~1u) | 0x10u;
        return (base & ~1u) | (kind == 1 ? 0x50u : 0u);
    }
    if (column == 1) {
        if (kind == 5 || kind == 9) return (base & ~1u) | 0x1cu;
        if (kind == 4 && isTransform()) return (base & ~1u) | 0x5cu;
        return base & ~0x21u;
    }
    return base;
}
