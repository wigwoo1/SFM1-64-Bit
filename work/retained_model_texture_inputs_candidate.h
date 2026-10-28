#pragma once
#include "model_render_inputs_candidate.h"
#include "material_texture_resource_candidate.h"
#include <memory>

enum class RetainedModelTextureChannelCandidate {Base,Second};
struct RetainedModelTextureSelectionCandidate {
 size_t material=0;
 RetainedModelTextureChannelCandidate channel=RetainedModelTextureChannelCandidate::Base;
 MaterialTextureReferenceCandidate texture;
 uint32_t mip=0,frame=0;
};
// Explicit texture-only preparation for an already admitted renderer input.
// Slots are renderer MATERIAL indices, not MDL texture indices or store IDs.
// Caller must map per-bind snapshots correctly before using this boundary; it
// does not project other proxy writes, infer animation time or admit a shader.
// Requested mip uses the renderer's existing last-mip clamp; frame is explicit
// and strict. Environment/render-target/unavailable resources cannot be decoded.
// Result owns RGBA bytes independently of the VTF service and original inputs.
// Keep/reuse the returned profile identity across frames to reuse GPU uploads;
// this helper does not introduce a per-frame cache or manage GPU retirement.
inline std::shared_ptr<const ModelRenderInputsCandidate>prepareRetainedModelTextureInputsCandidate(
 const ModelRenderInputsCandidate&source,const std::vector<RetainedModelTextureSelectionCandidate>&selections)noexcept{
 try{
  if(selections.empty()||selections.size()>8192||source.materials.size()>32768)return {};
  auto input=selections;std::vector<uint8_t>seen(source.materials.size(),0);
  for(const auto&selection:input){
   if(selection.material>=source.materials.size()||!selection.texture.valid()||selection.texture.isEnvironment()||!selection.texture.resource())return {};
   unsigned bit=0;if(selection.channel==RetainedModelTextureChannelCandidate::Base)bit=1;else if(selection.channel==RetainedModelTextureChannelCandidate::Second)bit=2;else return {};
   if(seen[selection.material]&bit)return {};seen[selection.material]|=uint8_t(bit);
   if(bit==2&&!source.materials[selection.material].unlitTwoTexture&&!source.materials[selection.material].vertexTransition)return {};
   const auto&layout=selection.texture.resource()->layout();if(layout.mips.empty()||selection.frame>=layout.frameCount)return {};
  }
  auto next=std::make_shared<ModelRenderInputsCandidate>(source);
  for(const auto&selection:input){const auto&resource=*selection.texture.resource();const auto&layout=resource.layout();const auto mip=selection.mip<layout.mips.size()?selection.mip:uint32_t(layout.mips.size()-1);
   auto decoded=decodeVtfMipCandidate(resource.bytes(),mip,selection.frame);auto&material=next->materials[selection.material];const bool base=selection.channel==RetainedModelTextureChannelCandidate::Base;
   std::string prefix=base?"$basetexture ":material.unlitTwoTexture?"$texture2 ":"$basetexture2 ";auto&diagnostics=material.source.missingTextureReferences;
   diagnostics.erase(std::remove_if(diagnostics.begin(),diagnostics.end(),[&](const auto&text){return text.compare(0,prefix.size(),prefix)==0;}),diagnostics.end());
   // Authored VMT syntax and shader metadata stay authored. Effective image and
   // VTF flags change; an empty host path avoids claiming a retained logical
   // resource still came from the baseline material's filesystem location.
   if(base){material.image=std::move(decoded);material.baseTextureFlags=layout.flags;material.source.baseTexture.clear();material.source.baseTextureMissing=false;}
   else{material.secondBaseImage=std::move(decoded);material.secondTextureFlags=layout.flags;material.source.baseTexture2.clear();material.source.baseTexture2Missing=false;}
  }
  return next;
 }catch(...){return {};}
}
