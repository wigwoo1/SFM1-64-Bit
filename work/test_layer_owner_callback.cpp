#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;RuntimeFloatLayerState state(r);std::array<char,16>a{},b{},c{},d{};a.fill(0x11);b.fill(0x12);c.fill(0x13);d.fill(0x14);
 auto log=createRuntimeFloatLog(r,m,a,b,4,&state);auto layer=createRuntimeFloatLayer(r,m,c,4);state.construct(layer);auto attr=r.attributes.find(log,"layers");unsigned calls=0;
 if(!attachMutatingFloatLayer(r,m,log,layer,false,[&](unsigned phase,uint32_t,size_t,size_t){++calls;if(state.owner(layer)!=log||r.references.occurrences(layer,attr)!=(phase==1?0u:1u))throw std::runtime_error("owner callback timing mismatch");},[](){return false;},&state)||calls!=2)throw std::runtime_error("owner notification not dispatched");
 auto quiet=createRuntimeFloatLayer(r,m,d,4);state.construct(quiet);m.owners.at(log).state&=0xfb;
 if(!attachMutatingFloatLayer(r,m,log,quiet,false,[&](unsigned,uint32_t,size_t,size_t){++calls;},[](){return false;},&state)||calls!=2||state.owner(quiet)!=0xffffffff)throw std::runtime_error("disabled callback ran");
 std::cout<<"owner_callback_timing_checks=2 notification_gate_checks=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
