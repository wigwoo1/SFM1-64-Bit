#pragma once
#include "anchored_document_time_candidate.h"
#include "monotonic_engine_time_candidate.h"
#include "document_frame_rate_lookup_candidate.h"
// Explicit restored-session root, not the active clip. The engine retained
// clock is supplied separately: native storage is global, not document-local.
class DocumentTimeCandidate {
 uint32_t restoredRoot_;
 AnchoredDocumentTimeState state_;
public:
 DocumentTimeCandidate(uint32_t restoredRoot,AnchoredDocumentTimeState state)
  :restoredRoot_(restoredRoot),state_(state){}
 AnchoredDocumentTimeState& state(){return state_;}
 const AnchoredDocumentTimeState& state()const{return state_;}
 uint32_t restoredRoot()const{return restoredRoot_;}
 int32_t sample(const Graph&graph,bool engineGate,uint32_t currentSecondsWord,
                uint32_t& sharedRetainedSecondsWord)const{
  return anchoredDocumentTimeCandidate(state_,[&]{return engineGate;},
   [&]{return monotonicEngineTicksCandidate(currentSecondsWord,sharedRetainedSecondsWord);},
   [&]{return documentFrameRateLookupCandidate(graph,restoredRoot_);});
 }
};
