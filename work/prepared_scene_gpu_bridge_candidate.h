#pragma once
#include "vulkan_prepared_scene_candidate.h"
#include "retained_scene_material_gpu_cache_candidate.h"
#include "material_gamma_table_candidate.h"
#include "material_sdk_tint_gamma_candidate.h"
#include "material_selfillum_draw_push_candidate.h"
#include "normal_two_texture_material_candidate.h"
// Retained CPU inputs and GPU cache must outlive submitted frames. Buffer upload
// and render-pass ownership remain with the caller; descriptor uploads precede
// the render pass on the same queue.
template<class Images>
class PreparedSceneGpuBridgeCandidate {
 RetainedSceneMaterialGpuCacheCandidate<Images>&materials_;
public:
 explicit PreparedSceneGpuBridgeCandidate(RetainedSceneMaterialGpuCacheCandidate<Images>&materials):materials_(materials){}
 template<class Prepared>
 VulkanPreparedSceneResourcesCandidate resources(const Prepared&scene,VkBuffer vertices,VkBuffer indices,VkDeviceSize vertexCapacity,VkDeviceSize indexCapacity,VkPipeline opaque,VkPipeline translucent,VkPipelineLayout layout,std::optional<NativeGammaIndexRounding>selfIllumRounding=std::nullopt,VkPipeline twoTexturePipeline=VK_NULL_HANDLE){
  if(scene.upload.vertices.size()>vertexCapacity/sizeof(SceneGpuVertexCandidate)||scene.upload.indices.size()>indexCapacity/sizeof(uint32_t))throw std::runtime_error("scene GPU bridge buffer capacity");
  if(scene.materialAlpha.size()!=scene.upload.materials.size())throw std::runtime_error("scene GPU bridge alpha coverage");
  std::vector<MaterialUvTransformCandidate>uvTransforms;uvTransforms.reserve(scene.upload.materials.size());
  for(const auto&address:scene.upload.materials)uvTransforms.push_back(scene.materialInputs.at(address.model).materials.at(address.material).baseTextureTransform);
  std::vector<std::array<float,16>>selfIllumPush;
  if(selfIllumRounding){
   const auto&gammaTable=materialGammaTableSharedCandidate();selfIllumPush.reserve(scene.upload.materials.size());
   for(size_t slot=0;slot<scene.upload.materials.size();++slot){
    const auto&address=scene.upload.materials[slot];const auto&material=scene.materialInputs.at(address.model).materials.at(address.material);
    const auto&mask=material.selfIllumMaskImage;
    bool maskAvailable=mask.width&&mask.height&&uint64_t(mask.width)*mask.height<=SIZE_MAX/4&&mask.rgba.size()==uint64_t(mask.width)*mask.height*4;
    auto tint=materialSdkTintGammaToLinearCandidate(material.selfIllum.tint,gammaTable,*selfIllumRounding);
    auto draw=materialSelfIllumDrawPushCandidate({},scene.materialAlpha.at(slot),material.alphaCutoff,material.selfIllum,maskAvailable,tint);
    std::array<float,16>payload{};payload[0]=draw.alpha;payload[1]=draw.alphaCutoff;payload[2]=draw.enabled;payload[3]=draw.separateMask;payload[4]=draw.linearTint[0];payload[5]=draw.linearTint[1];payload[6]=draw.linearTint[2];auto transform=scene.materialInputs.at(address.model).materials.at(address.material).baseTextureTransform;std::copy(transform.begin(),transform.begin()+4,payload.begin()+8);std::copy(transform.begin()+4,transform.end(),payload.begin()+12);selfIllumPush.push_back(payload);
   }
  }
  if(twoTexturePipeline){
   if(selfIllumRounding)throw std::runtime_error("two-texture and selfillum diagnostic conflict");
   for(size_t slot=0;slot<scene.upload.materials.size();++slot){const auto&address=scene.upload.materials[slot];validateNormalTwoTextureMaterialCandidate(scene.materialInputs.at(address.model).materials.at(address.material),scene.materialAlpha[slot]);}
  }
  auto sets=materials_.descriptors(scene.upload,scene.materialInputs);
  std::vector<float>cutoffs;cutoffs.reserve(scene.upload.materials.size());for(const auto&address:scene.upload.materials)cutoffs.push_back(scene.materialInputs.at(address.model).materials.at(address.material).alphaCutoff);
  std::vector<float>vertexLighting;vertexLighting.reserve(scene.upload.materials.size());for(const auto&address:scene.upload.materials)vertexLighting.push_back(scene.materialInputs.at(address.model).materials.at(address.material).vertexLighting?1.f:0.f);
  std::vector<float>vertexTransition;vertexTransition.reserve(scene.upload.materials.size());for(const auto&address:scene.upload.materials)vertexTransition.push_back(scene.materialInputs.at(address.model).materials.at(address.material).vertexTransition?1.f:0.f);
  VulkanPreparedSceneResourcesCandidate result{vertices,indices,vertexCapacity,indexCapacity,opaque,translucent,layout,std::move(sets),std::move(cutoffs),std::move(uvTransforms),std::move(selfIllumPush),std::move(vertexLighting),std::move(vertexTransition)};
  if(twoTexturePipeline){
   if(selfIllumRounding)throw std::runtime_error("two-texture and selfillum diagnostic conflict");
   result.materialPipelines.resize(scene.upload.materials.size());result.twoTextureSlots.resize(scene.upload.materials.size());result.secondTextureTransform.assign(scene.upload.materials.size(),identityMaterialUvTransformCandidate());
   for(size_t slot=0;slot<scene.upload.materials.size();++slot){const auto&address=scene.upload.materials[slot];const auto&material=scene.materialInputs.at(address.model).materials.at(address.material);if(!material.unlitTwoTexture)continue;
    result.materialPipelines[slot]=twoTexturePipeline;result.twoTextureSlots[slot]=1;result.secondTextureTransform[slot]=material.secondTextureTransform;
   }
  }
  return result;
 }
 void recordMaterialUploads(VkCommandBuffer command){materials_.recordUploads(command);}
 template<class Prepared>
 void record(const Prepared&scene,const VulkanSceneCommandFunctionsCandidate&functions,PFN_vkCmdPushConstants push,VkCommandBuffer command,const VulkanPreparedSceneResourcesCandidate&resources){recordVulkanPreparedSceneCandidate(functions,push,command,scene.upload,scene.camera,scene.materialAlpha,resources);}
 void submitted(){materials_.submitted();}void discardRecording(){materials_.discardRecording();}
};
