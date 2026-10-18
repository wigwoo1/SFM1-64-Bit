#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "float_log_default.h"
#include "channel_play_path.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_play_paths session.dmx");auto g=loadGraph(argv[1]);auto roots=hierarchyRefs(g,g.elements.at(0),"activeClip",1);ClipHierarchy h(g,roots.at(0));uint64_t copy=0,sample=0,none=0;
 for(auto i:h.channels){ChannelBinding b(g,i);if(b.mode!=3||b.log==0xffffffff||g.strings[g.elements[b.log].type]!="DmeFloatLog")continue;FloatLogDefault d(g,g.elements[b.log]);bool empty=true;for(auto ref:b.layers){auto*t=namedAttribute(g,g.elements.at(ref),"times");if(!t||t->type!=21)throw std::runtime_error("invalid key times");if(!t->values.empty())empty=false;}
  auto path=channelPlayPath(b.destination.present(),true,false,empty,d.enabled,b.source.present());copy+=path==ChannelPlayPath::CopySource;sample+=path==ChannelPlayPath::SampleLog;none+=path==ChannelPlayPath::NoWrite;
 }
 if(channelPlayPath(true,true,true,true,false,true)!=ChannelPlayPath::SampleLog||channelPlayPath(true,true,false,true,false,true)!=ChannelPlayPath::CopySource||channelPlayPath(true,true,false,true,true,true)!=ChannelPlayPath::SampleLog||channelPlayPath(false,true,false,true,false,true)!=ChannelPlayPath::NoWrite)throw std::runtime_error("play decision mismatch");
 std::cout<<"copy="<<copy<<" sample="<<sample<<" no_write="<<none<<" synthetic_cases=4\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
