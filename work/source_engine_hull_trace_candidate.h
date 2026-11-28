#pragma once
#include "source_engine_point_trace_candidate.h"
#include "source_server_hull_ray_candidate.h"

// Outer1016ece0 over prepared services. Mode0=Everything,1=world only,
// 2=entities only,3=Everything with static props passed to the predicate.
// Partition order, world annotation and real clipping belong to the host.
template<class Services,class Predicate>
SourcePhysicsSearchTraceCandidate sourceEngineHullTraceCandidate(
 const SourceServerHullRayCandidate& ray,uint32_t mask,unsigned mode,Services& services,Predicate&& predicate){
 if(mode>3)throw std::runtime_error("unsupported engine trace mode");
 SourcePhysicsSearchTraceCandidate result;
 if(mode==2){for(size_t a=0;a<3;++a){result.start[a]=ray.offset[a]+ray.center[a];result.end[a]=result.start[a]+ray.delta[a];}}
 else{
  result=services.traceWorld(ray,mask);
  if(result.startSolid||mode==1)return result;
 }
 const float scale=result.fraction;float exitScale=scale;auto shortRay=ray;
 if(scale==0){shortRay.delta={0,0,0};exitScale=result.fractionLeftSolid;result.fractionLeftSolid=1;}
 else{
  for(size_t a=0;a<3;++a){const float end=ray.delta[a]*scale+ray.center[a];shortRay.delta[a]=end-ray.center[a];}
  result.fractionLeftSolid/=scale;
 }
 result.fraction=1;
 const auto list=services.enumerateCandidates(shortRay);const size_t count=std::min(size_t(1024),list.size());
 if(list.size()>count)services.candidateOverflow(list.size()-count);
 for(size_t i=0;i<count;++i){
  const auto entity=list[i];const auto collideable=services.resolveCollideable(entity);
  const bool prop=sourceStaticPropEntityCandidate(entity,services);
  if(prop&&mode==2)continue;
  if((!prop||mode==3)&&!predicate(entity,mask))continue;
  const auto hit=services.clipToCollideable(shortRay,mask,collideable);
  sourceEngineMergeTraceCandidate(hit,result);if(result.allSolid)break;
 }
 result.fraction=scale*result.fraction;result.fractionLeftSolid=exitScale*result.fractionLeftSolid;
 if(!ray.isRay){for(size_t a=0;a<3;++a)result.start[a]=ray.offset[a]+ray.center[a];result.fractionLeftSolid=0;}
 return result;
}
