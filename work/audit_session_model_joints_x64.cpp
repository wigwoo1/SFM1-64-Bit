#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("session required");auto graph=loadGraph(argv[1]);auto&model=graph.elements.at(2552);std::cout<<"model_type="<<graph.strings.at(model.type)<<'\n';for(auto&a:model.attributes){std::cout<<graph.strings.at(a.name)<<" type="<<unsigned(a.type)<<" count="<<a.values.size()<<'\n';if(a.type==15&&graph.strings.at(a.name)=="bones")for(auto&v:a.values){auto&e=graph.elements.at(v.reference);std::cout<<"joint="<<v.reference<<" type="<<graph.strings.at(e.type)<<" name="<<graph.strings.at(e.name)<<'\n';}}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
