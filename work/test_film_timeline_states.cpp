#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_root_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "retained_film_track_candidate.h"
#include <iostream>
int main(){try{auto g=loadGraph("work/roundtrip-11.dmx");auto root=sessionRenderRootCandidate(g);RetainedFilmTrackCandidate index(g,root);auto frames=sessionTimelineFramesCandidate(g,root);size_t gap=0,blank=0,scene=0;for(size_t i=0;i<frames.size();++i){auto s=index.classify(frames.tick(i));if(!s.shot){if(s.state!=FilmTimelineStateCandidate::Gap)throw std::runtime_error("gap classification");++gap;continue;}const auto&e=g.elements.at(s.shot->clip);bool hasScene=!hierarchyRefs(g,e,"camera",1).empty()&&!hierarchyRefs(g,e,"scene",1).empty();if(hasScene!=(s.state==FilmTimelineStateCandidate::SceneClip))throw std::runtime_error("scene classification");if(hasScene)++scene;else ++blank;}
 if(!gap||!blank||!scene||gap+blank+scene!=frames.size())throw std::runtime_error("classification coverage");
 std::cout<<"film timeline states passed frames="<<frames.size()<<" gap="<<gap<<" scene_less="<<blank<<" scene="<<scene<<" scene_asset_readiness=not_claimed native_gap_display=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
