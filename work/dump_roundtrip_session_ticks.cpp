#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_frames_candidate.h"
#include "session_render_root_candidate.h"
#include <iostream>
int main(){auto g=loadGraph("work/roundtrip-11.dmx");auto root=sessionRenderRootCandidate(g,{});auto f=sessionTimelineFramesCandidate(g,root);std::cout<<"root="<<root<<" count="<<f.size()<<" ticks=";for(size_t i=0;i<10&&i<f.size();++i)std::cout<<i<<":"<<f.tick(i)<<" ";std::cout<<"\n";}
