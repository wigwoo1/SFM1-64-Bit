#pragma once
#include "studio_presentation_first_pass_candidate.h"
#include "studio_presentation_windowed_candidate.h"
template<class Services>void studioPresentationBuilderCandidate(
 D3DPRESENT_PARAMETERS& parameters,unsigned char& ownerFlags,unsigned& depthBits,
 HWND window,unsigned adapter,unsigned priorFormat,StudioPresentationDescriptorCandidate& descriptor,
 bool initial,Services& services){
 const auto display=studioPresentationFirstPassCandidate(adapter,descriptor,services);
 if(initial)return;
 studioPresentationWindowedCandidate(parameters,ownerFlags,depthBits,window,adapter,priorFormat,descriptor,
  static_cast<int32_t>(display[1]),static_cast<int32_t>(display[2]),services);
}
