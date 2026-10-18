#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_channel_binding session.dmx");auto g=loadGraph(argv[1]);auto roots=hierarchyRefs(g,g.elements.at(0),"activeClip",1);ClipHierarchy h(g,roots.at(0));uint64_t layers=0,nullSources=0;std::map<int32_t,uint64_t> modes;
 for(auto i:h.channels){ChannelBinding b(g,i);layers+=b.layers.size();nullSources+=!b.source.present();++modes[b.mode];}
 std::cout<<"bindings="<<h.channels.size()<<" layers="<<layers<<" null_sources="<<nullSources<<'\n';for(auto&m:modes)std::cout<<"mode "<<m.first<<" count="<<m.second<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
