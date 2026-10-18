#pragma once
#include "session_expression_chain_pass.h"
#include <chrono>
struct SessionColorTimelineSubsetResultCandidate{Graph graph;size_t chains,typedChannels,executed,changed;double discoveryMilliseconds=0,runtimeMilliseconds=0,additionalBindingsMilliseconds=0,passMilliseconds=0,planMilliseconds=0,bindingMilliseconds=0,operationMilliseconds=0,elementSetupMilliseconds=0,attributeSetupMilliseconds=0,referenceSetupMilliseconds=0;};
// Same recovered subset as evaluate-color-subset; native activation unfinished.
inline SessionColorTimelineSubsetResultCandidate evaluateSessionColorTimelineSubsetCandidate(Graph graph,int32_t tick,uint32_t root){
 auto start=std::chrono::steady_clock::now();auto discovery=discoverSessionExpressionChainsCandidate(graph,root);
 if(!discovery.invalidChannels.empty()||discovery.ambiguousPaths)throw std::runtime_error("chain discovery has unresolved bindings or time paths");
 auto discoveryEnd=std::chrono::steady_clock::now();SessionRuntime runtime(std::move(graph));auto runtimeEnd=std::chrono::steady_clock::now();auto typed=discoverSessionAdditionalPlayPathsCandidate(runtime.graph,root,discovery.chains);auto colors=discoverSessionColorCopyChannelsCandidate(runtime.graph,root);std::set<uint32_t>packSet;
 for(auto id:colors){ChannelBinding binding(runtime.graph,id);if(runtime.graph.strings.at(runtime.graph.elements.at(binding.source.element).type)!="DmePackColorOperator")throw std::runtime_error("color copy requires unsupported source operator");packSet.insert(binding.source.element);}
 std::vector<uint32_t>packs(packSet.begin(),packSet.end());auto additionalEnd=std::chrono::steady_clock::now();auto result=executeSessionExpressionChainPassCandidate(runtime,discovery.chains,tick,typed,colors,packs);auto passEnd=std::chrono::steady_clock::now();auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
 return {std::move(runtime.graph),discovery.chains.size(),typed.size(),result.executed.size(),result.changes.attributes.size(),ms(start,discoveryEnd),ms(discoveryEnd,runtimeEnd),ms(runtimeEnd,additionalEnd),ms(additionalEnd,passEnd),result.planMilliseconds,result.bindingMilliseconds,result.operationMilliseconds,ms(runtime.setupStarted,runtime.elementsReady),ms(runtime.elementsReady,runtime.attributesReady),ms(runtime.attributesReady,runtime.referencesReady)};
}
