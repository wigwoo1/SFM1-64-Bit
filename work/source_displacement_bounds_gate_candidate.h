#pragma once
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <emmintrin.h>

// Original 1014E790. Its unexpanded endpoint rejection and active-axis mask
// precede padded slab arithmetic, so this is not a generic expanded-box test.
inline bool sourceDisplacementBoundsGateCandidate(const std::array<float,3>&mins,
 const std::array<float,3>&maxs,const std::array<float,3>&start,
 const std::array<float,3>&delta,const std::array<float,3>&inverse,float padding=.03125f){
 for(size_t a=0;a<3;++a)if(!std::isfinite(mins[a])||!std::isfinite(maxs[a])||mins[a]>maxs[a]||!std::isfinite(start[a])||!std::isfinite(delta[a])||!std::isfinite(inverse[a]))throw std::runtime_error("invalid displacement bounds-gate input");
 if(!std::isfinite(padding)||padding<0)throw std::runtime_error("invalid displacement bounds padding");
 auto vector=[](const std::array<float,3>&v){return _mm_set_ps(0.f,v[2],v[1],v[0]);};
 const __m128 zero=_mm_setzero_ps(),one=_mm_set1_ps(1.f),d=vector(delta),s=vector(start);
 __m128 lo=_mm_sub_ps(vector(mins),s),hi=_mm_sub_ps(vector(maxs),s);
 const __m128 hiNegative=_mm_cmplt_ps(hi,zero),hiLessDelta=_mm_cmplt_ps(hi,d),deltaLessLo=_mm_cmplt_ps(d,lo),loPositive=_mm_cmplt_ps(zero,lo);
 const __m128 outside=_mm_or_ps(_mm_and_ps(hiLessDelta,hiNegative),_mm_and_ps(deltaLessLo,loPositive));
 const __m128 xyz=_mm_castsi128_ps(_mm_set_epi32(0,-1,-1,-1));if(_mm_movemask_ps(_mm_and_ps(outside,xyz)))return false;
 const __m128 active=_mm_or_ps(_mm_xor_ps(hiLessDelta,hiNegative),_mm_xor_ps(deltaLessLo,loPositive));
 const __m128 pad=_mm_set1_ps(padding),inv=vector(inverse);
 lo=_mm_mul_ps(_mm_sub_ps(lo,pad),inv);hi=_mm_mul_ps(_mm_add_ps(pad,hi),inv);
 const __m128 low=_mm_or_ps(_mm_and_ps(lo,active),_mm_andnot_ps(active,_mm_set1_ps(-std::numeric_limits<float>::max())));
 const __m128 high=_mm_or_ps(_mm_and_ps(hi,active),_mm_andnot_ps(active,_mm_set1_ps(std::numeric_limits<float>::max())));
 const __m128 near=_mm_min_ps(low,high),far=_mm_max_ps(low,high);
 __m128 exit=_mm_min_ps(_mm_min_ps(far,_mm_shuffle_ps(far,far,0x39)),_mm_shuffle_ps(far,far,0x4e));
 __m128 entry=_mm_max_ps(_mm_max_ps(near,_mm_shuffle_ps(near,near,0x39)),_mm_shuffle_ps(near,near,0x4e));
 exit=_mm_min_ps(_mm_shuffle_ps(exit,exit,0),one);entry=_mm_max_ps(_mm_shuffle_ps(entry,entry,0),zero);
 return _mm_movemask_ps(_mm_cmpeq_ps(_mm_cmplt_ps(exit,entry),zero))==15;
}
