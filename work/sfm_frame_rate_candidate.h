#pragma once
#include <cstdint>
#include <xmmintrin.h>
#include <emmintrin.h>
struct SfmFrameRateCandidate {std::uint16_t numerator,timeUnits;};
// 1097b6d0: rounding adapter must implement original x87 FISTP semantics.
// Original stores only low16 numerator bits; timeUnits is 10000 or 10010.
template<class RoundX87>
inline SfmFrameRateCandidate constructSfmFrameRateCandidate(float rate,RoundX87&& roundX87) {
    auto value=_mm_set_ss(rate);
    const auto absMask=_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    auto rounded=_mm_set_ss(static_cast<float>(roundX87(rate)));
    const auto integerDifference=_mm_cvtss_f32(_mm_and_ps(_mm_sub_ss(rounded,value),absMask));
    if(integerDifference<0.001f)return {static_cast<std::uint16_t>(roundX87(rate)),10000};
    auto scaled=_mm_mul_ss(_mm_mul_ss(value,_mm_set_ss(1001.0f)),_mm_set_ss(0.001f));
    const auto scaledValue=_mm_cvtss_f32(scaled);
    rounded=_mm_set_ss(static_cast<float>(roundX87(scaledValue)));
    const auto fractionalDifference=_mm_cvtss_f32(_mm_and_ps(_mm_sub_ss(rounded,scaled),absMask));
    if(fractionalDifference<0.01f)return {static_cast<std::uint16_t>(roundX87(scaledValue)),10010};
    return {static_cast<std::uint16_t>(roundX87(rate)),10000};
}
