#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_clip_hierarchy session.dmx");auto g=loadGraph(argv[1]);auto roots=hierarchyRefs(g,g.elements.at(0),"activeClip",1);if(roots.size()!=1)throw std::runtime_error("missing active clip");ClipHierarchy h(g,roots[0]);std::cout<<"nodes="<<h.nodes.size()<<" clips="<<h.clips.size()<<" frames="<<h.frames.size()<<" channels="<<h.channels.size()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
