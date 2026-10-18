#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "channel_runtime_metadata.h"
#include "write_channel_reference.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
 auto h=r.elements.graphHandles[i];auto a=r.attributes.find(h,"log");auto attr=r.attributes.resolve(r.graph,r.elements,a);auto old=attr->values[0].reference;
 if(old>=r.graph.elements.size()||r.graph.strings.at(r.graph.elements[old].type)!="DmeFloatLog")continue;
 if(!bindChannelRuntimeMetadata(r,m,h,4))throw std::runtime_error("metadata failed");auto ledger=r.references.incoming;ChannelEndpointCache cache{123,456};
 if(writeChannelReference(r,m,h,"log",h,cache,false,[](){return false;})||ledger!=r.references.incoming||attr->values[0].reference!=old||!m.queues.marked.empty()||cache.source!=123||cache.destination!=456)throw std::runtime_error("incompatible log target mutated state");
 if(!writeChannelReference(r,m,h,"log",0xffffffff,cache,false,[](){return false;})||cache.source!=123||cache.destination!=456)throw std::runtime_error("null log write failed");
 if(!writeChannelReference(r,m,h,"log",r.elements.graphHandles[old],cache,false,[](){return false;})||r.references.occurrences(r.elements.graphHandles[old],a)!=1||cache.source!=123||cache.destination!=456)throw std::runtime_error("float log target failed");
 std::cout<<"channel_log_type_checks=3\n";return 0;
 }throw std::runtime_error("no float log channel");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
