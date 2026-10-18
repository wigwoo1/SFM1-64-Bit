#pragma once
#include "studio_shader_color_select_candidate.h"
#include "studio_shader_color_conversion_candidate.h"
// Partial slot264 color branch for verified modes1..6 only. Third argument
// feeds1000fc90; fourth argument gates whether that helper is called.
// Remaining float-state uploads, side effects and tail submission are separate.
template<class Ops>uint32_t studioShaderColorPipelineCandidate(uint32_t mode,std::array<uint8_t,3> live,
 uint32_t conversionFlag,uint32_t bypassFlag,Ops& ops){
 const auto selected=studioShaderColorSelectCandidate(mode,live,bypassFlag);
 if(!selected.convert)return selected.packed;
 return studioShaderColorConversionCandidate(selected.rgb,conversionFlag,ops);
}
