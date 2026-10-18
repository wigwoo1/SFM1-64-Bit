#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
int main(){auto g=loadGraph("work/roundtrip-11.dmx");auto ref=hierarchyRefs(g,g.elements[2237],"timeFrame",1)[0];for(auto&a:g.elements[ref].attributes){auto name=g.strings[a.name];if(name=="start"||name=="offset")setWireU32(a.values[0],870833);if(name=="duration")setWireU32(a.values[0],500);}saveGraph(g,"work/short-timeline-session.dmx");}
