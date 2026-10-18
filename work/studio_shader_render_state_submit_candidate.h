#pragma once
#include <cstdint>
// Logical1000e330. Caller supplies the selected live cache entry and owner
// operations; native queue/device lifetimes and cache-table bounds are external.
template<class Ops>void studioShaderRenderStateSubmitCandidate(uint32_t& cache,uint32_t key,uint32_t value,uint32_t flush,Ops& ops){
 if(uint8_t(ops.suppressed()))return;
 if(cache==value)return;
 if(uint8_t(flush))ops.flush();
 if(ops.queueEnabled())ops.queueState(key,value);else ops.directState(key,value);
 cache=value;
}
