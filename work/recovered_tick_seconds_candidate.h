#pragma once
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
#include <emmintrin.h>
inline float recoveredTickSecondsCandidate(int32_t tick){uint32_t bits=0x38d1b717;float factor;std::memcpy(&factor,&bits,4);return _mm_cvtss_f32(_mm_mul_ss(_mm_cvtepi32_ps(_mm_cvtsi32_si128(tick)),_mm_set_ss(factor)));}
