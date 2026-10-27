#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>

// Owned typed projection of StudioRender10053CD0 selection. No x86 ABI pointers,
// material lookup/retention, proxy execution, clock, renderer or document access.
// Caller authenticates metadata, material identities and HasProxy observations.
// Selection is deduplicated by texture INDEX, never by material identity.
struct StudioProxyPrebindMaterialCandidate {
 uintptr_t identity=0;
 std::optional<bool>hasProxy;
};
struct StudioProxyPrebindModelCandidate {std::vector<int32_t>meshSkinReferences;};
struct StudioProxyPrebindBodyCandidate {
 int32_t base=1;
 std::vector<StudioProxyPrebindModelCandidate>models;
};
struct StudioProxyPrebindInputCandidate {
 std::optional<StudioProxyPrebindMaterialCandidate>forcedMaterial;
 int32_t forcedType=0,contextSkin=0,drawSkin=0,body=0;
 uint32_t textureCount=0,lod=0;
 std::vector<std::vector<int16_t>>skinFamilies;
 std::vector<std::vector<StudioProxyPrebindMaterialCandidate>>lodMaterials;
 std::vector<StudioProxyPrebindBodyCandidate>bodies;
};
struct StudioProxyPrebindVisitCandidate {
 std::optional<uint32_t>textureIndex; // absent only for forced-material path
 uintptr_t material=0;
 bool bind=false;
};
struct StudioProxyPrebindPlanCandidate {
 std::vector<StudioProxyPrebindVisitCandidate>visits;
};
inline bool studioProxyPrebindSelectionCandidate(const StudioProxyPrebindInputCandidate&input,StudioProxyPrebindPlanCandidate&out)noexcept {
 try{
  StudioProxyPrebindPlanCandidate plan;
  if(input.forcedMaterial){
   // Nonzero override type suppresses prebind before consulting model metadata.
   if(!input.forcedType){const auto&m=*input.forcedMaterial;if(!m.identity||!m.hasProxy)return false;plan.visits.push_back({{},m.identity,*m.hasProxy});}
  }else{
   // These are owned validation limits, not claimed native error behavior.
   if(input.textureCount>32768||input.bodies.size()>4096||input.skinFamilies.empty()||input.lod>=input.lodMaterials.size())return false;
   const auto skin=input.contextSkin>0?input.contextSkin:input.drawSkin;
   const auto family=skin>0&&size_t(skin)<input.skinFamilies.size()?size_t(skin):0;
   const auto&remap=input.skinFamilies[family];const auto&materials=input.lodMaterials[input.lod];
   std::vector<uint8_t>seen(input.textureCount,0);size_t meshes=0;
   for(const auto&body:input.bodies){
    if(body.base<=0||body.models.empty()||body.models.size()>4096)return false;
    // Original helper uses signed IDIV twice. Negative remainders are invalid
    // model indices; do not silently reinterpret the body value as unsigned.
    const int64_t selected=(int64_t(input.body)/body.base)%int64_t(body.models.size());
    if(selected<0)return false;const auto&model=body.models[size_t(selected)];
    if(model.meshSkinReferences.size()>(1u<<20)-meshes)return false;meshes+=model.meshSkinReferences.size();
    for(auto reference:model.meshSkinReferences){
     if(reference<0||size_t(reference)>=remap.size())return false;
     const auto texture=remap[size_t(reference)];
     if(texture<0||uint32_t(texture)>=input.textureCount)return false;
     if(seen[size_t(texture)])continue;seen[size_t(texture)]=1;
     if(size_t(texture)>=materials.size())return false;const auto&m=materials[size_t(texture)];
     if(!m.identity||!m.hasProxy)return false;
     plan.visits.push_back({uint32_t(texture),m.identity,*m.hasProxy});
    }
   }
  }
  out.visits.swap(plan.visits);return true;
 }catch(...){return false;}
}
