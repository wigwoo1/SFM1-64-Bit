#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "film_track_selection_candidate.h"
#include <iostream>
int main(){try{auto g=loadGraph("work/roundtrip-11.dmx");for(auto pair:std::vector<std::pair<int32_t,uint32_t>>{{984500,5986},{984666,5986},{984667,6753},{1034500,6848},{1034666,6848},{1034667,7615},{1174500,15477},{1174666,15477},{1174667,16244}}){auto selected=filmTrackClipAtTimeCandidate(g,2239,pair.first);if(!selected||*selected!=pair.second)throw std::runtime_error("overlap selection");}
 if(filmClipTrackCandidate(g,2237)!=std::optional<uint32_t>(2239)||filmClipTrackCandidate(g,5986))throw std::runtime_error("film track mapping");
 auto group=hierarchyRefs(g,g.elements[2237],"subClipTrackGroup",1)[0];auto*tracks=const_cast<Attribute*>(namedAttribute(g,g.elements[group],"tracks"));auto original=tracks->values;tracks->values.insert(tracks->values.begin(),Value{});if(filmClipTrackCandidate(g,2237))throw std::runtime_error("null first track skipped");tracks->values=original;
 auto*children=const_cast<Attribute*>(namedAttribute(g,g.elements[2239],"children"));std::reverse(children->values.begin(),children->values.end());if(filmTrackClipAtTimeCandidate(g,2239,984500)!=std::optional<uint32_t>(5986))throw std::runtime_error("sorting missing");
 std::cout<<"film track selection passed overlap_samples=6 exclusive_end_transitions=3 reversed_children=passed scope=method_candidate viewport_parity=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
