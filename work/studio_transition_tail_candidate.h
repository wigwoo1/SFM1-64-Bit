#pragma once
#include "studio_shader_color_select_candidate.h"
#include <cstring>
// Logical slot264 tail for input0..6. Ops bridges the recovered slot267,
// auxiliary100144a0, conversion1000fc90 and pixel100167b0 helpers.
template<class Ops>void studioTransitionTailCandidate(uint32_t input,uint32_t selected,uint32_t conversionFlag,uint32_t bypassFlag,Ops& ops){
 if(input>6)throw std::invalid_argument("unverified transition tail mode");
 if(!input){
  ops.transitionByte(0);
  const auto index=ops.auxiliaryIndex();if(index!=0xffffffff)ops.auxiliary(index,0);
  return;
 }
 ops.transitionByte(selected==0&&ops.currentState()!=0?1u:0u);
 const auto index=ops.auxiliaryIndex();if(index!=0xffffffff)ops.auxiliary(index,input);
 const auto color=studioShaderColorSelectCandidate(input,ops.liveColor(),bypassFlag);
 ops.publishConversionDisabled(!color.convert);
 const auto packed=color.convert?ops.convert(color.rgb,conversionFlag):color.packed;
 const uint32_t normalizationBits=0x3b808081;float normalization=0;std::memcpy(&normalization,&normalizationBits,4);
 std::array<float,3> normalized{};for(unsigned i=0;i<3;++i)normalized[i]=float(color.rgb[i])*normalization;
 ops.publishNormalized(normalized);
 if(selected){ops.pixelHelper();return;}
 if(ops.packedCache()!=packed){ops.publishPacked(packed);ops.renderState(34,packed,0);}
}
