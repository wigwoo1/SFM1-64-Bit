#pragma once

#include <cstdint>

// Value-only conversion for the reviewed lightmap upload contract. Mantissas
// truncate toward zero, overflow saturates, and NaNs become signed zero.
// This does not reproduce floating-point exception flags or traps.
inline constexpr uint16_t nativeLightmapHalfBitsCandidate(uint32_t inputBits) noexcept {
    const uint32_t sign = (inputBits >> 16u) & 0x8000u;
    const uint32_t magnitude = inputBits & 0x7fffffffu;
    const uint32_t exponent = magnitude >> 23u;
    const uint32_t mantissa = magnitude & 0x7fffffu;
    if (exponent == 255u && mantissa != 0u) {
        return static_cast<uint16_t>(sign);
    }
    if (magnitude > 0x477fe000u) {
        return static_cast<uint16_t>(sign | 0x7bffu);
    }
    if (exponent < 103u) {
        return static_cast<uint16_t>(sign);
    }
    if (exponent < 113u) {
        // Here the shift is in [14, 23], including half subnormals.
        return static_cast<uint16_t>(sign | ((0x800000u | mantissa) >> (126u - exponent)));
    }
    return static_cast<uint16_t>(sign | ((exponent - 112u) << 10u) | (mantissa >> 13u));
}
