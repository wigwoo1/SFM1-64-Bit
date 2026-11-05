#pragma once
#include <vulkan/vulkan.h>
#include "scene_gpu_upload_candidate.h"
#include <functional>
// Pure selection only: receives the pass/material fallback. Called exactly once
// per draw before recording starts; null results or exceptions emit no commands.
using VulkanScenePipelineSelectorCandidate=std::function<VkPipeline(const SceneGpuDrawCandidate&,VkPipeline)>;
struct VulkanSceneCommandFunctionsCandidate {
 PFN_vkCmdBindPipeline pipeline;
 PFN_vkCmdBindDescriptorSets descriptors;
 PFN_vkCmdBindVertexBuffers vertices;
 PFN_vkCmdBindIndexBuffer indices;
 PFN_vkCmdDrawIndexed draw;
};
// Caller owns buffers/descriptors and render-pass lifetime, and sets camera constants.
template<class Scene>
inline void recordVulkanSceneCommandsCandidate(const VulkanSceneCommandFunctionsCandidate&fn,VkCommandBuffer command,const Scene&scene,VkBuffer vertices,VkBuffer indices,VkPipeline opaque,VkPipeline translucent,VkPipelineLayout layout,const std::vector<VkDescriptorSet>&materials,const std::function<void(const SceneGpuDrawCandidate&)>&beforeDraw={},const std::vector<VkPipeline>&materialPipelines={},const VulkanScenePipelineSelectorCandidate&drawPipeline={}){
 if(!fn.pipeline||!fn.descriptors||!fn.vertices||!fn.indices||!fn.draw||!command||!vertices||!indices||!opaque||!translucent||!layout)throw std::runtime_error("scene Vulkan command resources");
 if(materials.size()!=scene.materials.size())throw std::runtime_error("scene Vulkan descriptor count");
 if(!materialPipelines.empty()&&materialPipelines.size()!=materials.size())throw std::runtime_error("scene Vulkan material pipeline count");
 auto validate=[&](const auto&draws){for(const auto&draw:draws){if(draw.materialSlot>=materials.size()||!materials[draw.materialSlot])throw std::runtime_error("scene Vulkan descriptor slot");const auto&address=scene.materials[draw.materialSlot];if(address.model!=draw.model||address.material!=draw.materialIndex)throw std::runtime_error("scene Vulkan material identity");if(draw.firstIndex>scene.indices.size()||draw.indexCount>scene.indices.size()-draw.firstIndex||draw.indexCount%3)throw std::runtime_error("scene Vulkan index span");for(size_t n=0;n<draw.indexCount;++n)if(scene.indices[draw.firstIndex+n]>=scene.vertices.size())throw std::runtime_error("scene Vulkan vertex index");}};
 validate(scene.opaque);validate(scene.translucent);
 auto fallback=[&](const auto&draw,VkPipeline pipeline){return materialPipelines.empty()||!materialPipelines[draw.materialSlot]?pipeline:materialPipelines[draw.materialSlot];};
 std::vector<VkPipeline>opaqueChoices,translucentChoices;
 if(drawPipeline){auto select=[&](const auto&draws,VkPipeline pipeline,auto&choices){choices.reserve(draws.size());for(const auto&draw:draws){const auto selected=drawPipeline(draw,fallback(draw,pipeline));if(!selected)throw std::runtime_error("scene Vulkan per-draw pipeline missing");choices.push_back(selected);}};select(scene.opaque,opaque,opaqueChoices);select(scene.translucent,translucent,translucentChoices);}
 if(scene.opaque.empty()&&scene.translucent.empty())return;
 VkDeviceSize offset=0;fn.vertices(command,0,1,&vertices,&offset);fn.indices(command,indices,0,VK_INDEX_TYPE_UINT32);
 // beforeDraw may bind compatible descriptors/streams/constants, but must not
 // change the pipeline: only this loop owns the bound-pipeline cache.
 auto record=[&](const auto&draws,VkPipeline pipeline,const auto&choices){if(draws.empty())return;fn.pipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);VkPipeline bound=pipeline;size_t n=0;for(const auto&draw:draws){const auto selected=drawPipeline?choices.at(n++):fallback(draw,pipeline);if(selected!=bound){fn.pipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,selected);bound=selected;}auto set=materials[draw.materialSlot];fn.descriptors(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);if(beforeDraw)beforeDraw(draw);fn.draw(command,draw.indexCount,1,draw.firstIndex,0,0);}};
 record(scene.opaque,opaque,opaqueChoices);record(scene.translucent,translucent,translucentChoices);
}
