#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "channel_runtime_metadata.h"
#include "write_channel_reference.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
  auto h=r.elements.graphHandles[i];if(!bindChannelRuntimeMetadata(r,m,h,4))throw std::runtime_error("metadata mismatch");
  auto a=r.attributes.find(h,"fromElement");auto attr=r.attributes.resolve(r.graph,r.elements,a);auto old=attr->values[0].reference;if(old==0xfffffffe)continue;
  auto target=r.elements.graphHandles.at(old==0?1:0);ChannelEndpointCache cache{123,456};m.cycle.phase=3;
  if(writeChannelReference(r,m,h,"fromElement",target,cache,false,[](){return false;})||cache.source!=123)throw std::runtime_error("rejected write invalidated cache");
  m.cycle.phase=2;if(!writeChannelReference(r,m,h,"fromElement",target,cache,false,[](){return false;})||cache.source!=0xffffffff||cache.destination!=456||r.references.occurrences(target,a)!=1||m.owners.at(h).mask!=3)throw std::runtime_error("integrated write mismatch");
  cache.source=789;if(writeChannelReference(r,m,h,"fromElement",target,cache,false,[](){return false;})||cache.source!=789)throw std::runtime_error("unchanged write invalidated cache");
  std::cout<<"channel_reference_edit_checks=3\n";return 0;
 }throw std::runtime_error("no fixture");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
