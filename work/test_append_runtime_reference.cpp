#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
#include "append_runtime_reference.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;uint32_t log=0xffffffff;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeFloatLog"){log=r.elements.graphHandles[i];break;}
 if(log==0xffffffff)throw std::runtime_error("no float log");
 auto attr=r.attributes.find(log,"layers");auto identity=r.attributes.identity(r.graph,r.elements,attr);auto&owner=m.owners[log];owner.state=4;owner.attributeFlags[identity]=0xd0f;
 std::array<char,16> id{};id.fill(char(0x46));auto target=createRuntimeFloatLayer(r,m,id,4);auto count=r.attributes.resolve(r.graph,r.elements,attr)->values.size();std::vector<unsigned>events;
 auto notify=[&](unsigned phase,uint32_t a,size_t start,size_t end){
  if(a!=attr||start!=count||end!=count||r.attributes.resolve(r.graph,r.elements,attr)->values.size()!=count+1||r.references.occurrences(target,attr)!=(phase==1?0u:1u))throw std::runtime_error("append notification order mismatch");events.push_back(phase);
 };
 auto propagate=[&](uint32_t h,bool refs){if(h!=log||!refs||r.references.occurrences(target,attr)!=1)throw std::runtime_error("propagation order mismatch");events.push_back(3);};
 if(!appendRuntimeReference(r,m,log,"layers",target,false,notify,propagate)||events!=std::vector<unsigned>{1,2,3}||m.queues.marked.size()!=1||(owner.state&3)!=3||(owner.attributeFlags.at(identity)&0x4080)!=0x4080)throw std::runtime_error("append mutation mismatch");
 auto size=r.attributes.resolve(r.graph,r.elements,attr)->values.size();owner.attributeFlags[identity]|=0x20;
 if(appendRuntimeReference(r,m,log,"layers",target,false,notify,propagate)||r.attributes.resolve(r.graph,r.elements,attr)->values.size()!=size||r.references.occurrences(target,attr)!=1)throw std::runtime_error("rejected append changed graph");
 validateGraph(r.graph);std::cout<<"append_order=write,notify,link,notify,propagate mutation_gate_rejection=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
