#pragma once
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "schedule_traversal.h"
#include "channel_dependencies.h"
#include "expression_dependencies.h"
#include "session_expression_runtime.h"
#include "channel_clip_time.h"
#include "session_change_scheduler.h"
#include "runtime_channel_play.h"
#include "runtime_expression.h"
#include "session_runtime.h"
// Explicitly selected float play -> expression -> float copy chain. Full native
// operator discovery, phase policy and timeline scheduling remain separate.
class SessionFloatExpressionPlaybackCandidate {
 SessionRuntime& runtime;uint32_t playId,expressionId,copyId,expressionHandle;
 std::vector<uint32_t> frames;SessionChangeScheduler scheduler;
 RuntimeCachedFloatPlayBinding play;RuntimeCachedFloatCopyBinding copy;
public:
 SessionFloatExpressionPlaybackCandidate(SessionRuntime&r,uint32_t p,uint32_t e,uint32_t c,std::vector<uint32_t>timeFrames):runtime(r),playId(p),expressionId(e),copyId(c),expressionHandle(r.elements.graphHandles.at(e)),frames(std::move(timeFrames)),scheduler({channelOperation(c,ChannelBinding(r.graph,c)),expressionOperation(r.graph,e),channelOperation(p,ChannelBinding(r.graph,p))}),play(r.graph,p,r.elements,r.attributes),copy(r.graph,c,r.elements,r.attributes){
  ChannelBinding pb(r.graph,p),cb(r.graph,c);
  if(pb.mode!=3||pb.destination.element!=e||pb.destination.attribute!="value"||pb.destination.scalarType!=3||cb.mode!=1||cb.source.element!=e||cb.source.attribute!="result"||cb.source.scalarType!=3||cb.destination.scalarType!=3)throw std::runtime_error("unsupported float expression playback chain");
 }
 std::vector<uint32_t> step(int32_t rootTick,bool clamp){
  // Validate required live elements before any chain mutation. Optional play
  // source remains governed by the native candidate fallback in its executor.
  for(auto handle:{play.channelHandle,play.destinationHandle,play.logHandle,copy.channelHandle,copy.sourceHandle,copy.destinationHandle,expressionHandle})if(!runtime.elements.resolve(runtime.graph,handle))throw std::runtime_error("stale playback chain element");
  for(auto handle:play.layerHandles)if(!runtime.elements.resolve(runtime.graph,handle))throw std::runtime_error("stale playback log layer");
  for(auto frame:frames)if(!runtime.elements.resolve(runtime.graph,runtime.elements.graphHandles.at(frame)))throw std::runtime_error("stale playback time frame");
  if(!cachedPlayMetadataCurrent(runtime.graph,play)||!runtimeChannelSnapshotCurrent(runtime.graph,copy))throw std::runtime_error("playback chain metadata changed");
  auto requireEndpoint=[&](uint32_t owner,const ChannelEndpoint&endpoint){
   auto attribute=runtime.attributes.resolve(runtime.graph,runtime.elements,runtime.attributes.find(owner,endpoint.attribute));
   if(!attribute||(attribute->type!=3&&attribute->type!=17)||endpoint.index<0||size_t(endpoint.index)>=attribute->values.size()||attribute->values.at(endpoint.index).wire.size()!=4)throw std::runtime_error("invalid playback chain attribute");
  };
  requireEndpoint(play.destinationHandle,play.saved.destination);requireEndpoint(copy.sourceHandle,copy.saved.source);requireEndpoint(copy.destinationHandle,copy.saved.destination);
  auto*formula=runtimeAttribute(runtime.graph,expressionId,"expr");auto*result=runtimeAttribute(runtime.graph,expressionId,"result");
  if(!formula||formula->type!=5||formula->values.size()!=1||!result||result->type!=3||result->values.size()!=1)throw std::runtime_error("invalid playback expression schema");
  const auto local=channelLocalTime(runtime.graph,frames,rootTick,clamp);
  return scheduler.step({playId},{},[&](uint32_t id,SessionAttributeChanges&changes){
   if(id==playId)return executeRuntimeFloatPlay(runtime.graph,runtime.elements,runtime.attributes,play,local,&changes);
   if(id==expressionId)return executeRuntimeExpression(runtime.graph,runtime.elements,expressionHandle,&changes);
   if(id==copyId)return executeRuntimeFloatCopy(runtime.graph,runtime.elements,runtime.attributes,copy,&changes);
   return false;
  });
 }
};
