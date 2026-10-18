#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "integer_append_undo_record.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;std::array<char,16>id{};id.fill(0x74);auto layer=createRuntimeFloatLayer(r,m,id,4);uint32_t dirty=0;auto noop=[](unsigned,uint32_t,int32_t,int32_t){};auto suppressed=[](){return false;};
 appendRuntimeIntegerArray(r,m,layer,"curvetypes",3,dirty,false,noop,suppressed);appendRuntimeIntegerArray(r,m,layer,"curvetypes",7,dirty,false,noop,suppressed);
 RuntimeIntegerAppendRecord record{layer,"curvetypes",1,-5};auto h=r.attributes.find(layer,"curvetypes");auto identity=r.attributes.identity(r.graph,r.elements,h);m.owners.at(layer).attributeFlags.at(identity)|=0x100;
 std::vector<unsigned>slots;auto notify=[&](unsigned slot,uint32_t,int32_t,int32_t){slots.push_back(slot);};
 if(!replayIntegerAppendRecord(r,m,record,false,dirty,false,notify,suppressed))throw std::runtime_error("forward insertion failed");auto a=r.attributes.resolve(r.graph,r.elements,h);if(a->values.size()!=3||wireInt(a->values[0])!=3||wireInt(a->values[1])!=-5||wireInt(a->values[2])!=7||slots!=std::vector<unsigned>{4,0})throw std::runtime_error("insertion mismatch");
 slots.clear();if(!replayIntegerAppendRecord(r,m,record,true,dirty,false,notify,suppressed)||a->values.size()!=2||wireInt(a->values[1])!=7||slots!=std::vector<unsigned>{8,0})throw std::runtime_error("reverse removal mismatch");
 dirty=0;slots.clear();m.owners.at(layer).attributeFlags.at(identity)|=0x20;if(!replayIntegerAppendRecord(r,m,record,false,dirty,false,notify,suppressed)||a->values.size()!=2||dirty||!slots.empty())throw std::runtime_error("gate replay mismatch");
 a->type=17;if(replayIntegerAppendRecord(r,m,record,false,dirty,false,notify,suppressed))throw std::runtime_error("wrong type replayed");
 std::cout<<"append_record_replay_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
