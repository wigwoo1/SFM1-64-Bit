#pragma once
#include "cached_key_search.h"
#include "gather_float_curve_key.h"
#include "float_curve_evaluation.h"
#include <climits>
inline float recoveredCurveTimeSeconds(int32_t tick){return _mm_cvtss_f32(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),tick),_mm_set_ss(0.0001f)));}
//1054D1DA..1054D222: convert absolute times before subtraction.
inline float recoveredCurveFraction(int32_t tick,int32_t start,int32_t end){
 if(end<=start)return 0;
 auto a=_mm_set_ss(recoveredCurveTimeSeconds(start));auto b=_mm_set_ss(recoveredCurveTimeSeconds(end));auto t=_mm_set_ss(recoveredCurveTimeSeconds(tick));
 return _mm_cvtss_f32(_mm_div_ss(_mm_sub_ss(t,a),_mm_sub_ss(b,a)));
}
struct RecoveredFloatCurveSampler{
 std::vector<int32_t>times,kinds;std::vector<float>values;RecoveredFloatCurveInfo info;float minimum=0,maximum=1;CachedKeySearch search;
 float sample(int32_t tick){
  if((times.empty()&&info.rightEdgeTime==0)||times.size()!=values.size()||times.size()>static_cast<size_t>(INT_MAX-2))throw std::runtime_error("invalid or unsupported empty curve track");
  auto index=search.find(times,tick);
  if(info.rightEdgeTime!=0&&tick>info.rightEdgeTime)return info.useEdgeInfo?(info.rightActive?info.rightValue:info.defaultEdgeZero):0.f;
  if(info.rightEdgeTime==0&&index>=static_cast<int32_t>(times.size())-1)return values.back();
  std::array<int32_t,4>ticks{},codes{};std::array<float,4>seconds{},samples{};
  for(int j=0;j<4;++j){auto key=gatherRecoveredFloatCurveKey(times,values,kinds,info,index+j-1);ticks[j]=key.time;seconds[j]=recoveredCurveTimeSeconds(key.time);samples[j]=key.value;codes[j]=key.kind;}
  return evaluateRecoveredFloatCurve(seconds,samples,codes,recoveredCurveFraction(tick,ticks[1],ticks[2]),minimum,maximum);
 }
};
