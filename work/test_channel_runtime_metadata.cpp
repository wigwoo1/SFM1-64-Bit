#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "channel_runtime_metadata.h"
#include "session_reference_propagation.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;size_t channels=0,checks=0;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
  auto h=r.elements.graphHandles[i];if(!bindChannelRuntimeMetadata(r,m,h,4))throw std::runtime_error("channel schema mismatch");++channels;
  for(auto name:{"fromElement","toElement"}){auto a=r.attributes.find(h,name);auto id=r.attributes.identity(r.graph,r.elements,a);if(m.owners.at(h).attributeFlags.at(id)!=0x1501)throw std::runtime_error("endpoint barrier flags mismatch");++checks;}
  auto log=r.attributes.find(h,"log");auto id=r.attributes.identity(r.graph,r.elements,log);if(m.owners.at(h).attributeFlags.at(id)!=0x401)throw std::runtime_error("log propagation flags mismatch");++checks;
 }
 if(!channels)throw std::runtime_error("no channels");
 std::cout<<"channels_registered="<<channels<<" recovered_fields="<<channels*8<<" flag_checks="<<checks<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
