#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "channel_runtime_metadata.h"
#include "write_channel_reference.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;uint32_t channel=0xffffffff;std::map<std::string,uint32_t>targets;
 for(size_t i=0;i<r.graph.elements.size();++i){auto type=r.graph.strings.at(r.graph.elements[i].type);if(recoveredLogClass(type))targets.emplace(type,r.elements.graphHandles[i]);if(channel==0xffffffff&&type=="DmeChannel")channel=r.elements.graphHandles[i];}
 if(channel==0xffffffff||targets.empty()||!bindChannelRuntimeMetadata(r,m,channel,4))throw std::runtime_error("missing typed log fixture");
 auto a=r.attributes.find(channel,"log");auto attr=r.attributes.resolve(r.graph,r.elements,a);auto original=attr->values[0].reference;ChannelEndpointCache cache{123,456};size_t edits=0;
 auto write=[&](uint32_t target){return writeChannelReference(r,m,channel,"log",target,cache,false,[](){return false;});};
 if(original!=0xffffffff&&!write(0xffffffff))throw std::runtime_error("initial clear failed");
 for(const auto&entry:targets){auto before=r.references.occurrences(entry.second,a);
  if(!write(entry.second)||r.references.occurrences(entry.second,a)!=before+1||cache.source!=123||cache.destination!=456||m.owners.at(channel).mask!=3)throw std::runtime_error("typed log write failed");
  if(!write(0xffffffff)||r.references.occurrences(entry.second,a)!=before)throw std::runtime_error("typed log release failed");
  edits+=2;std::cout<<"accepted_class="<<entry.first<<'\n';
 }
 if(original!=0xffffffff&&!write(r.elements.graphHandles.at(original)))throw std::runtime_error("restore failed");
 if(attr->values[0].reference!=original||m.queues.marked!=std::vector<uint32_t>{channel}||m.queues.propagated!=std::vector<uint32_t>{channel})throw std::runtime_error("final state mismatch");
 validateGraph(r.graph);std::cout<<"typed_classes="<<targets.size()<<" assignment_release_checks="<<edits<<" restored_original=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
