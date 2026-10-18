#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
// Recovered engine101bc4f3..101bc539. Interval must be supplied from actual clock ownership.
inline std::array<uint32_t,2>engineAnimationClockWordsCandidate(int32_t tick,int32_t previousTick,uint32_t intervalWord){
 float interval;std::memcpy(&interval,&intervalWord,4);const auto scale=_mm_set_ss(interval);
 const uint32_t differenceWord=uint32_t(tick)-uint32_t(previousTick);int32_t difference;std::memcpy(&difference,&differenceWord,4);
 std::array<uint32_t,2>result{};float value;
 _mm_store_ss(&value,_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),tick),scale));std::memcpy(&result[0],&value,4);
 _mm_store_ss(&value,_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),difference),scale));std::memcpy(&result[1],&value,4);return result;
}
