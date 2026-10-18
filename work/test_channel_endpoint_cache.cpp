#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "channel_endpoint_cache.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));size_t checks=0,channels=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
  auto h=r.elements.graphHandles[i];++channels;
  for(const char*name:{"fromElement","fromAttribute","toElement","toAttribute","fromIndex","toIndex","mode","log"}){
   ChannelEndpointCache cache{123,456};auto a=r.attributes.find(h,name);bool source=std::string(name)=="fromElement"||std::string(name)=="fromAttribute";bool dest=std::string(name)=="toElement"||std::string(name)=="toAttribute";
   if(invalidateChannelEndpointCache(r,h,a,cache)!=(source||dest)||cache.source!=(source?0xffffffff:123)||cache.destination!=(dest?0xffffffff:456))throw std::runtime_error("cache invalidation mismatch");++checks;
  }
 }
 if(!channels)throw std::runtime_error("no channels");std::cout<<"channels="<<channels<<" cache_callback_checks="<<checks<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
