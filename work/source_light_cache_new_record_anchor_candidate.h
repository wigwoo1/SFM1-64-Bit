#pragma once
#include "source_light_cache_position_candidate.h"

// New-record branch only: original 1019794A..101979C5. ConVar integer zero
// copies the query point; ANY nonzero value invokes cache-position selection.
// Existing and throttled/borrowed records reuse stored positions elsewhere.
// This helper must not be applied to a cache hit as a per-frame recenter rule.
template<class World>
std::array<float,3> sourceLightCacheNewRecordAnchorCandidate(
 const std::array<float,3>&point,int32_t inputLeaf,int32_t cacheCenter,World&world){
 for(float value:point)if(!std::isfinite(value))throw std::runtime_error("nonfinite new cache-record point");
 if(inputLeaf<0)throw std::runtime_error("negative new cache-record leaf");
 return cacheCenter==0?point:sourceLightCachePositionCandidate(point,inputLeaf,world);
}
