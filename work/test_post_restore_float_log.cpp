#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "runtime_float_layer_state.h"
#include "post_restore_float_log.h"
#include "float_log_runtime_metadata.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));RuntimeFloatLayerState state(r);size_t count=0;MutationCoordinator mutation;uint32_t dirty=0;size_t repairs=0,hooks=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLogLayer")state.construct(r.elements.graphHandles[i]);
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLog"){
 auto log=r.elements.graphHandles[i];auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));if(a->values.empty())continue;if(!bindFloatLogRuntimeMetadata(r,mutation,log,7))throw std::runtime_error("metadata failed"); for(auto&entry:mutation.owners.at(log).attributeFlags)entry.second|=0x80; count+=postRestoreFloatLog(r,mutation,state,log,dirty,[&](uint32_t){++repairs;},[&](uint32_t){++hooks;},[](){return false;}); if((mutation.owners.at(log).state&3)||mutation.owners.at(log).mask!=3)throw std::runtime_error("restore state mismatch"); for(auto entry:mutation.owners.at(log).attributeFlags)if(entry.second&0x80)throw std::runtime_error("restore flags not cleared");
 for(const auto&v:a->values)if(v.reference<r.graph.elements.size()&&r.graph.strings.at(r.graph.elements[v.reference].type)=="DmeFloatLogLayer"&&state.owner(r.elements.graphHandles[v.reference])!=log)throw std::runtime_error("owner rebind mismatch");
 }
 if(dirty!=0x1000||repairs!=hooks||!repairs)throw std::runtime_error("restore hooks mismatch"); if(!count)throw std::runtime_error("no owners rebound");std::cout<<"layer_owner_bindings="<<count<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

