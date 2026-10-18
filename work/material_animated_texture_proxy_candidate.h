#pragma once
#include "vmt_keyvalues_candidate.h"
#include <optional>
#include <charconv>
#include <cmath>
struct MaterialAnimatedTextureProxyCandidate {std::string textureVariable,frameVariable;std::optional<float>frameRate;std::optional<int32_t>animationNoWrap;};
// Extract configuration only. No Source clock/default/playback parity implied.
inline std::vector<MaterialAnimatedTextureProxyCandidate>materialAnimatedTextureProxiesCandidate(const VmtNodeCandidate&root){
 std::vector<MaterialAnimatedTextureProxyCandidate>result;bool seen=false;
 for(const auto&proxies:root.children)if(vmtAsciiLowerCandidate(proxies.key)=="proxies"){
  if(seen||!proxies.block)throw std::runtime_error("material proxy block shape");seen=true;
  for(const auto&node:proxies.children)if(vmtAsciiLowerCandidate(node.key)=="animatedtexture"){
   if(!node.block)throw std::runtime_error("animated texture proxy block required");
   auto single=[&](const std::string&key){auto values=vmtValuesCandidate(node,key);if(values.size()!=1||values[0].empty())throw std::runtime_error("animated texture proxy field "+key);return values[0];};
   MaterialAnimatedTextureProxyCandidate entry;entry.textureVariable=single("animatedtexturevar");entry.frameVariable=single("animatedtextureframenumvar");
   const auto rates=vmtValuesCandidate(node,"animatedtextureframerate");if(rates.size()>1)throw std::runtime_error("duplicate animation rate");if(!rates.empty()){float rate;const auto&text=rates.front();auto parsed=std::from_chars(text.data(),text.data()+text.size(),rate);if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(rate))throw std::runtime_error("invalid animation rate");entry.frameRate=rate;}
   const auto wraps=vmtValuesCandidate(node,"animationnowrap");if(wraps.size()>1)throw std::runtime_error("duplicate no-wrap setting");if(!wraps.empty()){int32_t flag;const auto&text=wraps.front();auto parsed=std::from_chars(text.data(),text.data()+text.size(),flag);if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size())throw std::runtime_error("invalid no-wrap setting");entry.animationNoWrap=flag;}
   for(const auto&prior:result)if(vmtAsciiLowerCandidate(prior.frameVariable)==vmtAsciiLowerCandidate(entry.frameVariable))throw std::runtime_error("ambiguous animated frame variable");
   result.push_back(std::move(entry));
  }
 }
 return result;
}
