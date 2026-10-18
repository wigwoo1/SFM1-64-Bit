#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_discovery.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8);if(argc!=2)return 2;SessionRuntime runtime(loadGraph(argv[1]));auto original=runtime.graph;
 auto roots=hierarchyRefs(runtime.graph,runtime.graph.elements.at(0),"activeClip",1);if(roots.size()!=1)throw std::runtime_error("active clip required");
 auto discovered=discoverSessionExpressionChainsCandidate(runtime.graph,roots[0]);size_t passed=0,failed=0;std::map<std::string,size_t>failures;
 for(const auto&chain:discovered.chains){ChannelBinding copy(runtime.graph,chain.copy);
  std::map<AttributeIdentity,Attribute>saved;
  for(auto endpoint:std::vector<AttributeIdentity>{{chain.expression,"value"},{chain.expression,"result"},{copy.destination.element,copy.destination.attribute}}){auto*a=runtimeAttribute(runtime.graph,endpoint.first,endpoint.second);if(!a)throw std::runtime_error("missing discovered write");saved.emplace(endpoint,*a);}
  for(int32_t tick:{-10000,0,10000}){
   try{SessionFloatExpressionPlaybackCandidate adapter(runtime,chain.play,chain.expression,chain.copy,chain.frames);auto executed=adapter.step(tick,false);if(executed.empty()||executed.front()!=chain.play)throw std::runtime_error("source omitted");
    // Compare with direct execution from the same original attribute state.
    std::map<AttributeIdentity,Attribute>actual;for(const auto&item:saved){auto*a=runtimeAttribute(runtime.graph,item.first.first,item.first.second);actual.emplace(item.first,*a);*a=item.second;}
    ChannelBinding play(runtime.graph,chain.play);SessionAttributeChanges changes;auto local=channelLocalTime(runtime.graph,chain.frames,tick,false);
    if(!executeSessionFloatPlay(runtime.graph,play,local,&changes)||!executeSessionExpression(runtime.graph,chain.expression,&changes)||!executeSessionFloatCopy(runtime.graph,copy,&changes))throw std::runtime_error("direct chain failed");
    for(const auto&item:actual){const auto*a=runtimeAttribute(runtime.graph,item.first.first,item.first.second);if(a->values.size()!=item.second.values.size())throw std::runtime_error("cardinality mismatch");for(size_t i=0;i<a->values.size();++i)if(a->values[i].wire!=item.second.values[i].wire)throw std::runtime_error("direct payload mismatch");}++passed;
   }catch(const std::exception&e){++failed;++failures[e.what()];}
   for(const auto&item:saved)*runtimeAttribute(runtime.graph,item.first.first,item.first.second)=item.second;
  }
 }
 if(!graphExactEqualCandidate(original,runtime.graph))throw std::runtime_error("unreported mutation survived restoration");
 std::cout<<"candidate_chains="<<discovered.chains.size()<<" samples_passed="<<passed<<" samples_failed="<<failed<<" restored_graph_exact=passed\n";for(const auto&item:failures)std::cout<<"failure_count="<<item.second<<" reason="<<item.first<<'\n';
 return failed?1:0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}}
