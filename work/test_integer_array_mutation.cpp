#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "mutate_runtime_integer_array.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x71);auto layer=createRuntimeFloatLayer(r,m,id,4);uint32_t dirty=0;
 auto h=r.attributes.find(layer,"curvetypes");auto identity=r.attributes.identity(r.graph,r.elements,h);m.owners.at(layer).attributeFlags.at(identity)|=0x100;
 std::vector<std::array<int32_t,4>>events;
 auto notify=[&](unsigned slot,uint32_t attr,int32_t first,int32_t last){auto a=r.attributes.resolve(r.graph,r.elements,attr);events.push_back({static_cast<int32_t>(slot),first,last,static_cast<int32_t>(a->values.size())});};auto suppressed=[](){return false;};
 if(!clearRuntimeIntegerArray(r,m,layer,"curvetypes",dirty,false,notify,suppressed)||events.size()!=2||events[0]!=std::array<int32_t,4>{8,0,-1,0})throw std::runtime_error("empty clear notification mismatch");
 events.clear();if(appendRuntimeIntegerArray(r,m,layer,"curvetypes",7,dirty,false,notify,suppressed)!=0||appendRuntimeIntegerArray(r,m,layer,"curvetypes",9,dirty,false,notify,suppressed)!=1||events.size()!=4||events[0]!=std::array<int32_t,4>{4,0,0,1}||events[2]!=std::array<int32_t,4>{4,1,1,2})throw std::runtime_error("append ordering mismatch");
 events.clear();if(!clearRuntimeIntegerArray(r,m,layer,"curvetypes",dirty,false,notify,suppressed)||events[0]!=std::array<int32_t,4>{8,0,1,2}||events[1]!=std::array<int32_t,4>{0,0,0,0}||!(dirty&0x4000)||m.owners.at(layer).mask!=2)throw std::runtime_error("clear side effects mismatch");
 events.clear();dirty=0;m.owners.at(layer).attributeFlags.at(identity)|=0x20;
 if(clearRuntimeIntegerArray(r,m,layer,"curvetypes",dirty,false,notify,suppressed)||appendRuntimeIntegerArray(r,m,layer,"curvetypes",8,dirty,false,notify,suppressed)!=-1||dirty||!events.empty())throw std::runtime_error("mutation gate failed");
 std::cout<<"integer_array_mutation_checks=4 callback_order=verified\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
