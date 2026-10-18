#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_animation_keys session.dmx");auto g=loadGraph(argv[1]);uint64_t layers=0,keys=0;
 for(auto&e:g.elements){if(g.strings[e.type].find("LogLayer")==std::string::npos)continue;KeyTrack track(g,e);++layers;
  for(size_t i=0;i<track.times->values.size();++i){auto tick=wireInt(track.times->values[i]);auto*v=track.exact(tick);if(!v)throw std::runtime_error("stored key not found");if(i==0||tick!=wireInt(track.times->values[i-1])){if(v->wire!=track.values->values[i].wire||v->text!=track.values->values[i].text)throw std::runtime_error("key lookup value mismatch");}++keys;}
 }
 std::cout<<"{\"layers\":"<<layers<<",\"stored_keys_checked\":"<<keys<<",\"scope\":\"exact_key_lookup_only\"}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
