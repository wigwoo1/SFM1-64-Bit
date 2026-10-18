#pragma once
#include "model_render_record_allocate_candidate.h"
//100D8220. Original record+8 is supplied through a pointer-width-safe getter.
// Service ownership and engine interface binding remain caller responsibilities.
template<class Value,class StudioRelease,class PairCleanup,class ResourceCleanup>
inline void modelRenderRecordReleaseCandidate(ModelRenderRecordAllocationCandidate& state,uint32_t argument,Value value,StudioRelease studioRelease,PairCleanup pairCleanup,ResourceCleanup resourceCleanup){
 const uint16_t index=uint16_t(argument);if(index==0xffff)return;
 studioRelease(value(index));pairCleanup(argument);resourceCleanup(argument);
 engineShadowPoolUnlinkCandidate(state.pool,argument);
 state.pool.links[index].next=state.freeHead;state.freeHead=index;
}
