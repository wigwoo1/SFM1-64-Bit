#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "document_frame_rate_lookup_candidate.h"
#include "anchored_document_time_candidate.h"
#include "session_frame_rate_candidate.h"
int main(){try{
 Graph g;g.strings={"owner","settings","renderSettings","frameRate"};g.elements.resize(3);
 auto ref=[](uint32_t name,uint32_t target){Value value;value.reference=target;return Attribute{name,1,{value}};};
 g.elements[0].attributes={ref(1,1)};g.elements[1].attributes={ref(2,2)};
 Value rate;rate.wire.resize(4);float fps=25;std::memcpy(rate.wire.data(),&fps,4);g.elements[2].attributes={{3,3,{rate}}};
 AnchoredDocumentTimeState state(0);state.saved684=201;state.flag6c0=true;
 auto sample=[&]{return anchoredDocumentTimeCandidate(state,[]{return false;},[]()->int32_t{throw std::runtime_error("unexpected engine clock");},[&]{return documentFrameRateLookupCandidate(g,0);});};
 if(sample()!=400)throw std::runtime_error("graph frame rate not used");
 g.elements[2].attributes.clear();if(sample()!=0)throw std::runtime_error("missing rate default");
 g.elements[1].attributes.clear();if(sample()!=0)throw std::runtime_error("missing render settings default");
 g.elements[0].attributes.clear();if(sample()!=0)throw std::runtime_error("missing settings default");
 if(documentFrameRateLookupCandidate(g,0xffffffffu))throw std::runtime_error("missing owner lookup");
 g.elements[0].attributes={ref(1,1)};g.elements[1].attributes={ref(2,2)};g.elements[2].attributes={{3,2,{rate}}};bool rejected=false;try{sample();}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("unsupported type accepted");
 auto session=loadGraph("work/movie-640x480-session.dmx");unsigned owners=0;
 for(uint32_t i=0;i<session.elements.size();++i){const auto active=hierarchyRefs(session,session.elements[i],"activeClip",1);if(active.size()!=1)continue;
  const auto fpsValue=documentFrameRateLookupCandidate(session,i);if(!fpsValue)throw std::runtime_error("fixture render rate missing");
  if(recoveredFloatRateCandidate(*fpsValue)!=sessionFrameRateCandidate(session,active[0],false))throw std::runtime_error("real session rate mismatch");
  const auto before=session.elements[i].attributes;session.elements[i].attributes.clear();if(documentFrameRateLookupCandidate(session,i))throw std::runtime_error("real missing settings fallback");session.elements[i].attributes=before;++owners;
 }
 if(owners!=1)throw std::runtime_error("real session owner coverage");
 std::cout<<"document_frame_rate_graph_fallback_checks=6 real_session_owner_checks="<<owners<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
