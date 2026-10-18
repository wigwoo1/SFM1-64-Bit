#pragma once
#include <array>
#include <cstring>
#include <vector>
#include <cstdint>
#include <stdexcept>
struct RecoveredVectorCurveInfo {
 int32_t defaultKind=0,rightEdgeTime=0,leftKind=0,rightKind=0;
 bool useEdgeInfo=false,leftActive=false,rightActive=false;
 std::array<float,3> leftValue{},rightValue{},defaultEdgeZero{},disabledEdgeValue{};
};
struct RecoveredVectorCurveKey {int32_t time,kind;std::array<float,3> value;};
// 10507760: disabled edge uses a global vector; caller supplies its runtime value.
inline std::array<float,3> recoveredVectorEdgeValue(const RecoveredVectorCurveInfo&info,bool right){
 if(!info.useEdgeInfo)return info.disabledEdgeValue;
 if(right?info.rightActive:info.leftActive)return right?info.rightValue:info.leftValue;
 return info.defaultEdgeZero;
}
// 1051BE30: side activity gates kind independently of useEdgeInfo.
inline RecoveredVectorCurveKey gatherRecoveredVectorCurveKey(const std::vector<int32_t>&times,const std::vector<std::array<float,3>>&values,const std::vector<int32_t>&kinds,int32_t index,const RecoveredVectorCurveInfo&info,bool ownerPresent=true){
 if(!ownerPresent)return {0,0,{}};
 if(times.size()!=values.size())throw std::runtime_error("vector key count mismatch");
 if(index<0)return {0,info.leftActive&&info.leftKind?info.leftKind:info.defaultKind,recoveredVectorEdgeValue(info,false)};
 if(static_cast<size_t>(index)>=times.size()){
  int32_t time=info.rightEdgeTime;
  if(!time&&!times.empty()){uint32_t bits=static_cast<uint32_t>(times.back())+1;static_assert(sizeof(time)==sizeof(bits));std::memcpy(&time,&bits,4);}
  return {time,info.rightActive&&info.rightKind?info.rightKind:info.defaultKind,recoveredVectorEdgeValue(info,true)};
 }
 return {times[index],static_cast<size_t>(index)<kinds.size()&&kinds[index]?kinds[index]:info.defaultKind,values[index]};
}
