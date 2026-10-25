#pragma once
#include "model_render_inputs_candidate.h"
#include <sstream>

// Preflight the verified normal/additive subset before any GPU cache factory.
// This is admission validation, not a complete native material evaluator.
inline void validateNormalTwoTextureMaterialCandidate(const ModelRenderMaterialCandidate&material,float alpha){
 if(!material.unlitTwoTexture)return;
 if(vmtAsciiLowerCandidate(material.source.syntax.key)!="unlittwotexture"||!material.additive||material.translucent||material.vertexTransition||material.alphaCutoff!=0||material.selfIllum.enabled||alpha!=1)
  throw std::runtime_error("two-texture normal additive subset unsupported");
 // The current diagnostic has one linear/repeat sampler and no texture-alpha
 // variant. Admit only known opaque VTF flags (optionally SRGB, 0x40).
 // This deliberately conservative allowlist is not native IsTranslucent.
 if(!material.baseTextureFlags||!material.secondTextureFlags)
  throw std::runtime_error("two-texture VTF flags required before GPU allocation");
 if((*material.baseTextureFlags&~uint32_t(0x40))||(*material.secondTextureFlags&~uint32_t(0x40)))
  throw std::runtime_error("two-texture VTF flags outside verified subset");
 // Authored vertex alpha/color and depth overrides are not transported by
 // this shader. An opaque/unit retained value must not hide those requests.
 for(const char*key:{"$vertexalpha","$vertexcolor","$ignorez"}){
  for(const auto&node:material.source.syntax.children)
   if(node.block&&vmtAsciiLowerCandidate(node.key)==key)
    throw std::runtime_error(std::string("two-texture material flag must be scalar: ")+key);
  auto values=vmtValuesCandidate(material.source.syntax,key);
  if(values.size()>1||(!values.empty()&&values[0]!="0"))
   throw std::runtime_error(std::string("two-texture unsupported material flag: ")+key);
 }
 auto imageValid=[](const VtfRgbaImageCandidate&image){return image.width&&image.height&&uint64_t(image.width)*image.height<=SIZE_MAX/4&&image.rgba.size()==uint64_t(image.width)*image.height*4;};
 if(material.source.baseTextureMissing||material.source.baseTexture2Missing||!imageValid(material.image)||!imageValid(material.secondBaseImage))throw std::runtime_error("two-texture decoded images required");
 for(const auto*transform:{&material.baseTextureTransform,&material.secondTextureTransform})for(float value:*transform)if(!std::isfinite(value))throw std::runtime_error("two-texture transform nonfinite");
 for(const char*key:{"$color","$color2","$srgbtint"}){
  auto values=vmtValuesCandidate(material.source.syntax,key);if(values.size()>1)throw std::runtime_error("duplicate two-texture modulation");if(values.empty())continue;
  auto text=values[0];const auto begin=text.find_first_not_of(" \t\r\n"),end=text.find_last_not_of(" \t\r\n");
  if(begin==std::string::npos)throw std::runtime_error("empty two-texture tint");text=text.substr(begin,end-begin+1);
  if(text.front()=='['){if(text.back()!=']')throw std::runtime_error("unterminated two-texture tint");text=text.substr(1,text.size()-2);}
  if(text.find_first_of("[]")!=std::string::npos)throw std::runtime_error("two-texture tint brackets");
  std::istringstream input(text);float value;size_t count=0;while(input>>value){if(value!=1)throw std::runtime_error("two-texture tint variant unfinished");++count;}
  if(!input.eof()||(count!=3&&count!=4))throw std::runtime_error("two-texture tint encoding unsupported");
 }
 auto cloak=vmtValuesCandidate(material.source.syntax,"$cloakfactor");if(cloak.size()>1)throw std::runtime_error("duplicate two-texture cloak factor");
 if(!cloak.empty()){std::istringstream input(cloak[0]);float value;std::string extra;if(!(input>>value)||value!=0||(input>>extra))throw std::runtime_error("two-texture cloak variant unfinished");}
}
