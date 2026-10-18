#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_discovery.h"
#include "runtime_typed_channel_copy.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;SessionRuntime runtime(loadGraph(argv[1]));auto base=runtime.graph;auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);size_t channels=0,writes=0;
 for(const auto&item:channelTimePaths(base,roots.at(0))){ChannelBinding b(runtime.graph,item.first);if(b.mode!=1||b.destination.scalarType!=8)continue;++channels;if(b.source.scalarType!=8)throw std::runtime_error("color conversion required");RuntimeCachedTypedCopyBinding live(runtime.graph,item.first,runtime.elements,runtime.attributes);auto*source=runtimeAttribute(runtime.graph,b.source.element,b.source.attribute);auto*destination=runtimeAttribute(runtime.graph,b.destination.element,b.destination.attribute);auto oldSource=*source,oldDestination=*destination;
  for(uint32_t word:{0u,0xff000000u,0x12345678u,0xffffffffu}){setWireU32(source->values.at(b.source.index),word);setWireU32(destination->values.at(b.destination.index),word^0xffffffffu);SessionAttributeChanges changes;
   if(!executeRuntimeTypedCopy(runtime.graph,runtime.elements,runtime.attributes,live,&changes)||destination->values.at(b.destination.index).wire!=source->values.at(b.source.index).wire||changes.attributes!=std::vector<AttributeIdentity>{{b.destination.element,b.destination.attribute}})throw std::runtime_error("color copy mismatch");
   SessionAttributeChanges repeat;if(!executeRuntimeTypedCopy(runtime.graph,runtime.elements,runtime.attributes,live,&repeat)||!repeat.attributes.empty())throw std::runtime_error("equal color changed");++writes;
  }*source=oldSource;*destination=oldDestination;
 }
 if(!channels||!graphExactEqualCandidate(base,runtime.graph))throw std::runtime_error("color fixture restoration failed");std::cout<<"color_channels="<<channels<<" packed_write_checks="<<writes<<" equal_journal_full_graph_restore=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
