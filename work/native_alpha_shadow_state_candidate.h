#pragma once
#include <cstdint>
#include <xmmintrin.h>
#include <optional>
#include <stdexcept>
#include <cstddef>
// Installed shaderapidx9 AlphaFunc: comparison enum 0..7 maps to 1..8;
// reference is MULSS by 255 then CVTTSS2SI. Out-of-range enums leave state.
struct NativeAlphaShadowStateCandidate{uint32_t comparison=7;int32_t reference=178;uint8_t controls=0;};
// Recovered this+0x6c byte: alpha-test enable owns bit 0x40 only.
inline void setNativeAlphaShadowEnabledCandidate(NativeAlphaShadowStateCandidate&state,uint8_t value){state.controls=static_cast<uint8_t>((state.controls&~0x40u)|((value&1u)<<6));}
inline bool nativeAlphaShadowEnabledCandidate(const NativeAlphaShadowStateCandidate&state){return (state.controls&0x40u)!=0;}
struct NativeAggregateAlphaCandidate{uint32_t comparison;int32_t reference;};
// Alpha-only projection of installed aggregate routine at 0x1002d510.
// Standard source-alpha blending retains a minimum reference even when the
// explicit alpha-test bit is off. Other blend/depth fields remain separate.
inline NativeAggregateAlphaCandidate aggregateNativeAlphaShadowCandidate(const NativeAlphaShadowStateCandidate&state,uint32_t sourceBlend,uint32_t destinationBlend){
 if(nativeAlphaShadowEnabledCandidate(state))return {state.comparison,state.reference};
 return {7,(state.controls&0x20u)&&sourceBlend==5&&destinationBlend==6?1:0};
}
inline bool setNativeAlphaShadowFunctionCandidate(NativeAlphaShadowStateCandidate&state,uint32_t comparison,float reference){
 if(comparison>7)return false;
 const auto product=_mm_mul_ss(_mm_set_ss(reference),_mm_set_ss(255.f));
 state.comparison=comparison+1;state.reference=_mm_cvttss_si32(product);return true;
}
// Installed shader helper skips absent/nonpositive/unordered references.
inline bool applyNativeShaderAlphaReferenceCandidate(NativeAlphaShadowStateCandidate&state,const std::optional<float>&reference){
 return reference&&*reference>0.f?setNativeAlphaShadowFunctionCandidate(state,6,*reference):false;
}

// Non-texture aggregate fields recovered from installed 0x1002d510.
struct NativeAggregateBlendCandidate {
 uint32_t source,destination,operation,separateSource,separateDestination,separateOperation;
 NativeAggregateAlphaCandidate alpha;
 uint8_t secondaryControls;
};
inline NativeAggregateBlendCandidate aggregateNativeBlendShadowCandidate(const NativeAlphaShadowStateCandidate&state,uint8_t secondary,uint32_t source,uint32_t destination,uint32_t operation,uint32_t separateSource,uint32_t separateDestination,uint32_t separateOperation){
 const bool blending=(state.controls&0x20u)!=0;
 NativeAggregateBlendCandidate result{blending?source:2u,blending?destination:1u,blending?operation:1u,(secondary&1u)?separateSource:2u,(secondary&1u)?separateDestination:1u,(secondary&1u)?separateOperation:1u,aggregateNativeAlphaShadowCandidate(state,source,destination),secondary};
 if(blending||!nativeAlphaShadowEnabledCandidate(state))result.secondaryControls=static_cast<uint8_t>(secondary&~8u);
 return result;
}

// Aggregate texture loop copies only bit 0 from source +0x88 to output +0x58.
// Caller owns hardware-stage limits; no fixed native maximum is inferred here.
inline void aggregateNativeTextureStageFlagsCandidate(uint8_t*output,size_t outputSize,const uint8_t*source,size_t sourceSize,size_t stageCount){
 if(stageCount>outputSize||stageCount>sourceSize||(stageCount&&(!output||!source)))throw std::runtime_error("texture stage storage bounds");
 for(size_t i=0;i<stageCount;++i)output[i]=static_cast<uint8_t>((output[i]&0xfeu)|(source[i]&1u));
}
