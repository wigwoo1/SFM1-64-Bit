#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <emmintrin.h>
struct MaterialAnimationFrameCandidate {int32_t frame=0,previousFrame=0;bool wrapDetected=false,invokeWrapCallback=false;};
// Recovered tf/client.dll 1000f120 arithmetic. Caller supplies actual float clock words.
// No replacement for the original clock producer or material-variable binding.
inline MaterialAnimationFrameCandidate materialAnimationFrameCandidate(uint32_t currentWord,uint32_t durationWord,uint32_t startWord,uint32_t rateWord,int32_t count,bool wrap){
 if(count<=0)throw std::invalid_argument("material animation frame count");
 auto value=[](uint32_t word){float f;std::memcpy(&f,&word,4);return _mm_set_ss(f);};
 auto current=_mm_sub_ss(value(currentWord),value(startWord));
 auto previous=_mm_sub_ss(current,value(durationWord));
 const auto zero=_mm_setzero_ps();float currentScalar,previousScalar;
 _mm_store_ss(&currentScalar,current);_mm_store_ss(&previousScalar,previous);
 // COMISS JA: ordered negative clamps; unordered and signed zero retain their words.
 if(currentScalar<0)current=zero;if(previousScalar<0)previous=zero;
 const auto rate=value(rateWord);const auto currentProduct=_mm_mul_ss(rate,current),previousProduct=_mm_mul_ss(rate,previous);
 MaterialAnimationFrameCandidate result;result.frame=_mm_cvttss_si32(currentProduct)%count;result.previousFrame=_mm_cvttss_si32(previousProduct)%count;
 result.wrapDetected=result.previousFrame>result.frame;
 if(result.wrapDetected){if(wrap)result.invokeWrapCallback=true;else{float countFloat,previousFloat;_mm_store_ss(&countFloat,_mm_cvtsi32_ss(zero,count));_mm_store_ss(&previousFloat,previousProduct);result.invokeWrapCallback=countFloat>previousFloat;result.frame=count-1;}}
 return result;
}
