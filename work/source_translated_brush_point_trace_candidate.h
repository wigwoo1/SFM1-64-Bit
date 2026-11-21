#pragma once
#include "source_swept_world_trace_candidate.h"

// Zero-angle point branch of the engine's transformed BSP trace. The origin
// supplied by the collideable is subtracted from the start only. The original
// delta and swept flag survive the transformation. Results use world positions.
inline SourceSweptWorldTraceCandidate sourceTranslatedBrushPointTraceCandidate(
 const SourcePreparedWorldRayCandidate& world,size_t modelIndex,
 const std::array<float,3>& origin,const std::array<float,3>& start,
 const std::array<float,3>& delta,uint32_t mask,bool swept){
 const int32_t root=world.modelRoot(modelIndex);
 std::array<float,3> localStart{};
 for(size_t axis=0;axis<localStart.size();++axis)localStart[axis]=start[axis]-origin[axis];
 SourceSweptWorldTraceCandidate result;
 if(swept)world.traceSweptPointAtRoot(root,localStart,delta,mask,result.state);
 else world.traceStationaryPointAtRoot(root,localStart,mask,result.state);
 sourceWorldTracePositionsCandidate(start,delta,result);
 return result;
}
