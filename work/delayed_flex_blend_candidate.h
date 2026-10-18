#pragma once
#include <cmath>
#include <cstddef>
#include <stdexcept>
// Caller owns time, initialization and reset policy. No native timing inferred.
inline void delayedFlexBlendCandidate(const float* current,float* history,std::size_t count,float coefficient){
 if(!std::isfinite(coefficient)||coefficient<0.0f||coefficient>1.0f)
  throw std::runtime_error("invalid delayed flex coefficient");
 if(count&&(!current||!history))throw std::runtime_error("missing delayed flex buffer");
 for(std::size_t i=0;i<count;++i)
  if(!std::isfinite(current[i])||!std::isfinite(history[i]))
   throw std::runtime_error("nonfinite delayed flex input");
 const float inverse=1.0f-coefficient;
 for(std::size_t i=0;i<count;++i){
  const float present=current[i]*inverse;
  const float retained=history[i]*coefficient;
  history[i]=present+retained;
 }
}
