#pragma once
#include <initializer_list>
#include "engine_projection_planes_candidate.h"
//10246AF0: original thiscall returns true when any packed plane rejects box.
// Native loads16bytes per bound although onlyXYZ used; owned broadcastsXYZ.
inline bool engineProjectionBoxOutsidePlanesCandidate(const EngineProjectionPlanesCandidate& planes,const float* minimum,const float* maximum){
 auto select=[](__m128 mask,float low,float high){return _mm_or_ps(_mm_and_ps(mask,_mm_set1_ps(low)),_mm_andnot_ps(mask,_mm_set1_ps(high)));};
 for(const auto* block:{&planes.first,&planes.last}){
  auto load=[](const uint32_t* p){return _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(p)));};
  __m128 z=_mm_mul_ps(select(load(block->negative[2]),minimum[2],maximum[2]),load(block->components[2]));
  __m128 y=_mm_mul_ps(select(load(block->negative[1]),minimum[1],maximum[1]),load(block->components[1]));
  __m128 x=_mm_mul_ps(select(load(block->negative[0]),minimum[0],maximum[0]),load(block->components[0]));
  __m128 distance=_mm_add_ps(_mm_add_ps(z,y),x);
  if(_mm_movemask_ps(_mm_cmplt_ps(distance,load(block->components[3]))))return true;
 }
 return false;
}
