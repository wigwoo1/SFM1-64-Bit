#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
#include "animation_time_frame.h"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_animation_time_frame session.dmx");auto g=loadGraph(argv[1]);uint64_t frames=0;
 for(auto&e:g.elements)if(g.strings[e.type]=="DmeTimeFrame"){AnimationTimeFrame f(g,e);++frames;}
 Graph sample;sample.strings={"DmeTimeFrame","start","duration","offset","scale","times","values"};Element frame;frame.type=0;
 for(unsigned i=1;i<=4;++i){Attribute a;a.name=i;a.type=i==4?3:7;Value v;v.wire.resize(4);setWireU32(v,i==1?10:i==2?100:i==3?5:0x40000000);a.values.push_back(v);frame.attributes.push_back(a);}
 Element layer;Attribute times;times.name=5;times.type=21;Attribute values;values.name=6;values.type=17;
 for(unsigned tick:{10u,30u,210u}){Value t;t.wire.resize(4);setWireU32(t,tick);times.values.push_back(t);Value v;v.wire.resize(4);setWireU32(v,tick+100);values.values.push_back(v);}layer.attributes={times,values};
 AnimationTimeFrame f(sample,frame);KeyTrack track(sample,layer);
 for(auto test:std::array<std::array<int32_t,3>,3>{{{{0,1,110}},{{20,0,130}},{{200,1,310}}}}){auto*v=f.exactKey(track,test[0],test[1]!=0);if(!v||wireInt(*v)!=test[2])throw std::runtime_error("time-frame key lookup mismatch");}
 if(f.exactKey(track,21,false))throw std::runtime_error("off-key time incorrectly sampled");
 std::cout<<"frames_loaded="<<frames<<" integrated_cases=4\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
