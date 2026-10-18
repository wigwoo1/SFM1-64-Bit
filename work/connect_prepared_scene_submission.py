from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include "vulkan_scene_commands_candidate.h"','#include "vulkan_prepared_scene_candidate.h"')
old='recordVulkanSceneCommandsCandidate({bindPipeline,bindDescriptors,bindVertices,bindIndices,draw},command,sceneView,vertexBuffer,indexBuffer,pipeline,translucentPipeline,layout,sets,[&](const SceneGpuDrawCandidate&draw){float alpha=currentMaterialAlpha.at(draw.materialSlot);load<PFN_vkCmdPushConstants>(library,"vkCmdPushConstants")(command,layout,VK_SHADER_STAGE_FRAGMENT_BIT,96,4,&alpha);});'
new='recordVulkanPreparedSceneCandidate({bindPipeline,bindDescriptors,bindVertices,bindIndices,draw},load<PFN_vkCmdPushConstants>(library,"vkCmdPushConstants"),command,sceneView,camera,currentMaterialAlpha,{vertexBuffer,indexBuffer,vertices.size()*sizeof(Vertex),indices.size()*sizeof(uint32_t),pipeline,translucentPipeline,layout,std::move(sets)});'
if s.count(old)!=1:raise RuntimeError('command bridge marker')
p.write_text(s.replace(old,new))
