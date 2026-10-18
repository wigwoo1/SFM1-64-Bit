#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_log.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;auto count=r.graph.elements.size();std::array<char,16>log{},layer{};log.fill(char(0x61));layer.fill(char(0x62));
 auto handle=createRuntimeFloatLog(r,m,log,layer,4);auto attr=r.attributes.find(handle,"layers");auto values=r.attributes.resolve(r.graph,r.elements,attr);
 if(r.graph.elements.size()!=count+2||values->values.size()!=1||values->values[0].reference!=count+1||r.references.occurrences(r.elements.graphHandles[count+1],attr)!=1)throw std::runtime_error("log creation mismatch");
 auto used=r.attributes.resolve(r.graph,r.elements,r.attributes.find(handle,"usedefaultvalue"));auto def=r.attributes.resolve(r.graph,r.elements,r.attributes.find(handle,"defaultvalue"));
 if(used->values.at(0).wire!=std::vector<char>(1,0)||def->values.at(0).wire!=std::vector<char>(4,0))throw std::runtime_error("log defaults mismatch");
 bool rejected=false;try{createRuntimeFloatLog(r,m,log,layer,4);}catch(const std::runtime_error&){rejected=true;}if(!rejected||r.graph.elements.size()!=count+2)throw std::runtime_error("duplicate creation mutated graph");
 saveGraph(r.graph,argv[2]);SessionRuntime copy(loadGraph(argv[2]));auto h=copy.elements.graphHandles[count];auto a=copy.attributes.find(h,"layers");
 if(copy.graph.elements[count].id!=log||copy.graph.elements[count+1].id!=layer||copy.attributes.resolve(copy.graph,copy.elements,a)->values.at(0).reference!=count+1||copy.references.occurrences(copy.elements.graphHandles[count+1],a)!=1)throw std::runtime_error("created log reload mismatch");
 std::cout<<"float_log_creation_checks=4 initial_layers=1 reload=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
