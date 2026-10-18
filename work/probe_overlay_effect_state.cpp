#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "overlay_effect_state_candidate.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");
 if(argc!=2)throw std::runtime_error("usage: probe_overlay_effect_state session.dmx");
 const auto g=loadGraph(argv[1]);
 size_t effects=0,filmClips=0,linkedEffects=0,emptyLinks=0;
 for(const auto&e:g.elements){
  const auto&type=g.strings.at(e.type);
  if(type=="DmeMaterialOverlayFXClip"){
   const OverlayEffectStateCandidate state(g,e);(void)state;++effects;
  }
  if(type!="DmeFilmClip")continue;
  ++filmClips;
  for(const auto&a:e.attributes){
   if(g.strings.at(a.name)!="materialOverlay")continue;
   if(a.type!=1||a.values.size()!=1)throw std::runtime_error("invalid overlay reference");
   auto ref=a.values[0].reference;
   if(ref==0xffffffff){++emptyLinks;continue;}
   if(ref>=g.elements.size())throw std::runtime_error("external/unresolved overlay reference");
   const OverlayEffectStateCandidate state(g,g.elements[ref]);(void)state;++linkedEffects;
  }
 }
 std::cout<<"{\"pointer_bits\":64,\"film_clips\":"<<filmClips<<",\"decoded_effects\":"<<effects<<",\"linked_effects\":"<<linkedEffects<<",\"empty_links\":"<<emptyLinks<<",\"scope\":\"read_only_session_overlay_inventory\"}\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
