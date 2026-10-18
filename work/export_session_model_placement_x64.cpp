#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_model_placement_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=4)throw std::runtime_error("session model-element-id fresh-output required");std::string text=argv[2];size_t consumed=0;auto id=std::stoul(text,&consumed);if(consumed!=text.size()||id>UINT32_MAX)throw std::runtime_error("model element id");if(std::filesystem::exists(argv[3]))throw std::runtime_error("refuse placement overwrite");auto graph=loadGraph(argv[1]);GraphTransformHierarchy hierarchy(graph);auto placement=readSessionModelPlacementCandidate(graph,uint32_t(id),hierarchy);std::ofstream output(argv[3],std::ios::binary);output.write(reinterpret_cast<const char*>(&placement.matrix),64);output.close();if(!output)throw std::runtime_error("placement write");std::cout<<"exported_model="<<id<<" asset="<<placement.asset<<" matrix_bytes=64 native_draw_parity=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
