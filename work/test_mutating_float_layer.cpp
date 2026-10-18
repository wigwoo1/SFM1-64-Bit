#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "attach_mutating_float_layer.h"
#include "float_log_runtime_metadata.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("usage: input output");
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;uint32_t log=0xffffffff;size_t logIndex=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLog"){log=r.elements.graphHandles[i];logIndex=i;break;}
 if(log==0xffffffff)throw std::runtime_error("no float log fixture");
 if(!bindFloatLogRuntimeMetadata(r,m,log,4))throw std::runtime_error("metadata registration failed"); unsigned notifications=0; auto attach=[&](uint32_t owner,uint32_t target){return attachMutatingFloatLayer(r,m,owner,target,false,[&](unsigned,uint32_t,size_t,size_t){++notifications;},[](){return false;});}; auto layers=r.attributes.find(log,"layers");auto before=r.attributes.resolve(r.graph,r.elements,layers)->values;
 std::array<char,16> id{};id.fill(char(0x39));auto index=r.graph.elements.size();auto layer=createRuntimeFloatLayer(r,m,id,4);
 if(attach(layer,log)||attach(0xffffffff,layer))throw std::runtime_error("invalid attachment accepted");
 for(unsigned n=0;n<2;++n)if(!attach(log,layer))throw std::runtime_error("attachment failed");
 auto after=r.attributes.resolve(r.graph,r.elements,layers);
 if(after->values.size()!=before.size()+2||r.references.occurrences(layer,layers)!=2)throw std::runtime_error("duplicate occurrence mismatch");
 for(size_t i=0;i<before.size();++i)if(before[i].reference!=after->values[i].reference)throw std::runtime_error("old layer order changed");
 if(notifications!=4||m.queues.marked!=std::vector<uint32_t>{log}||m.queues.propagated!=std::vector<uint32_t>{log}||m.owners.at(log).mask!=3)throw std::runtime_error("integrated mutation/propagation mismatch"); saveGraph(r.graph,argv[2]);SessionRuntime copy(loadGraph(argv[2]));auto reloadedLog=copy.elements.graphHandles.at(logIndex);auto reloadedLayer=copy.elements.graphHandles.at(index);auto attr=copy.attributes.find(reloadedLog,"layers");auto values=copy.attributes.resolve(copy.graph,copy.elements,attr);
 if(copy.graph.elements[index].id!=id||values->values.size()!=before.size()+2||values->values[before.size()].reference!=index||values->values.back().reference!=index||copy.references.occurrences(reloadedLayer,attr)!=2)throw std::runtime_error("attached layer reload mismatch");
 std::cout<<"attachment_checks=7 appended_occurrences=2 reload_ledger_occurrences=2\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

