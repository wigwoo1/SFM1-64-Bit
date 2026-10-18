#pragma once
#include "engine_shadow_enable_candidate.h"
#include "engine_projection_cleanup_candidate.h"
#include "engine_projection_pair_unlink_candidate.h"
// Connects recovered slot15 sequencing to cleanup and paired-map algorithms.
// Resource release/head/flags services remain supplied by the owned runtime.
template<class Head,class Entry,class Release,class SetHead,class CurrentHead,class OppositeHead,class Flags>
inline void engineProjectionEnableStageCandidate(uint32_t handle,uint32_t enabled,
 EngineProjectionPairMapCandidate& pairs,Head head,Entry entry,Release release,
 SetHead setHead,CurrentHead currentHead,OppositeHead oppositeHead,Flags flags){
 engineShadowEnableCandidate(handle,enabled,
  [&](uint32_t h){engineProjectionCleanupCandidate(h,head,entry,release,setHead);},
  [&](uint32_t h){engineProjectionPairUnlinkCandidate(pairs,h,currentHead,oppositeHead);},flags);
}
