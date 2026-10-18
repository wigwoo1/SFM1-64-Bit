#pragma once
#include <cstdint>
struct StudioDynamicMeshDrawRangeCandidate{uint32_t first=0,count=0;};
// Logical1000a137..1000a17c after preparation succeeds. Caller supplies the
// recovered effective base index; override count is queried only for fullrange.
template<class OverrideCount>
StudioDynamicMeshDrawRangeCandidate studioDynamicMeshDrawRangeCandidate(int32_t first,int32_t count,uint32_t baseIndex,bool indexOverride,uint32_t primitive,uint32_t vertices,uint32_t indices,OverrideCount overrideCount){
 if(first!=-1&&count!=0)return {uint32_t(first)+baseIndex,uint32_t(count)};
 uint32_t resolvedCount;
 if(indexOverride)resolvedCount=overrideCount();
 else resolvedCount=(!primitive||primitive==10)?vertices:indices;
 return {baseIndex,resolvedCount};
}
