#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: inspect_clip_links session.dmx");auto g=loadGraph(argv[1]);std::map<std::string,unsigned> shown;
 for(size_t i=0;i<g.elements.size();++i){auto&e=g.elements[i];auto&t=g.strings[e.type];if(t.find("Clip")==std::string::npos&&t!="DmeTrack"&&t!="DmeTrackGroup"&&t!="DmeChannel")continue;if(shown[t]++>=2)continue;
  std::cout<<"element "<<i<<" "<<t<<" "<<g.strings[e.name]<<'\n';
  for(auto&a:e.attributes){std::cout<<"  "<<g.strings[a.name]<<" type="<<unsigned(a.type)<<" count="<<a.values.size();if(a.type==1||a.type==15){for(size_t j=0;j<std::min<size_t>(a.values.size(),5);++j){auto ref=a.values[j].reference;std::cout<<" ->"<<ref;if(ref<g.elements.size())std::cout<<":"<<g.strings[g.elements[ref].type];}}std::cout<<'\n';}
 }return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
