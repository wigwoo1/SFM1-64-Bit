#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "attach_mutating_float_layer.h"
#include "float_log_runtime_metadata.h"
#include "channel_runtime_metadata.h"
#include "session_mutation_dispatch.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("usage: input output");
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;uint32_t log=0xffffffff;size_t logIndex=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
  auto channel=r.elements.graphHandles[i];if(!bindChannelRuntimeMetadata(r,m,channel,4))throw std::runtime_error("channel metadata mismatch");
  auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(channel,"log"));auto ref=a->values.at(0).reference;
  if(log==0xffffffff&&ref<r.graph.elements.size()&&r.graph.strings.at(r.graph.elements[ref].type)=="DmeFloatLog"){log=r.elements.graphHandles[ref];logIndex=ref;}
 }
 if(log==0xffffffff||!bindFloatLogRuntimeMetadata(r,m,log,4))throw std::runtime_error("missing channel float log");
 std::vector<uint32_t> expected{log};
 for(auto a:r.references.incoming.at(log)){
  auto identity=r.attributes.identity(r.graph,r.elements,a);if(!identity)continue;auto record=reinterpret_cast<const SessionAttributeRecord*>(identity);auto owner=m.owners.find(record->ownerHandle);
  if(owner!=m.owners.end()){auto flag=owner->second.attributeFlags.find(identity);if(flag!=owner->second.attributeFlags.end()&&!(flag->second&0x1000)&&std::find(expected.begin(),expected.end(),record->ownerHandle)==expected.end())expected.push_back(record->ownerHandle);}
 }
 if(expected.size()<2)throw std::runtime_error("no incoming channel fixture");
 std::array<char,16>id{};id.fill(char(0x51));auto index=r.graph.elements.size();auto layer=createRuntimeFloatLayer(r,m,id,4);unsigned notifications=0;
 if(!attachMutatingFloatLayer(r,m,log,layer,false,[&](unsigned,uint32_t,size_t,size_t){++notifications;},[](){return false;}))throw std::runtime_error("attach failed");
 if(notifications!=2||m.queues.propagated!=expected||m.queues.marked!=std::vector<uint32_t>{log})throw std::runtime_error("channel propagation queue mismatch");
 for(auto h:expected)if(m.owners.at(h).mask!=(h==log?3u:1u)||!(m.owners.at(h).state&16))throw std::runtime_error("propagated masks mismatch");
 SessionMutationDispatch dispatch{r,m};unsigned marked=0;dispatch.processMarked([&](MutationOwner*){++marked;});dispatch.processPropagated();
 if(marked!=1||!m.queues.marked.empty()||!m.queues.propagated.empty())throw std::runtime_error("drain failed");
 for(auto h:expected)if(m.owners.at(h).mask&3||m.owners.at(h).state&16)throw std::runtime_error("propagation reset failed");
 saveGraph(r.graph,argv[2]);SessionRuntime reload(loadGraph(argv[2]));auto attr=reload.attributes.find(reload.elements.graphHandles.at(logIndex),"layers");
 if(reload.graph.elements[index].id!=id||reload.attributes.resolve(reload.graph,reload.elements,attr)->values.back().reference!=index||reload.references.occurrences(reload.elements.graphHandles.at(index),attr)!=1)throw std::runtime_error("reload failed");
 std::cout<<"propagated_owners="<<expected.size()<<" channel_owners="<<expected.size()-1<<" notifications=2 drain_reset=1 reload=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
