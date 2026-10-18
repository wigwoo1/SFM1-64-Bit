#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_root_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "retained_film_track_candidate.h"
#include <iostream>
int main(){try{auto g=loadGraph("work/roundtrip-11.dmx");auto root=sessionRenderRootCandidate(g);RetainedFilmTrackCandidate index(g,root);auto frames=sessionTimelineFramesCandidate(g,root);auto track=filmClipTrackCandidate(g,root);size_t selected=0,empty=0;for(size_t i=0;i<frames.size();++i){auto tick=frames.tick(i);auto actual=index.select(tick);auto expected=filmTrackClipAtTimeCandidate(g,*track,tick);if(bool(actual)!=bool(expected)||(actual&&actual->clip!=*expected))throw std::runtime_error("retained selection mismatch");if(actual)++selected;else ++empty;}
 for(auto pair:std::vector<std::pair<int32_t,uint32_t>>{{984500,5986},{1034500,6848},{1174500,15477}})if(index.select(pair.first)->clip!=pair.second)throw std::runtime_error("overlap mismatch");
 auto refs=hierarchyRefs(g,g.elements[root],"timeFrame",1);auto&f=g.elements[refs[0]];for(auto&a:f.attributes){auto name=g.strings[a.name];if(name=="start")setWireU32(a.values[0],100);if(name=="offset")setWireU32(a.values[0],200);if(name=="scale")setFloat(a,0,2);}
 RetainedFilmTrackCandidate transformed(g,root);auto changed=transformed.select(492150);if(!changed||changed->clip!=5986||changed->parentTick!=984500)throw std::runtime_error("root media conversion");
 std::cout<<"retained film track passed frames="<<frames.size()<<" clips="<<index.clipCount()<<" selected="<<selected<<" no_clip="<<empty<<" overlaps=resolved root_offset_scale=passed native_runtime=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
