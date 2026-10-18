#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_pass.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2&&argc!=3)return 2;auto base=loadGraph(argv[1]);auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(base,roots.at(0));auto typed=discoverSessionAdditionalPlayPathsCandidate(base,roots.at(0),discovery.chains);if(base.elements.size()>25923){SessionRuntime negative(base);ChannelBinding conflict(negative.graph,18239);auto*value=runtimeAttribute(negative.graph,conflict.log,"defaultvalue");setFloat(*value,0,1.0f);auto before=negative.graph;bool rejected=false;try{executeSessionExpressionChainPassCandidate(negative,discovery.chains,0,typed);}catch(const std::runtime_error&){rejected=true;}if(!rejected||!graphExactEqualCandidate(before,negative.graph))throw std::runtime_error("unequal writer guard failed");}size_t executions=0;
 for(int32_t tick:{-10000,0,10000}){SessionRuntime combined(base),independent(base);auto result=executeSessionExpressionChainPassCandidate(combined,discovery.chains,tick,typed);executions+=result.executed.size();if(argc==3&&tick==0&&!graphExactEqualCandidate(combined.graph,loadGraph(argv[2])))throw std::runtime_error("saved CLI combined graph mismatch");
  for(const auto&item:typed){RuntimeCachedTypedPlayBinding binding(independent.graph,item.first,independent.elements,independent.attributes);if(!executeRuntimeTypedPlay(independent.graph,independent.elements,independent.attributes,binding,channelLocalTime(independent.graph,item.second,tick,false)))throw std::runtime_error("independent typed play failed");}
  for(const auto&chain:discovery.chains){SessionFloatExpressionPlaybackCandidate adapter(independent,chain.play,chain.expression,chain.copy,chain.frames);adapter.step(tick,false);}
  if(!graphExactEqualCandidate(combined.graph,independent.graph))throw std::runtime_error("combined graph differs from independent chains");
  auto stable=combined.graph;auto repeated=executeSessionExpressionChainPassCandidate(combined,discovery.chains,tick,typed);if(!repeated.changes.attributes.empty()||!graphExactEqualCandidate(stable,combined.graph))throw std::runtime_error("repeat pass changed graph");
  for(const auto&changed:result.changes.attributes)*runtimeAttribute(combined.graph,changed.first,changed.second)=*runtimeAttribute(base,changed.first,changed.second);
  if(!graphExactEqualCandidate(base,combined.graph))throw std::runtime_error("unreported combined change");
 }
 std::cout<<"combined_scalar_vector_quaternion_chains="<<discovery.chains.size()<<" executions="<<executions<<" samples=3 full_graph_independent_repeat_preservation=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
