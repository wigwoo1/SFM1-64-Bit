#pragma once
#include "gather_quaternion_curve_key.h"
#include "quaternion_curve_dispatch.h"
#include "float_curve_sampler.h"
#include <algorithm>
// 1054D4D0: uncompressed vector layer with live owner and curve info.
struct RecoveredQuaternionCurveSampler {
 std::vector<int32_t> times,kinds;std::vector<std::array<float,4>>values;
 RecoveredQuaternionCurveInfo info;CachedKeySearch search;
 std::array<float,4> sample(int32_t tick){
  if(times.size()!=values.size()||times.size()>static_cast<size_t>(INT_MAX-2)||!std::is_sorted(times.begin(),times.end())||(times.empty()&&!info.rightEdgeTime))throw std::runtime_error("invalid or unsupported empty vector curve track");
  auto index=search.find(times,tick);
  // Native vector path returns zero beyond the configured right edge.
  if(info.rightEdgeTime&&tick>info.rightEdgeTime)return {0,0,0,1};
  if(!info.rightEdgeTime&&index>=static_cast<int32_t>(times.size())-1)return values.back();
  std::array<int32_t,4> ticks{},codes{};std::array<std::array<float,4>,4>points{};
  for(int j=0;j<4;++j){auto key=gatherRecoveredQuaternionCurveKey(times,values,kinds,index+j-1,info);ticks[j]=key.time;codes[j]=key.kind;points[j]=key.value;}
  return evaluateRecoveredQuaternionEnvelope(points,recoveredCurveFraction(tick,ticks[1],ticks[2]),codes[1],codes[2]);
 }
};
