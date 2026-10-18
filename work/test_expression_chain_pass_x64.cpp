#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_pass.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2&&argc!=3)return 2;auto base=loadGraph(argv[1]);auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(base,roots.at(0));size_t executions=0;
 for(int32_t tick:{-10000,0,10000}){SessionRuntime combined(base),independent(base);auto result=executeSessionExpressionChainPassCandidate(combined,discovery.chains,tick);executions+=result.executed.size();if(argc==3&&tick==0&&!graphExactEqualCandidate(combined.graph,loadGraph(argv[2])))throw std::runtime_error("saved CLI combined graph mismatch");
  for(const auto&chain:discovery.chains){SessionFloatExpressionPlaybackCandidate adapter(independent,chain.play,chain.expression,chain.copy,chain.frames);adapter.step(tick,false);}
  if(!graphExactEqualCandidate(combined.graph,independent.graph))throw std::runtime_error("combined graph differs from independent chains");
  auto stable=combined.graph;auto repeated=executeSessionExpressionChainPassCandidate(combined,discovery.chains,tick);if(!repeated.changes.attributes.empty()||!graphExactEqualCandidate(stable,combined.graph))throw std::runtime_error("repeat pass changed graph");
  for(const auto&changed:result.changes.attributes)*runtimeAttribute(combined.graph,changed.first,changed.second)=*runtimeAttribute(base,changed.first,changed.second);
  if(!graphExactEqualCandidate(base,combined.graph))throw std::runtime_error("unreported combined change");
 }
 std::cout<<"combined_chains="<<discovery.chains.size()<<" executions="<<executions<<" samples=3 full_graph_independent_repeat_preservation=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
