#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_shot_timeline_candidate.h"
#include <iostream>
int main(){try{RetainedShotTimelineCandidate timeline(loadGraph("work/roundtrip-11.dmx"),2237);for(auto p:std::vector<std::pair<int32_t,uint32_t>>{{870833,2470},{984500,5986},{984666,5986},{984667,6753},{1034500,6848},{1034667,7615},{1174500,15477},{1174667,16244},{1259500,20053}}){auto selected=timeline.selectShot(p.first);if(selected.clip!=p.second)throw std::runtime_error("integrated selection");}
 auto evaluated=timeline.step(984500);if(evaluated.shot.clip!=5986)throw std::runtime_error("overlap evaluation");auto repeated=timeline.step(984666);if(repeated.rebuilt)throw std::runtime_error("overlap retained pass rebuilt");
 auto before=timeline.stepAtShutterSample(984666,984667);
 if(before.rebuilt||before.shot.clip!=5986||before.shot.parentTick!=984667)throw std::runtime_error("shutter sample switched to following shot");
 auto after=timeline.stepAtShutterSample(984667,984666);
 if(!after.rebuilt||after.shot.clip!=6753||after.shot.parentTick!=984666)throw std::runtime_error("shutter sample went back to preceding shot");
 std::cout<<"retained shot selection passed boundaries=9 overlap_evaluation=passed retained_pass_reuse=passed shutter_cut_directions=2\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
