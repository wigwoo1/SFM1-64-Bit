#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_discovery.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)return 2;auto g=loadGraph(argv[1]);auto paths=channelTimePaths(g,2237);
 for(uint32_t id:{16593u,18239u,24275u,25921u}){ChannelBinding b(g,id);std::cout<<"channel="<<id<<" name="<<g.strings.at(g.elements.at(id).name)<<" mode="<<b.mode<<" source="<<b.source.element<<":"<<b.source.attribute<<" destination="<<b.destination.element<<":"<<b.destination.attribute<<" log="<<b.log<<" layers="<<b.layers.size()<<'\n';
  for(auto frame:paths.at(id).at(0)){AnimationTimeFrame t(g,g.elements.at(frame));std::cout<<"frame="<<frame<<" start="<<t.start<<" duration="<<t.duration<<" offset="<<t.offset<<" scale="<<t.scale<<'\n';for(uint32_t e=0;e<g.elements.size();++e){auto*a=namedAttribute(g,g.elements[e],"timeFrame");if(a&&a->type==1&&a->values.size()==1&&a->values[0].reference==frame)std::cout<<"clip="<<e<<" name="<<g.strings.at(g.elements[e].name)<<'\n';}}
  if(b.source.present()){const auto&e=g.elements.at(b.source.element);std::cout<<"source_type="<<g.strings.at(e.type)<<" source_name="<<g.strings.at(e.name)<<'\n';}
  FloatLogDefault defaults(g,g.elements.at(b.log));std::cout<<"default_enabled="<<defaults.enabled<<" default_value="<<defaults.value<<'\n';
  for(auto layer:b.layers){KeyTrack track(g,g.elements.at(layer));std::cout<<"layer="<<layer<<" keys="<<track.times->values.size();if(!track.times->values.empty())std::cout<<" first="<<wireInt(track.times->values.front())<<" last="<<wireInt(track.times->values.back());std::cout<<'\n';}
 }return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
