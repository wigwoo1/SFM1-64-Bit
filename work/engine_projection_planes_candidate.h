#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <emmintrin.h>
// Owned SIMD layout recovered from10246860/102468E0/102467E0.
// Source and destination must not overlap. Native execution parity pending.
struct alignas(16) EngineProjectionPlaneBlockCandidate {
 uint32_t components[4][4];
 uint32_t negative[3][4];
 uint32_t absolute[3][4];
};
struct alignas(16) EngineProjectionPlanesCandidate {
 EngineProjectionPlaneBlockCandidate first, last;
};
static_assert(sizeof(EngineProjectionPlaneBlockCandidate)==0xa0);
static_assert(offsetof(EngineProjectionPlaneBlockCandidate,negative)==0x40);
static_assert(offsetof(EngineProjectionPlaneBlockCandidate,absolute)==0x70);
static_assert(offsetof(EngineProjectionPlanesCandidate,last)==0xa0);
static_assert(sizeof(EngineProjectionPlanesCandidate)==0x140);
inline void engineProjectionPackBlockCandidate(const uint32_t* planes,unsigned count,EngineProjectionPlaneBlockCandidate& block){
 for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)block.components[c][r]=r<count?planes[r*4+c]:0;
 const __m128 zero=_mm_setzero_ps();
 const __m128 mask=_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
 for(unsigned c=0;c<3;++c){
  const __m128 values=_mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(block.components[c])));
  _mm_store_si128(reinterpret_cast<__m128i*>(block.negative[c]),_mm_castps_si128(_mm_cmplt_ps(values,zero)));
  _mm_store_si128(reinterpret_cast<__m128i*>(block.absolute[c]),_mm_castps_si128(_mm_and_ps(values,mask)));
 }
}
inline void engineProjectionPackPlanesCandidate(const uint32_t* sixPlanes,EngineProjectionPlanesCandidate& output){
 engineProjectionPackBlockCandidate(sixPlanes,4,output.first);
 engineProjectionPackBlockCandidate(sixPlanes+16,2,output.last);
}
