#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_hierarchy.h"
#include "session_camera_settings_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("session required");auto graph=loadGraph(argv[1]);GraphTransformHierarchy hierarchy(graph);size_t count=0;double maxError=0;for(uint32_t id=0;id<graph.elements.size();++id){if(graph.strings.at(graph.elements[id].type)!="DmeCamera")continue;readSessionCameraSettingsCandidate(graph,id);auto world=hierarchy.world(id);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)if(!std::isfinite(world[i][j]))throw std::runtime_error("nonfinite camera world matrix");for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){double dot=0;for(unsigned k=0;k<3;++k)dot+=double(world[k][i])*world[k][j];maxError=std::max(maxError,std::abs(dot-(i==j?1.0:0.0)));}std::cout<<"camera="<<id<<" world_rows=";for(auto&row:world)for(float v:row)std::cout<<v<<',';std::cout<<'\n';++count;}if(!count||maxError>1e-4)throw std::runtime_error("camera rigid basis check");std::cout<<"camera_world_candidates="<<count<<" max_orthonormal_error="<<maxError<<" native_projection_parity=unverified graph_modified=no\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

