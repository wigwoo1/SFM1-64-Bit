#pragma once
#include "vmt_keyvalues_candidate.h"
#include <charconv>
#include <cmath>
#include "native_alpha_shadow_state_candidate.h"
// Separate recovered state metadata; Vulkan still uses the raw cutoff below.
inline std::optional<NativeAlphaShadowStateCandidate> materialNativeAlphaStateCandidate(const VmtNodeCandidate&syntax){
 auto shader=vmtAsciiLowerCandidate(syntax.key);if(shader!="vertexlitgeneric")return {};
 NativeAlphaShadowStateCandidate state;auto flags=vmtValuesCandidate(syntax,"$alphatest");if(flags.size()>1||(!flags.empty()&&flags[0]!="0"&&flags[0]!="1"))throw std::runtime_error("unsupported native alpha test flag");setNativeAlphaShadowEnabledCandidate(state,!flags.empty()&&flags[0]=="1");
 auto values=vmtValuesCandidate(syntax,"$alphatestreference");if(values.size()>1)throw std::runtime_error("duplicate native alpha reference");std::optional<float>reference;if(!values.empty()){float value{};const auto&text=values[0];auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(value)||value<0||value>1)throw std::runtime_error("native alpha reference outside supported finite range");reference=value;}applyNativeShaderAlphaReferenceCandidate(state,reference);return state;
}
// The installed shaderapidx9 shadow reset restores alpha reference 0.7 (178/255).
// Its installed VertexLitGeneric helper skips absent/nonpositive references;
// that leaves the reset state intact. Valve's public SDK declares defaults of
// 0.0 for VertexLitGeneric and 0.7 for UnlitGeneric. Both therefore resolve to
// the installed 8-bit threshold 178 when alpha testing is enabled.
inline float materialAlphaCutoffCandidate(const VmtNodeCandidate&syntax){
 auto flags=vmtValuesCandidate(syntax,"$alphatest");if(flags.empty())return 0;
 if(flags.size()!=1||(flags[0]!="0"&&flags[0]!="1"))throw std::runtime_error("unsupported alpha test flag");if(flags[0]=="0")return 0;
 auto values=vmtValuesCandidate(syntax,"$alphatestreference");if(values.size()>1)throw std::runtime_error("duplicate alpha test reference");float value=0;if(!values.empty()){const auto&text=values[0];auto result=std::from_chars(text.data(),text.data()+text.size(),value);if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||!std::isfinite(value)||value<0||value>1)throw std::runtime_error("alpha test reference must be finite and in [0,1]");}
 if(value<=0){const auto shader=vmtAsciiLowerCandidate(syntax.key);if(shader!="vertexlitgeneric"&&shader!="unlitgeneric")throw std::runtime_error("alpha test default reference unknown for shader "+syntax.key);return 178.f/255.f;}
 return float(static_cast<uint32_t>(value*255.f))/255.f;
}
