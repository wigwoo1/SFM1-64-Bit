#pragma once
#include "session_expression_chain_plan.h"
#include "runtime_typed_channel_copy.h"
#include "equivalent_constant_channel_writers.h"
#include "pack_color_dependencies.h"
#include <chrono>
struct SessionExpressionChainPassResultCandidate {std::vector<uint32_t>executed;SessionAttributeChanges changes;double planMilliseconds=0,bindingMilliseconds=0,operationMilliseconds=0;};
// Isolated discovered-chain pass. Full native scheduling/activation is absent.
inline SessionExpressionChainPassResultCandidate executeSessionExpressionChainPassCandidate(SessionRuntime&runtime,const std::vector<SessionExpressionChainCandidate>&chains,int32_t rootTick,const std::map<uint32_t,std::vector<uint32_t>>&typedPaths={},const std::vector<uint32_t>&colorCopies={},const std::vector<uint32_t>&packColors={}){
 auto profileStart=std::chrono::steady_clock::now();auto plan=planSessionExpressionChainsCandidate(runtime.graph,chains);
 for(const auto&item:typedPaths){ChannelBinding binding(runtime.graph,item.first);if(binding.mode!=3||(binding.destination.scalarType!=3&&binding.destination.scalarType!=10&&binding.destination.scalarType!=13))throw std::runtime_error("unsupported additional typed channel");plan.operations.push_back(channelOperation(item.first,binding));}
 for(auto id:colorCopies){ChannelBinding binding(runtime.graph,id);if(binding.mode!=1||binding.source.scalarType!=8||binding.destination.scalarType!=8)throw std::runtime_error("unsupported additional color copy");plan.operations.push_back(channelOperation(id,binding));}
 for(auto id:packColors)plan.operations.push_back(packColorOperationCandidate(runtime.graph,id));
 if(!typedPaths.empty()||!colorCopies.empty()||!packColors.empty()){
  std::set<uint32_t>ids;std::map<AttributeIdentity,std::vector<uint32_t>>writers;plan.conflictingWriters.clear();
  for(const auto&operation:plan.operations){if(!ids.insert(operation.id).second)throw std::runtime_error("duplicate combined operation");for(const auto&output:operation.outputs)writers[output].push_back(operation.id);}
  for(const auto&item:writers)if(item.second.size()>1)plan.conflictingWriters.emplace(item);
  auto nodes=attributeDependencies(plan.operations);std::vector<uint32_t>roots;for(uint32_t i=0;i<nodes.size();++i)roots.push_back(i);plan.schedule=traverseSchedule(nodes,roots);
 }
 for(auto it=plan.conflictingWriters.begin();it!=plan.conflictingWriters.end();){if(equivalentConstantChannelWritersCandidate(runtime.graph,it->second))it=plan.conflictingWriters.erase(it);else ++it;}
 if(plan.schedule.cycle||!plan.conflictingWriters.empty())throw std::runtime_error("unresolved combined chain dependency policy");
 auto planEnd=std::chrono::steady_clock::now();
 std::map<uint32_t,std::vector<uint32_t>>playFrames;std::set<uint32_t>expressions,copies;
 for(const auto&chain:chains){auto inserted=playFrames.emplace(chain.play,chain.frames);if(!inserted.second&&inserted.first->second!=chain.frames)throw std::runtime_error("conflicting channel time paths");expressions.insert(chain.expression);copies.insert(chain.copy);}
 for(const auto&item:typedPaths)if(!playFrames.emplace(item).second)throw std::runtime_error("duplicate channel time path");
 copies.insert(colorCopies.begin(),colorCopies.end());
 std::map<uint32_t,RuntimeCachedFloatPlayBinding>plays;std::map<uint32_t,RuntimeCachedFloatCopyBinding>copyBindings;
 for(const auto&item:playFrames)plays.emplace(item.first,RuntimeCachedFloatPlayBinding(runtime.graph,item.first,runtime.elements,runtime.attributes));
 for(auto id:copies)copyBindings.emplace(id,RuntimeCachedFloatCopyBinding(runtime.graph,id,runtime.elements,runtime.attributes));
 std::set<uint32_t>packIds(packColors.begin(),packColors.end());
 SessionExpressionChainPassResultCandidate result;
 auto bindingEnd=std::chrono::steady_clock::now();
 for(auto id:plan.schedule.order){bool success=false;auto play=plays.find(id);
  if(play!=plays.end())success=executeRuntimeTypedPlay(runtime.graph,runtime.elements,runtime.attributes,play->second,channelLocalTime(runtime.graph,playFrames.at(id),rootTick,false),&result.changes);
  else if(expressions.count(id))success=executeRuntimeExpression(runtime.graph,runtime.elements,runtime.elements.graphHandles.at(id),&result.changes);
  else if(packIds.count(id)){if(runtime.elements.resolve(runtime.graph,runtime.elements.graphHandles.at(id)))success=executeSessionPackColorCandidate(runtime.graph,id,&result.changes);}
  else if(copies.count(id))success=executeRuntimeTypedCopy(runtime.graph,runtime.elements,runtime.attributes,copyBindings.at(id),&result.changes);
  if(!success)throw std::runtime_error("combined expression chain operation failed");result.executed.push_back(id);
 }auto operationEnd=std::chrono::steady_clock::now();auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};result.planMilliseconds=ms(profileStart,planEnd);result.bindingMilliseconds=ms(planEnd,bindingEnd);result.operationMilliseconds=ms(bindingEnd,operationEnd);return result;
}
