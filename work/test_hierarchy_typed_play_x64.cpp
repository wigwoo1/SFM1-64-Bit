#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_discovery.h"
#include "runtime_typed_channel_play.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;SessionRuntime runtime(loadGraph(argv[1]));auto original=runtime.graph;auto roots=hierarchyRefs(original,original.elements.at(0),"activeClip",1);auto paths=channelTimePaths(original,roots.at(0));size_t channels=0,passed=0,rejected=0;std::map<std::string,size_t>unsupported;
 for(const auto&item:paths){ChannelBinding b(runtime.graph,item.first);if(b.mode!=3||!b.destination.present()||(b.destination.scalarType!=10&&b.destination.scalarType!=13))continue;++channels;
  if(item.second.size()!=1){++unsupported["ambiguous time path"];continue;}
  auto*destination=runtimeAttribute(runtime.graph,b.destination.element,b.destination.attribute);auto saved=*destination;RuntimeCachedTypedPlayBinding live(runtime.graph,item.first,runtime.elements,runtime.attributes);
  for(int32_t tick:{-10000,0,10000}){SessionAttributeChanges changes;auto local=channelLocalTime(runtime.graph,item.second.at(0),tick,false);
   try{bool success=executeRuntimeTypedPlay(runtime.graph,runtime.elements,runtime.attributes,live,local,&changes);if(!success){++rejected;std::string reason="type="+std::to_string(b.destination.scalarType)+" layers="+std::to_string(b.layers.size());if(b.log!=0xffffffff)reason+=" curved="+std::to_string(!hierarchyRefs(runtime.graph,runtime.graph.elements.at(b.log),"curveinfo",1).empty());++unsupported[reason];if(!changes.attributes.empty()||destination->values.at(b.destination.index).wire!=saved.values.at(b.destination.index).wire)throw std::runtime_error("rejected typed play mutated");}
    else{auto actual=*destination;*destination=saved;SessionAttributeChanges direct;bool ok=b.destination.scalarType==10?executeSessionVectorPlay(runtime.graph,b,local,&direct):executeSessionQuaternionPlay(runtime.graph,b,local,&direct);if(!ok||destination->values.at(b.destination.index).wire!=actual.values.at(b.destination.index).wire)throw std::runtime_error("typed direct mismatch");++passed;}
   }catch(const std::exception&e){++unsupported[std::string("exception=")+e.what()];throw;}
   *destination=saved;
  }
 }
 if(!graphExactEqualCandidate(original,runtime.graph))throw std::runtime_error("unreported typed mutation");std::cout<<"typed_channels="<<channels<<" samples_passed="<<passed<<" samples_rejected="<<rejected<<" graph_restored_exact=passed\n";for(const auto&item:unsupported)std::cout<<"count="<<item.second<<" reason="<<item.first<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
