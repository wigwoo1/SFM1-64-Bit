#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
struct ElapsedFrameClockCandidate {
 uint64_t anchor=0,periodNumerator=1000000000;uint32_t periodDenominator=30;size_t origin=0;
 void setPackedRate(uint32_t rate){auto n=rate&65535u,d=rate>>16;if(!n||!d)throw std::runtime_error("invalid playback rate");periodNumerator=uint64_t(d)*100000;periodDenominator=n;}
 void reset(uint64_t now,size_t frame){anchor=now;origin=frame;}
 uint32_t waitMilliseconds(uint64_t now)const{
  if(now<anchor)throw std::runtime_error("invalid playback clock");
  auto phase=(((now-anchor)%periodNumerator)*periodDenominator)%periodNumerator;
  auto ns=(periodNumerator-phase+periodDenominator-1)/periodDenominator;
  auto ms=(ns+999999)/1000000;return uint32_t(ms>1000?1000:ms);
 }
 size_t sample(uint64_t now,size_t count)const{
  if(!count||now<anchor)throw std::runtime_error("invalid playback clock");
  auto elapsed=now-anchor;
  // Reduce before multiplying so long-running playback cannot overflow.
  auto whole=(elapsed/periodNumerator)%count;
  auto tail=((elapsed%periodNumerator)*periodDenominator)/periodNumerator;
  return (origin%count+whole*periodDenominator+tail)%count;
 }
};
