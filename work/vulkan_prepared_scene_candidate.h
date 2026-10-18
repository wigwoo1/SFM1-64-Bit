#pragma once
#include "vulkan_scene_commands_candidate.h"
#include "render_camera_candidate.h"
#include <cmath>
struct VulkanPreparedSceneResourcesCandidate {
 VkBuffer vertices,indices;
 VkDeviceSize vertexCapacity,indexCapacity;
 VkPipeline opaque,translucent;
 VkPipelineLayout layout;
 std::vector<VkDescriptorSet>materials;
 std::vector<float>alphaCutoff;
 std::vector<MaterialUvTransformCandidate>baseTextureTransform;
 // Optional fragment payload from offset96: alpha/cutoff/selfillum flags/tint,
 // padding, and two float4 rows for base UV mapping (64 bytes total).
 std::vector<std::array<float,16>>selfIllumPush;
 std::vector<float>vertexLighting;
 std::vector<float>vertexTransition;
 // Optional per-material pipelines preserve the group's default when null.
 // Two-texture layouts must expose fragment push bytes96..175 (80 bytes).
 std::vector<VkPipeline>materialPipelines;
 std::vector<uint8_t>twoTextureSlots;
 std::vector<MaterialUvTransformCandidate>secondTextureTransform;

};
template<class Scene>
inline void recordVulkanPreparedSceneCandidate(const VulkanSceneCommandFunctionsCandidate&fn,PFN_vkCmdPushConstants push,VkCommandBuffer command,const Scene&upload,const RenderCameraCandidate&camera,const std::vector<float>&alpha,const VulkanPreparedSceneResourcesCandidate&resources){
 if(!push)throw std::runtime_error("scene push constant function required");
 if(upload.vertices.size()>resources.vertexCapacity/sizeof(SceneGpuVertexCandidate)||upload.indices.size()>resources.indexCapacity/sizeof(uint32_t))throw std::runtime_error("prepared scene buffer capacity");
 if(alpha.size()!=upload.materials.size())throw std::runtime_error("prepared scene alpha count");
 for(float value:alpha)if(!std::isfinite(value)||value<0||value>1)throw std::runtime_error("prepared scene alpha range");
 for(float value:camera.rows)if(!std::isfinite(value))throw std::runtime_error("prepared scene camera nonfinite");
 if(!resources.alphaCutoff.empty()&&resources.alphaCutoff.size()!=upload.materials.size())throw std::runtime_error("prepared scene cutoff count");for(float value:resources.alphaCutoff)if(!std::isfinite(value)||value<0||value>1)throw std::runtime_error("prepared scene cutoff range");

 if(!resources.materialPipelines.empty()&&resources.materialPipelines.size()!=upload.materials.size())throw std::runtime_error("prepared scene pipeline coverage");
 if(!resources.twoTextureSlots.empty()&&resources.twoTextureSlots.size()!=upload.materials.size())throw std::runtime_error("prepared scene two-texture coverage");
 if(!resources.secondTextureTransform.empty()&&resources.secondTextureTransform.size()!=upload.materials.size())throw std::runtime_error("prepared scene second UV coverage");
 for(const auto&values:resources.secondTextureTransform)for(float value:values)if(!std::isfinite(value))throw std::runtime_error("prepared scene second UV nonfinite");
 for(size_t slot=0;slot<resources.twoTextureSlots.size();++slot){
  if(resources.twoTextureSlots[slot]>1)throw std::runtime_error("prepared scene two-texture flag");
  if(resources.twoTextureSlots[slot]){
   if(resources.materialPipelines.empty()||!resources.materialPipelines[slot]||resources.secondTextureTransform.empty()||!resources.selfIllumPush.empty())throw std::runtime_error("prepared scene two-texture resources");
   if(alpha[slot]!=1||(!resources.alphaCutoff.empty()&&resources.alphaCutoff[slot]!=0))throw std::runtime_error("two-texture alpha variant unfinished");
  }
 }
 bool cameraPushed=false;
 if(!resources.baseTextureTransform.empty()&&resources.baseTextureTransform.size()!=upload.materials.size())throw std::runtime_error("prepared scene UV transform coverage");for(const auto&values:resources.baseTextureTransform)for(float value:values)if(!std::isfinite(value))throw std::runtime_error("prepared scene UV transform nonfinite");
 if(!resources.selfIllumPush.empty()&&resources.selfIllumPush.size()!=upload.materials.size())throw std::runtime_error("prepared scene selfillum push coverage");
 if(!resources.vertexLighting.empty()&&resources.vertexLighting.size()!=upload.materials.size())throw std::runtime_error("prepared scene vertex-lighting coverage");for(float value:resources.vertexLighting)if(value!=0&&value!=1)throw std::runtime_error("prepared scene vertex-lighting flag");
 if(!resources.vertexTransition.empty()&&resources.vertexTransition.size()!=upload.materials.size())throw std::runtime_error("prepared scene vertex-transition coverage");for(float value:resources.vertexTransition)if(value!=0&&value!=1)throw std::runtime_error("prepared scene vertex-transition flag");
 for(const auto&values:resources.selfIllumPush){for(float value:values)if(!std::isfinite(value))throw std::runtime_error("prepared scene selfillum push nonfinite");if(values[0]<0||values[0]>1||values[1]<0||values[1]>1||(values[2]!=0&&values[2]!=1)||(values[3]!=0&&values[3]!=1)||values[4]<0||values[5]<0||values[6]<0)throw std::runtime_error("prepared scene selfillum push range");}
 recordVulkanSceneCommandsCandidate(fn,command,upload,resources.vertices,resources.indices,resources.opaque,resources.translucent,resources.layout,resources.materials,[&](const SceneGpuDrawCandidate&draw){
  if(!cameraPushed){push(command,resources.layout,VK_SHADER_STAGE_VERTEX_BIT,0,64,camera.rows.data());cameraPushed=true;}
  const auto transform=resources.baseTextureTransform.empty()?identityMaterialUvTransformCandidate():resources.baseTextureTransform.at(draw.materialSlot);
  if(!resources.twoTextureSlots.empty()&&resources.twoTextureSlots.at(draw.materialSlot)){
   float values[20]{1,0,0,1};std::copy(transform.begin(),transform.begin()+4,values+4);std::copy(transform.begin()+4,transform.end(),values+8);
   const auto&second=resources.secondTextureTransform.at(draw.materialSlot);std::copy(second.begin(),second.begin()+4,values+12);std::copy(second.begin()+4,second.end(),values+16);
   push(command,resources.layout,VK_SHADER_STAGE_FRAGMENT_BIT,96,80,values);
  }
  else if(!resources.selfIllumPush.empty()){auto values=resources.selfIllumPush.at(draw.materialSlot);values[0]=alpha.at(draw.materialSlot);values[7]=resources.vertexLighting.empty()?0.f:resources.vertexLighting.at(draw.materialSlot);std::copy(transform.begin(),transform.begin()+4,values.begin()+8);std::copy(transform.begin()+4,transform.end(),values.begin()+12);push(command,resources.layout,VK_SHADER_STAGE_FRAGMENT_BIT,96,64,values.data());}
  else{float values[12]{alpha.at(draw.materialSlot),resources.alphaCutoff.empty()?0.f:resources.alphaCutoff.at(draw.materialSlot),resources.vertexLighting.empty()?0.f:resources.vertexLighting.at(draw.materialSlot),resources.vertexTransition.empty()?0.f:resources.vertexTransition.at(draw.materialSlot)};std::copy(transform.begin(),transform.begin()+4,values+4);std::copy(transform.begin()+4,transform.end(),values+8);push(command,resources.layout,VK_SHADER_STAGE_FRAGMENT_BIT,96,48,values);}
 },resources.materialPipelines);
}
