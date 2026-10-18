#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
#include "channel_runtime_metadata.h"
#include "write_channel_reference.h"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;size_t channelIndex=r.graph.elements.size();
 for(uint32_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
  ChannelBinding b(r.graph,i);if(b.mode==3&&b.destination.present()&&b.destination.scalarType==3){channelIndex=i;break;}
 }
 if(channelIndex==r.graph.elements.size())throw std::runtime_error("no float play channel");auto channel=r.elements.graphHandles[channelIndex];if(!bindChannelRuntimeMetadata(r,m,channel,4))throw std::runtime_error("channel metadata failed");
 auto index=r.graph.elements.size();std::array<char,16>log{},layer{};log.fill(char(0x71));layer.fill(char(0x72));auto created=createRuntimeFloatLog(r,m,log,layer,4);ChannelEndpointCache cache{123,456};
 if(!writeChannelReference(r,m,channel,"log",created,cache,false,[](){return false;})||cache.source!=123||cache.destination!=456)throw std::runtime_error("log assignment failed");
 auto reference=r.attributes.find(channel,"log");auto layers=r.attributes.find(created,"layers");if(r.references.occurrences(created,reference)!=1||r.references.occurrences(r.elements.graphHandles[index+1],layers)!=1)throw std::runtime_error("created chain ledger mismatch");
 ChannelBinding binding(r.graph,static_cast<uint32_t>(channelIndex));if(binding.log!=index||binding.layers!=std::vector<uint32_t>{static_cast<uint32_t>(index+1)})throw std::runtime_error("created binding mismatch");
 if(!executeSessionFloatPlay(r.graph,binding,0,nullptr,true)||wireFloat(runtimeAttribute(r.graph,binding.destination.element,binding.destination.attribute)->values.at(binding.destination.index))!=0.0f)throw std::runtime_error("created empty log playback failed");
 saveGraph(r.graph,argv[2]);SessionRuntime reload(loadGraph(argv[2]));ChannelBinding restored(reload.graph,static_cast<uint32_t>(channelIndex));
 if(restored.log!=index||restored.layers.size()!=1||reload.graph.elements[index].id!=log||reload.graph.elements[index+1].id!=layer||!executeSessionFloatPlay(reload.graph,restored,0,nullptr,true))throw std::runtime_error("restored creation playback failed");
 if(wireFloat(runtimeAttribute(reload.graph,restored.destination.element,restored.destination.attribute)->values.at(restored.destination.index))!=0.0f)throw std::runtime_error("restored default mismatch");
 std::cout<<"created_channel_log_chain=1 forced_empty_play=1 reload_play=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

