#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_root_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "scene_frame_source_candidate.h"
#include <iostream>
int main(){try{
 SessionTimelineFramesCandidate sixty(-100,10000,60u|(10000u<<16));
 if(sixty.size()!=60||sixty.tick(0)!=-100||sixty.tick(1)!=66||sixty.tick(59)!=9733)throw std::runtime_error("integer cadence");
 SessionTimelineFramesCandidate ntsc(0,10000,30u|(10010u<<16));
 if(ntsc.size()!=30||ntsc.tick(1)!=333||ntsc.tick(29)!=9676)throw std::runtime_error("fractional cadence");
 SessionTimelineFramesCandidate longTimeline(0,2000000,60u|(10000u<<16));
 if(longTimeline.size()!=12000||longTimeline.tick(11999)!=1999833)throw std::runtime_error("long timeline");
 SceneFrameSourceCandidate startFrameSource;startFrameSource.setTimeline(sixty);
 if(!sceneFrameSourceMatchesInitialTickCandidate(startFrameSource,1,sixty.tick(1))||
    sceneFrameSourceMatchesInitialTickCandidate(startFrameSource,1,sixty.tick(0))||
    sceneFrameSourceMatchesInitialTickCandidate(startFrameSource,sixty.size(),sixty.tick(0)))
  throw std::runtime_error("nonzero automatic timeline start validation");
 bool rejected=false;try{sixty.tick(60);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("endpoint accepted");
 rejected=false;try{SessionTimelineFramesCandidate bad(INT32_MAX,2,60u|(10000u<<16));}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("overflow accepted");
 auto graph=loadGraph("work/roundtrip-11.dmx");auto root=sessionRenderRootCandidate(graph);auto frames=sessionTimelineFramesCandidate(graph,root);
 size_t active=0,gaps=0,overlaps=0;for(size_t i=0;i<frames.size();++i){auto shots=activeFilmShotsCandidate(graph,root,frames.tick(i));if(shots.size()==1)++active;else if(shots.empty())++gaps;else {++overlaps;std::cout<<"timeline_overlap frame="<<i<<" tick="<<frames.tick(i)<<" clips=";for(const auto&shot:shots)std::cout<<shot.clip<<',';std::cout<<'\n';}}
 if(!active)throw std::runtime_error("no renderable timeline samples");
 std::cout<<"session timeline frames passed count="<<frames.size()<<" first="<<frames.tick(0)<<" last="<<frames.tick(frames.size()-1)<<" active="<<active<<" gaps="<<gaps<<" overlaps="<<overlaps<<" rational_rate=passed lazy_12000_frames=passed native_rounding=unverified\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
