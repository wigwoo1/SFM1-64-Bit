#pragma once
#include <array>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "cached_key_search.h"
#include "float_interpolation.h"
#include "quaternion_interpolation.h"
// 1054F560: nonempty, uncompressed vector layer without owner curve info.
struct QuaternionKeySampler {
 std::vector<int32_t> times;
 std::vector<std::array<float,4>> values;
 CachedKeySearch search;
 QuaternionKeySampler(std::vector<int32_t> t,std::vector<std::array<float,4>> v):times(std::move(t)),values(std::move(v)){
  if(times.empty()||times.size()!=values.size()||times.size()>INT32_MAX||!std::is_sorted(times.begin(),times.end()))throw std::runtime_error("invalid vector key arrays");
 }
 std::array<float,4> sample(int32_t tick){
  auto i=search.find(times,tick);
  if(i<0)return values.front();
  if(static_cast<size_t>(i)==values.size()-1)return values.back();
  float fraction=timeFraction(tick,times[i],times[i+1],false);
  return interpolateRecoveredQuaternion(values[i],values[i+1],fraction);
 }
};
