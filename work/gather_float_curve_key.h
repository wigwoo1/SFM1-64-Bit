#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
struct RecoveredFloatCurveInfo{
 int32_t defaultKind=0,rightEdgeTime=0;
 bool useEdgeInfo=false,leftActive=false,rightActive=false;
 float leftValue=0,rightValue=0,defaultEdgeZero=0;
 int32_t leftKind=0,rightKind=0;
};
struct RecoveredFloatCurveKey{int32_t time=0;float value=0;int32_t kind=0;};
//1051B020, with edge values105089F0/10508480. Uncompressed input only.
inline RecoveredFloatCurveKey gatherRecoveredFloatCurveKey(const std::vector<int32_t>&times,const std::vector<float>&values,const std::vector<int32_t>&kinds,const RecoveredFloatCurveInfo&info,int32_t index,bool ownerPresent=true){
 if(!ownerPresent)return {};
 if(times.size()!=values.size())throw std::runtime_error("curve key/value count mismatch");
 auto edge=[&](bool right){bool active=right?info.rightActive:info.leftActive;return info.useEdgeInfo?(active?(right?info.rightValue:info.leftValue):info.defaultEdgeZero):0.f;};
 if(index<0)return {0,edge(false),info.leftActive&&info.leftKind?info.leftKind:info.defaultKind};
 if(static_cast<size_t>(index)>=times.size()){
  int32_t time=info.rightEdgeTime;
  if(time==0&&!times.empty()){uint32_t wrapped=static_cast<uint32_t>(times.back())+1u;std::memcpy(&time,&wrapped,sizeof(time));}
  return {time,edge(true),info.rightActive&&info.rightKind?info.rightKind:info.defaultKind};
 }
 auto i=static_cast<size_t>(index);return {times[i],values[i],i<kinds.size()&&kinds[i]?kinds[i]:info.defaultKind};
}
