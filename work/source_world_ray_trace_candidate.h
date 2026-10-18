#pragma once
#include "source_swept_world_trace_candidate.h"
// Zero-extent world Ray_t dispatch. Swept-only API retains its refusal of a
// stationary query; callers opt into the reconstructed stationary path here.
// A native Ray_t carries this flag explicitly. Its constructor can classify a
// nonzero delta as stationary when float squared-length arithmetic underflows.
inline SourceSweptWorldTraceCandidate sourceWorldRayTraceWithSweptFlagCandidate(
 const SourcePreparedWorldRayCandidate&world,const std::array<float,3>&start,
 const std::array<float,3>&delta,uint32_t mask,bool swept,bool computePositions=true,bool worldAvailable=true){
 SourceSweptWorldTraceCandidate result;if(!worldAvailable)return result;
 if(!swept)world.traceStationaryPoint(start,mask,result.state);
 else world.traceRay(start,delta,mask,result.state);
 if(computePositions)sourceWorldTracePositionsCandidate(start,delta,result);return result;
}
inline SourceSweptWorldTraceCandidate sourceWorldRayTraceCandidate(
 const SourcePreparedWorldRayCandidate&world,const std::array<float,3>&start,
 const std::array<float,3>&delta,uint32_t mask,bool computePositions=true,bool worldAvailable=true){
 return sourceWorldRayTraceWithSweptFlagCandidate(world,start,delta,mask,delta!=std::array<float,3>{0,0,0},computePositions,worldAvailable);
}
