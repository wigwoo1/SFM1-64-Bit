#pragma once
#include "studio_transition_vertex_owner_candidate.h"
#include "studio_transition_tail_candidate.h"
// Composes live vertex-owner state with the recovered tail's owner services.
// Tail services must observe the same state as vertexOwner and bind recovered
// slot267/auxiliary/conversion/pixel/render-state adapters. Input domain0..6;
// renderer storage ownership, command consumers and mode7 remain unresolved.
template<class Services>void studioTransitionOwnerEntryCandidate(
 StudioTransitionVertexOwnerCandidate& vertexOwner,uint32_t input,uint32_t flag,
 uint32_t conversionFlag,uint32_t bypassFlag,Services& services){
 const auto selected=studioTransitionVertexOwnerCandidate(vertexOwner,input,flag,services);
 studioTransitionTailCandidate(input,selected,conversionFlag,bypassFlag,services);
}
