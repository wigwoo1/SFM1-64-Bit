#pragma once
#include "vmt_keyvalues_candidate.h"
#include <charconv>
#include <cstdint>
#include <stdexcept>
struct MaterialColorSpaceCandidate {bool baseSrgb=true,outputSrgb=true;};
// Recovered generic-helper conditional policy, not universal shader behavior.
// Integer1 disables conversion; absent/other integer values retain conversion.
// Unsupported authored representations are rejected instead of guessed.
inline MaterialColorSpaceCandidate materialColorSpaceCandidate(const VmtNodeCandidate&syntax){
 auto enabled=[&](const char*key){
  auto values=vmtValuesCandidate(syntax,key);
  if(values.size()>1)throw std::runtime_error("duplicate material color-space parameter");
  if(values.empty())return true;
  const auto&text=values.front();int32_t value=0;
  auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
  if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size())throw std::runtime_error("unsupported material color-space integer");
  return value!=1;
 };
 return {enabled("$gammacolorread"),enabled("$linearwrite")};
}
