from pathlib import Path
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text()
old='pipelineInfo.pViewportState=&viewportInfo;'
new='pipelineInfo.pViewportState=&viewportInfo;VkDynamicState dynamicStates[2]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamicInfo{};dynamicInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;dynamicInfo.dynamicStateCount=2;dynamicInfo.pDynamicStates=dynamicStates;pipelineInfo.pDynamicState=&dynamicInfo;'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='beginPass(command,&renderInfo,VK_SUBPASS_CONTENTS_INLINE);'
new='beginPass(command,&renderInfo,VK_SUBPASS_CONTENTS_INLINE);VkViewport frameViewport{0,0,float(outputWidth),float(outputHeight),0,1};VkRect2D frameScissor{{0,0},{outputWidth,outputHeight}};load<PFN_vkCmdSetViewport>(library,"vkCmdSetViewport")(command,0,1,&frameViewport);load<PFN_vkCmdSetScissor>(library,"vkCmdSetScissor")(command,0,1,&frameScissor);'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
