#pragma once
#include "studio_transition_vertex_stage_candidate.h"
#include "studio_transition_tail_candidate.h"
// Logical composition for input0..6; Ops supplies live owner state and bridges
// recovered helpers. No native ABI, complete renderer or mode7 claim.
template<class Ops>void studioShaderTransitionCandidate(uint32_t input,uint32_t flag,uint32_t conversionFlag,uint32_t bypassFlag,Ops& ops){
 const auto selected=studioTransitionVertexStageCandidate(input,flag,ops);
 studioTransitionTailCandidate(input,selected,conversionFlag,bypassFlag,ops);
}
