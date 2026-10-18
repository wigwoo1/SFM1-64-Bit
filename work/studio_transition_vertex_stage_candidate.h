#pragma once
#include "studio_transition_vertex_values_candidate.h"
#include <stdexcept>
// Slot264 through paired vertex uploads for prototype modes0..6. The color
// dispatch, slot267, auxiliary/pixel helpers and mode7 domain remain separate.
template<class Ops>uint32_t studioTransitionVertexStageCandidate(uint32_t input,uint32_t flag,Ops& ops){
 if(input>6)throw std::invalid_argument("unverified transition input domain");
 ops.entryService();
 const auto state=ops.currentState();
 if(input==6||state==2)return 2;
 const int32_t level=ops.capability();
 if(level>90&&!static_cast<uint8_t>(flag))return 2;
 const uint32_t selected=level>90?1u:0u;
 const bool active=ops.currentState()!=0&&input!=0;
 const auto parameters=studioTransitionVertexValuesCandidate(active,ops);
 const auto color=ops.vertexColor();
 studioShaderConstantPairCandidate(parameters,color,ops);
 return selected;
}
