#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_model_placement_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("session required");auto graph=loadGraph(argv[1]);GraphTransformHierarchy hierarchy(graph);size_t count=0;for(uint32_t id=0;id<graph.elements.size();++id)if(graph.strings.at(graph.elements[id].type)=="DmeGameModel"){auto placement=readSessionModelPlacementCandidate(graph,id,hierarchy);auto origin=placement.matrix.clip({0,0,0});if(origin[3]!=1)throw std::runtime_error("placement homogeneous coordinate");std::cout<<"model="<<id<<" asset="<<placement.asset<<" world_origin="<<origin[0]<<","<<origin[1]<<","<<origin[2]<<'\n';++count;}if(!count)throw std::runtime_error("no model fixtures");std::cout<<"session_model_placement_candidates="<<count<<" native_draw_parity=unverified graph_modified=no\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
