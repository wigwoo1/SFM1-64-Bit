#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_log_layer_bounds.h"
#include "session_log_empty.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_session_log_bounds session.dmx");auto graph=loadGraph(argv[1]);unsigned logs=0,layersCount=0;
 for(const auto&log:graph.elements)if(graph.strings[log.type]=="DmeFloatLog"){
  auto layers=hierarchyRefs(graph,log,"layers",15);std::vector<std::pair<bool,bool>> flags(layers.size(),{false,false});auto bounds=sessionLogBounds(graph,layers,flags);++logs;layersCount+=unsigned(layers.size());
  for(size_t i=0;i<layers.size();++i){const auto*times=namedAttribute(graph,graph.elements[layers[i]],"times");if(bounds[i].times.size()!=times->values.size())throw std::runtime_error("bound key count mismatch");for(size_t j=0;j<bounds[i].times.size();++j)if(bounds[i].times[j]!=wireInt(times->values[j]))throw std::runtime_error("bound tick mismatch");}
  if(!layers.empty()){flags.back()={true,true};auto infinite=sessionLogBounds(graph,layers,flags);if(selectLogLayer(infinite,-2147483647)!=int(layers.size()-1)||selectLogLayer(infinite,2147483647)!=int(layers.size()-1))throw std::runtime_error("runtime infinite overlay mismatch");}
 }
 uint32_t empty=0xffffffff,full=0xffffffff,wrong=0xffffffff;
 for(uint32_t i=0;i<graph.elements.size();++i){const auto&e=graph.elements[i];if(graph.strings[e.type]!="DmeFloatLogLayer"){wrong=i;continue;}const auto*t=namedAttribute(graph,e,"times");if(t&&t->type==21){if(t->values.empty())empty=i;else full=i;}}
 if(empty==0xffffffff||full==0xffffffff||wrong==0xffffffff)throw std::runtime_error("missing emptiness fixtures");
 if(!sessionFloatLogEmpty(graph,{})||!sessionFloatLogEmpty(graph,{empty,wrong,0xffffffff})||sessionFloatLogEmpty(graph,{empty,full})||sessionFloatLogEmpty(graph,{full,empty})||sessionFloatLogEmpty(graph,{wrong,full,0xffffffff}))throw std::runtime_error("whole-log emptiness mismatch");
 std::cout<<"logs="<<logs<<" layer_bounds="<<layersCount<<" explicit_infinite_overlays_verified=1 whole_log_empty_checks=5\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
