#pragma once
#include "studio_pixel_constant_submit_candidate.h"
// Recovered one-register transport; callers guarantee four readable words.
template<class Backend>struct StudioSinglePixelTransportCandidate {
 Backend& backend;
 void submitPixel(unsigned index,unsigned /*count*/,const uint32_t* words){
  std::array<uint32_t,4> local{};
  std::memcpy(local.data(),words,sizeof(local));
  studioPixelConstantSubmitCandidate(index,local,backend);
 }
};
