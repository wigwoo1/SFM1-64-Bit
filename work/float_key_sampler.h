#pragma once
#include "cached_key_search.h"
#include "float_interpolation.h"
// Uncompressed, no-curve-info branch only; caller must verify owner path.
struct FloatKeySampler {
 std::vector<int32_t> times;std::vector<float> values;CachedKeySearch search;
 FloatKeySampler(const Graph&g,const Element&layer){KeyTrack track(g,layer);if(track.values->type!=17)throw std::runtime_error("expected float value array");
  if(track.times->values.size()>INT32_MAX)throw std::runtime_error("too many keys");
  for(auto&v:track.times->values)times.push_back(wireInt(v));for(auto&v:track.values->values)values.push_back(wireFloat(v));
  if(times.empty())throw std::runtime_error("empty layer requires owner default evaluation");
 }
 float sample(int32_t tick){auto i=search.find(times,tick);if(i<0)return values.front();if(static_cast<size_t>(i)==values.size()-1)return values.back();return interpolateFloat(values[i],values[i+1],timeFraction(tick,times[i],times[i+1],false));}
};
