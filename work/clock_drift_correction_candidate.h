#pragma once
#include "clock_drift_mean_candidate.h"
#include "clock_drift_rate_candidate.h"
#include "clock_drift_history_candidate.h"
#include "clock_correction_gate_candidate.h"
extern "C" void clockDriftDurationAddX87(const uint32_t*,const uint32_t*,uint32_t*);
// Explicit predicate results replace unresolved interface dispatch. Logging excluded.
inline uint32_t clockDriftCorrectionCandidate(std::array<uint32_t,16>&history,
 uint32_t durationWord,uint32_t intervalWord,uint32_t minWord,uint32_t maxWord,
 uint32_t amountWord,bool enabled,bool demoPredicate,bool threaded){
 uint32_t correctionWord=0;
 if(enabled&&!demoPredicate){
  uint32_t driftWord=clockDriftMeanIntervalCandidate(history,intervalWord);
  float drift,minimum,duration;std::memcpy(&drift,&driftWord,4);std::memcpy(&minimum,&minWord,4);std::memcpy(&duration,&durationWord,4);
  auto scaled=_mm_mul_ss(_mm_set_ss(drift),_mm_set_ss(1000));
  auto minimumValue=_mm_set_ss(minimum);auto negMask=_mm_castsi128_ps(_mm_set_epi32(0,0,0,int32_t(0x80000000u)));
  auto negativeDrift=_mm_xor_ps(_mm_set_ss(drift),negMask);auto correction=_mm_setzero_ps();
  bool positive=_mm_comigt_ss(scaled,minimumValue)!=0;
  bool negative=!positive&&_mm_comigt_ss(_mm_xor_ps(minimumValue,negMask),scaled)!=0;
  if(positive||negative){
   auto rateInput=positive?scaled:_mm_xor_ps(scaled,negMask);
   float rateValue;_mm_store_ss(&rateValue,rateInput);uint32_t rateWord;std::memcpy(&rateWord,&rateValue,4);
   rateWord=clockDriftRateCandidate(rateWord,minWord,maxWord,amountWord);
   if(positive)rateWord^=0x80000000u;
   std::memcpy(&rateValue,&rateWord,4);correction=_mm_mul_ss(_mm_set_ss(rateValue),_mm_set_ss(duration));
   if(positive){if(!_mm_comigt_ss(correction,negativeDrift))correction=negativeDrift;}
   else{if(!_mm_comigt_ss(negativeDrift,correction))correction=negativeDrift;}
  }
  if(threaded)correction=negativeDrift;
  float correctionValue;_mm_store_ss(&correctionValue,correction);std::memcpy(&correctionWord,&correctionValue,4);
  clockDriftHistoryCandidate(history,correctionWord,intervalWord);
 }
 uint32_t result;clockDriftDurationAddX87(&correctionWord,&durationWord,&result);return result;
}
// Owned configuration path; object dispatch/lifecycle is supplied by caller.
inline uint32_t clockDriftCorrectionWithNetworkStateCandidate(std::array<uint32_t,16>&history,
 uint32_t durationWord,uint32_t intervalWord,uint32_t minWord,uint32_t maxWord,
 uint32_t amountWord,int32_t state,uint32_t networkWord,int32_t enabled,
 bool demoPredicate,bool threaded){
 return clockDriftCorrectionCandidate(history,durationWord,intervalWord,minWord,maxWord,
  amountWord,clockCorrectionGateCandidate(state,networkWord,enabled),demoPredicate,threaded);
}
