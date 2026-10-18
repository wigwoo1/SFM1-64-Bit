#pragma once
#include <array>
#include <cstdint>
//109827f0. Nonnull terminated source and table values0..255 preconditions.
inline std::uint32_t hashOwnerSymbolCandidate(const char* source,
    const std::array<std::uint32_t,256>& table) {
    auto* bytes=reinterpret_cast<const unsigned char*>(source);
    std::uint32_t a=0,b=0;
    while (*bytes) {
        a=table[*bytes++ ^ b];
        if (!*bytes) break;
        b=table[*bytes++ ^ a];
    }
    return (a<<8)|b;
}
