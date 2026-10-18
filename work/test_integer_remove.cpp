#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "remove_runtime_integer_array.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x73);auto layer=createRuntimeFloatLayer(r,m,id,4);uint32_t dirty=0;auto noop=[](unsigned,uint32_t,int32_t,int32_t){};auto suppressed=[](){return false;};
 for(int32_t v:{3,5,7})appendRuntimeIntegerArray(r,m,layer,"curvetypes",v,dirty,false,noop,suppressed);
 auto h=r.attributes.find(layer,"curvetypes");auto identity=r.attributes.identity(r.graph,r.elements,h);m.owners.at(layer).attributeFlags.at(identity)|=0x100;
 std::vector<std::array<int32_t,4>>events;auto notify=[&](unsigned slot,uint32_t attr,int32_t first,int32_t last){auto a=r.attributes.resolve(r.graph,r.elements,attr);events.push_back({static_cast<int32_t>(slot),first,last,static_cast<int32_t>(a->values.size())});};
 if(!removeRuntimeIntegerArray(r,m,layer,"curvetypes",1,dirty,false,notify,suppressed)||events!=std::vector<std::array<int32_t,4>>{{8,1,1,3},{0,0,0,2}})throw std::runtime_error("remove callbacks mismatch");
 auto a=r.attributes.resolve(r.graph,r.elements,h);if(a->values.size()!=2||wireInt(a->values[0])!=3||wireInt(a->values[1])!=7)throw std::runtime_error("tail shift mismatch");
 events.clear();dirty=0;if(removeRuntimeIntegerArray(r,m,layer,"curvetypes",-1,dirty,false,notify,suppressed)||removeRuntimeIntegerArray(r,m,layer,"curvetypes",2,dirty,false,notify,suppressed)||dirty||!events.empty())throw std::runtime_error("invalid index mutated");
 m.owners.at(layer).attributeFlags.at(identity)|=0x20;if(removeRuntimeIntegerArray(r,m,layer,"curvetypes",0,dirty,false,notify,suppressed)||a->values.size()!=2||dirty||!events.empty())throw std::runtime_error("remove gate failed");
 std::cout<<"integer_remove_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
