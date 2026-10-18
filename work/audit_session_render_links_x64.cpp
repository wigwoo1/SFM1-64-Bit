#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("session required");auto graph=loadGraph(argv[1]);for(uint32_t id=0;id<graph.elements.size();++id){auto&e=graph.elements[id];if(id!=0&&graph.strings.at(e.type)!="DmeFilmClip")continue;std::cout<<"element="<<id<<" type="<<graph.strings.at(e.type)<<" name="<<graph.strings.at(e.name)<<'\n';for(auto&a:e.attributes){auto name=graph.strings.at(a.name);if(name!="camera"&&name!="scene"&&name!="activeClip"&&name!="workCamera")continue;std::cout<<"  "<<name<<" type="<<unsigned(a.type);for(auto&v:a.values)std::cout<<" ref="<<v.reference;std::cout<<'\n';}}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
