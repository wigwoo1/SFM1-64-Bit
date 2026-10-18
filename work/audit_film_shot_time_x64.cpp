#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "film_shot_time_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("session required");auto g=loadGraph(argv[1]);ClipHierarchy hierarchy(g,2237);for(auto id:hierarchy.clips){auto&e=g.elements[id];if(g.strings[e.type]!="DmeFilmClip")continue;AnimationTimeFrame f(g,g.elements.at(hierarchy.frames.at(id)));std::cout<<"film="<<id<<" start="<<f.start<<" duration="<<f.duration<<" offset="<<f.offset<<" scale="<<f.scale<<'\n';}for(int32_t tick=0;tick<100000;tick+=1000){auto shots=activeFilmShotsCandidate(g,2237,tick);std::cout<<"tick="<<tick;for(auto shot:shots)std::cout<<" shot="<<shot.clip<<" local="<<shot.localTick;std::cout<<'\n';}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
