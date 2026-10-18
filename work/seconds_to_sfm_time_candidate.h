#pragma once
#include <xmmintrin.h>
#include <emmintrin.h>
#include <cstdint>
// 1097b170 SSE sequence, preserving original single-precision intermediates
// and caller MXCSR behavior. Do not substitute std::round (negative ties differ).
inline std::int32_t secondsToSfmTimeCandidate(float seconds) {
    auto x=_mm_add_ss(_mm_mul_ss(_mm_set_ss(seconds),_mm_set_ss(10000.0f)),_mm_set_ss(0.5f));
    auto sign=_mm_and_ps(_mm_set_ss(-0.0f),x);
    auto magnitude=_mm_xor_ps(x,sign);
    auto threshold=_mm_set_ss(8388608.0f);
    auto offset=_mm_or_ps(_mm_and_ps(threshold,_mm_cmplt_ss(magnitude,threshold)),sign);
    auto rounded=_mm_sub_ss(_mm_add_ss(x,offset),offset);
    auto difference=_mm_sub_ss(rounded,x);
    auto correction=_mm_and_ps(_mm_cmpnle_ss(difference,sign),_mm_set_ss(1.0f));
    return _mm_cvttss_si32(_mm_sub_ss(rounded,correction));
}
