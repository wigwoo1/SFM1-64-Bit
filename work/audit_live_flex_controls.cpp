#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_shot_timeline_candidate.h"
#include "session_local_flex_controllers_candidate.h"
#include <fstream>
#include <iostream>
int main(){try{
 std::ifstream file(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl)",std::ios::binary);if(!file)throw std::runtime_error("missing MDL");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});auto metadata=readMdl49FlexMetadata(bytes);
 RetainedShotTimelineCandidate timeline(loadGraph("work/roundtrip-11.dmx"),2237);std::vector<float>first;std::set<size_t>changed;size_t samples=0;
 std::ofstream output("outputs/live-flex-control-samples.csv");if(!output)throw std::runtime_error("audit output unavailable");output<<"tick";for(const auto&c:metadata)output<<','<<c.name;output<<'\n';output.precision(9);
 for(int n=0;n<61;++n){int32_t tick=870833+n*10000/60;auto shot=timeline.selectShot(tick);if(shot.clip!=2470)break;timeline.step(tick,2,2);auto controls=readSessionLocalFlexControllersCandidate(timeline.graph(),2552,metadata);if(first.empty())first=controls;for(size_t i=0;i<controls.size();++i)if(controls[i]!=first[i])changed.insert(i);output<<tick;for(float v:controls)output<<','<<v;output<<'\n';++samples;}
 timeline.step(870833,2,2);if(readSessionLocalFlexControllersCandidate(timeline.graph(),2552,metadata)!=first)throw std::runtime_error("backward seek changed flex controls");
 std::cout<<"live_flex_control_audit samples="<<samples<<" controllers="<<metadata.size()<<" varying="<<changed.size()<<" backward_seek_exact=1\n";for(auto i:changed)std::cout<<"varying_controller="<<metadata[i].name<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
