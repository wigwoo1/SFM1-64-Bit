#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "runtime_float_layer_state.h"
#include "dispatch_float_log_owner_callbacks.h"
#include "float_log_runtime_metadata.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));RuntimeFloatLayerState state(r);size_t count=0;MutationCoordinator mutation;SessionMutationDispatch dispatch{r,mutation};
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLogLayer")state.construct(r.elements.graphHandles[i]);
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLog"){
 auto log=r.elements.graphHandles[i];auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));if(a->values.empty())continue;if(!bindFloatLogRuntimeMetadata(r,mutation,log,4))throw std::runtime_error("metadata failed");mutation.queues.mark(mutation.owners.at(log).state,log);
 } count=dispatchMarkedFloatLogOwners(dispatch,state,[](uint32_t){throw std::runtime_error("unexpected non-log callback");}); if(!mutation.queues.marked.empty())throw std::runtime_error("marked queue not cleared"); for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLog"){auto log=r.elements.graphHandles[i];auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers")); for(const auto&v:a->values)if(v.reference<r.graph.elements.size()&&r.graph.strings.at(r.graph.elements[v.reference].type)=="DmeFloatLogLayer"&&state.owner(r.elements.graphHandles[v.reference])!=log)throw std::runtime_error("owner rebind mismatch");
 }
 if(!count)throw std::runtime_error("no owners rebound");std::cout<<"layer_owner_bindings="<<count<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

