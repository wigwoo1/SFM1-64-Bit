#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_root_candidate.h"
#include "session_frame_rate_candidate.h"
#include "elapsed_frame_clock_candidate.h"
#include <iostream>
int main(){try{auto graph=loadGraph("work/roundtrip-11.dmx");auto rate=sessionFrameRateCandidate(graph,sessionRenderRootCandidate(graph));ElapsedFrameClockCandidate clock;clock.setPackedRate(rate);clock.reset(100,3);if(clock.sample(1000000100,1000)!=63||clock.waitMilliseconds(100)!=17)throw std::runtime_error("saved scene playback cadence");clock.setPackedRate(recoveredFloatRateCandidate(24));clock.reset(100,3);if(clock.sample(1000000100,1000)!=27||clock.waitMilliseconds(100)!=42)throw std::runtime_error("explicit playback cadence");std::cout<<"scene playback rate passed saved_60fps_elapsed_1s=60 explicit_24fps_elapsed_1s=24 waits_ms=17,42\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
