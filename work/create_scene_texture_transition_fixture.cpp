#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
int main(){try{auto graph=loadGraph("work/roundtrip-11.dmx");size_t edits=0;for(auto&attribute:graph.elements.at(2553).attributes)if(graph.strings.at(attribute.name)=="$basetexture"){setString(graph,attribute,0,"models/anno/mamagen/headkogenta");++edits;}if(edits!=1)throw std::runtime_error("fixture material edit coverage");saveGraph(graph,"work/texture-transition-session.dmx");std::cout<<"controlled saved session texture fixture created edits=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
