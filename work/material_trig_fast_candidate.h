#pragma once
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>

// Independently expressed numerical fast path observed in the installed CRT.
// Requires round-to-nearest-even, gradual underflow, and /fp:strict. No FMA.
// Returns false for finite |radians| >=8192, handled by the separate owned
// legacy fallback candidate. No original instructions are embedded here.
inline double materialTrigTableCandidate(unsigned index) {
    // Rounded binary64 sin(k*pi/32), k=0..16. Independently regenerated with
    // 100-decimal-digit arithmetic; all installed sine/cosine table entries
    // follow from these values by reflection, including positive zero axes.
    constexpr double quarter[] = {
        0x0p+0, 0x1.917a6bc29b42cp-4, 0x1.8f8b83c69a60bp-3,
        0x1.294062ed59f06p-2, 0x1.87de2a6aea963p-2,
        0x1.e2b5d3806f63bp-2, 0x1.1c73b39ae68c8p-1,
        0x1.44cf325091dd6p-1, 0x1.6a09e667f3bcdp-1,
        0x1.8bc806b151741p-1, 0x1.a9b66290ea1a3p-1,
        0x1.c38b2f180bdb1p-1, 0x1.d906bcf328d46p-1,
        0x1.e9f4156c62ddap-1, 0x1.f6297cff75cb0p-1,
        0x1.fd88da3d12526p-1, 0x1p+0
    };
    index &= 63;
    if(index <= 16) return quarter[index];
    if(index <= 32) return quarter[32-index];
    if(index <= 48) return -quarter[index-32];
    return -quarter[64-index];
}

inline bool materialTrigFastCandidate(float radians, bool cosine, float& value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &radians, sizeof bits);
    const uint32_t magnitude = bits & 0x7fffffffu;
    if(magnitude >= 0x7f800000u) {
        // SSE subtraction preserves a NaN's sign/payload while quieting it;
        // infinities produce the native indefinite NaN. FP flags not promised.
        _mm_store_ss(&value, _mm_sub_ss(_mm_set_ss(radians), _mm_set_ss(radians)));
        return true;
    }
    const double x = radians;
    if(magnitude < 0x00800000u) {
        // The original tiny path is deliberately not replaced by host sin/cos.
        value = static_cast<float>(cosine ? 1.0-x*x : x*0x1.fffffff8p-1);
        return true;
    }
    if(magnitude >= 0x46000000u) return false;
    const int n = _mm_cvtss_si32(_mm_mul_ss(_mm_set_ss(radians), _mm_set_ss(0x1.45f306p+3f)));
    // Original exponent-shift trick is exactly a power-of-two rescaling of
    // these products within the fast range. Keep the two reductions separate.
    const double r = x - static_cast<double>(n)*0x1.921fb54444p-4;
    const double corrected = r + static_cast<double>(n)*0x1.2e7b9676733afp-44;
    const double squared = r*r;
    const double fourth = squared*squared;
    const double cosPolynomial = (squared*(-0.5)+1.0)+fourth*(1.0/24.0);
    const double sinPolynomial = (squared*(-1.0/6.0)+1.0)+fourth*(1.0/120.0);
    const unsigned index = (static_cast<unsigned>(n)+(cosine ? 16u : 0u)) & 63u;
    const double sineTerm = materialTrigTableCandidate(index)*cosPolynomial;
    const double cosineTerm = (corrected*materialTrigTableCandidate(index+16))*sinPolynomial;
    value = static_cast<float>(sineTerm+cosineTerm);
    return true;
}
