#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <immintrin.h>
#include "source_collision_surface_candidate.h"
struct SourceOptimizedBoxLineStateCandidate {
 float fraction=1,leftSolid=0;bool startSolid=false,allSolid=false;uint32_t contents=0;
 std::array<float,4>plane{};
 SourceCollisionSurfaceCandidate surface;
 // Retained cplane_t bytes: type, signbits, two reserved bytes. Terrain hits
 // can change the normal without updating these tags.
 std::array<uint8_t,4>planeTags{};
};
// Owned SSE2 reconstruction of 10146850 for zero-extent line rays only.
// Box selection by the map loader, surface metadata and swept hulls are separate.
inline void sourceOptimizedBoxLineRayCandidate(const std::array<float,3>&mins,const std::array<float,3>&maxs,
 const std::array<float,3>&start,const std::array<float,3>&end,uint32_t contents,SourceOptimizedBoxLineStateCandidate&state,bool*enteringPlaneHit=nullptr,const std::array<float,3>*rayDelta=nullptr){
 std::array<float,4>low{},high{},delta{},inverse{};
 for(size_t i=0;i<3;++i){if(!std::isfinite(mins[i])||!std::isfinite(maxs[i])||mins[i]>maxs[i]||!std::isfinite(start[i])||!std::isfinite(end[i]))throw std::runtime_error("invalid optimized box/ray");low[i]=mins[i]-start[i];high[i]=maxs[i]-start[i];delta[i]=rayDelta?(*rayDelta)[i]:end[i]-start[i];inverse[i]=delta[i]==0?std::numeric_limits<float>::max():1.0f/delta[i];if(!std::isfinite(low[i])||!std::isfinite(high[i])||!std::isfinite(delta[i])||!std::isfinite(inverse[i]))throw std::runtime_error("optimized box input arithmetic overflow");}
 if(!std::isfinite(state.fraction)||!std::isfinite(state.leftSolid)||state.fraction<0||state.fraction>1||state.leftSolid<0||state.leftSolid>1)throw std::runtime_error("invalid optimized box state");
 const __m128 zero=_mm_setzero_ps(),one=_mm_set1_ps(1),negativeLimit=_mm_set1_ps(-std::numeric_limits<float>::max()),positiveLimit=_mm_set1_ps(std::numeric_limits<float>::max());
 const __m128 xyz=_mm_castsi128_ps(_mm_set_epi32(0,-1,-1,-1));
 __m128 lo=_mm_loadu_ps(low.data()),hi=_mm_loadu_ps(high.data()),d=_mm_loadu_ps(delta.data()),inv=_mm_loadu_ps(inverse.data());
 const __m128 startBelow=_mm_cmplt_ps(zero,lo),startAbove=_mm_cmplt_ps(hi,zero);
 const __m128 endAbove=_mm_cmplt_ps(hi,d),endBelow=_mm_cmplt_ps(d,lo);
 const __m128 reject=_mm_and_ps(_mm_or_ps(_mm_and_ps(endAbove,startAbove),_mm_and_ps(endBelow,startBelow)),xyz);
 if(_mm_movemask_ps(reject))return;
 const __m128 active=_mm_or_ps(_mm_xor_ps(startAbove,endAbove),_mm_xor_ps(endBelow,startBelow));
 const __m128 inactiveEnter=_mm_andnot_ps(active,negativeLimit),inactiveExit=_mm_andnot_ps(active,positiveLimit);
 auto enterLanes=[&](__m128 a,__m128 b){return _mm_or_ps(_mm_and_ps(_mm_min_ps(a,b),active),inactiveEnter);};
 auto exitLanes=[&](__m128 a,__m128 b){return _mm_or_ps(_mm_and_ps(_mm_max_ps(a,b),active),inactiveExit);};
 auto max3=[](__m128 value){return _mm_max_ps(_mm_max_ps(value,_mm_shuffle_ps(value,value,0x39)),_mm_shuffle_ps(value,value,0x4e));};
 auto min3=[](__m128 value){return _mm_min_ps(_mm_min_ps(value,_mm_shuffle_ps(value,value,0x39)),_mm_shuffle_ps(value,value,0x4e));};
 const __m128 initialLo=_mm_mul_ps(lo,inv),initialHi=_mm_mul_ps(hi,inv);
 const float unexpandedEnter=_mm_cvtss_f32(_mm_max_ps(_mm_shuffle_ps(max3(enterLanes(initialLo,initialHi)),max3(enterLanes(initialLo,initialHi)),0),zero));
 const float unexpandedExit=_mm_cvtss_f32(_mm_min_ps(_mm_shuffle_ps(min3(exitLanes(initialLo,initialHi)),min3(exitLanes(initialLo,initialHi)),0),one));
 if(unexpandedExit<unexpandedEnter)return;
 const bool outside=_mm_movemask_ps(_mm_and_ps(_mm_or_ps(startAbove,startBelow),xyz))!=0;
 const __m128 epsilon=_mm_set1_ps(0.03125f);lo=_mm_mul_ps(_mm_sub_ps(lo,epsilon),inv);hi=_mm_mul_ps(_mm_add_ps(hi,epsilon),inv);
 const __m128 order=_mm_cmple_ps(lo,hi);
 const __m128 minimumFaces=_mm_castsi128_ps(_mm_set_epi32(-1,2,1,0)),maximumFaces=_mm_castsi128_ps(_mm_set_epi32(-1,5,4,3));
 const __m128 faces=_mm_or_ps(_mm_and_ps(order,minimumFaces),_mm_andnot_ps(order,maximumFaces));
 const __m128 entering=enterLanes(lo,hi),leaving=exitLanes(lo,hi);
 const __m128 xyEntry=_mm_max_ps(entering,_mm_shuffle_ps(entering,entering,0x39));
 const __m128 yLessX=_mm_cmplt_ps(_mm_shuffle_ps(entering,entering,0x39),entering);
 const __m128 xyFace=_mm_or_ps(_mm_and_ps(yLessX,faces),_mm_andnot_ps(yLessX,_mm_shuffle_ps(faces,faces,0x39)));
 const __m128 zLessXY=_mm_cmplt_ps(_mm_shuffle_ps(entering,entering,0x4e),xyEntry);
 const __m128 finalFace=_mm_or_ps(_mm_and_ps(zLessXY,xyFace),_mm_andnot_ps(zLessXY,_mm_shuffle_ps(faces,faces,0x4e)));
 const __m128 e=max3(entering),l=min3(leaving);
 const float entry=_mm_cvtss_f32(_mm_max_ps(_mm_shuffle_ps(e,e,0),zero)),exit=_mm_cvtss_f32(_mm_min_ps(_mm_shuffle_ps(l,l,0),one));
 if(exit<entry)return;
 auto result=state;
 if(!outside||result.leftSolid>entry){result.startSolid=true;result.contents=contents;if(exit>=1){result.allSolid=true;result.fraction=0;}else if(exit>result.leftSolid){result.leftSolid=exit;if(exit>=result.fraction){result.fraction=1;result.surface={};}}}
 else if(result.fraction>entry){const int face=_mm_cvtsi128_si32(_mm_castps_si128(finalFace));if(face<0||face>5)throw std::runtime_error("invalid optimized entering face");const size_t axis=size_t(face%3);result.fraction=entry;result.contents=contents;result.plane={0,0,0,face<3?-mins[axis]:maxs[axis]};result.plane[axis]=face<3?-1.0f:1.0f;result.planeTags[0]=uint8_t(axis);result.planeTags[1]=face<3?uint8_t(1u<<axis):0;result.surface={SourceCollisionSurfaceKindCandidate::BoxFace,uint32_t(face)};if(enteringPlaneHit)*enteringPlaneHit=true;}
 if(!std::isfinite(result.fraction)||!std::isfinite(result.leftSolid))throw std::runtime_error("optimized box output nonfinite");state=result;
}
