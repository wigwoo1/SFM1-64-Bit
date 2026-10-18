#pragma once
#include <cstdint>
#include <cstring>
#include <emmintrin.h>

struct EngineTickAccumulatorResultCandidate { int32_t ticks; uint32_t remainderWord; };
// Installed engine10182418..10182486. Explicit state inputs, no UI-clock policy.
inline EngineTickAccumulatorResultCandidate engineTickAccumulatorCandidate(
    uint32_t accumulatedWord, uint32_t elapsedWord, uint32_t intervalWord,
    bool retainAccumulator, bool alternateTicks, int32_t currentTick) {
    float accumulated, elapsed, interval;
    std::memcpy(&accumulated,&accumulatedWord,4);
    std::memcpy(&elapsed,&elapsedWord,4);
    std::memcpy(&interval,&intervalWord,4);
    __m128 value=_mm_set_ss(accumulated), scale=_mm_set_ss(interval);
    if(!retainAccumulator) value=_mm_add_ss(_mm_set_ss(elapsed),value);
    int32_t ticks=0;
    // COMISS/JB skips ordered less and unordered. Ordered greater/equal consumes.
    if(_mm_comige_ss(value,scale)) {
        ticks=_mm_cvttss_si32(_mm_div_ss(value,scale));
        if(alternateTicks) {
            uint32_t word=(uint32_t(currentTick)+uint32_t(ticks)+1u)&~1u;
            word-=uint32_t(currentTick);
            std::memcpy(&ticks,&word,4);
        }
        value=_mm_sub_ss(value,_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),ticks),scale));
    }
    float result;_mm_store_ss(&result,value);
    uint32_t word;std::memcpy(&word,&result,4);
    return {ticks,word};
}
