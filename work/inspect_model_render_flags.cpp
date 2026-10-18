#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
int main(){try{auto g=loadGraph("work/roundtrip-11.dmx");auto&e=g.elements.at(2552);std::cout<<"element=2552 type="<<g.strings[e.type]<<'\n';for(auto&a:e.attributes){std::cout<<g.strings[a.name]<<" type="<<unsigned(a.type)<<" count="<<a.values.size();if(a.type==4)for(auto&v:a.values)for(unsigned char b:v.wire)std::cout<<" bool_byte="<<unsigned(b);std::cout<<'\n';}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
