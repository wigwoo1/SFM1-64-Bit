#pragma once
#include <cstdint>
inline bool elementCallbackMaskSelectedCandidate(std::uint32_t required,
    std::uint32_t current) {
    return (required&current&3u)==required;
}
