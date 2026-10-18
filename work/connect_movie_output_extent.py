from pathlib import Path
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text();s=s.replace('#include "session_frame_rate_candidate.h"','#include "session_frame_rate_candidate.h"\n#include "vulkan_scene_output_extent_candidate.h"',1)
needle='uint32_t resolvedSessionRate=0;'
insert='auto outputExtent=vulkanSceneOutputExtentCandidate();VkPhysicalDeviceProperties outputProperties{};entry<PFN_vkGetPhysicalDeviceProperties>(library,"vkGetPhysicalDeviceProperties")(physical,&outputProperties);const auto&limits=outputProperties.limits;if(outputExtent.width>limits.maxImageDimension2D||outputExtent.height>limits.maxImageDimension2D||outputExtent.width>limits.maxFramebufferWidth||outputExtent.height>limits.maxFramebufferHeight||outputExtent.width>limits.maxViewportDimensions[0]||outputExtent.height>limits.maxViewportDimensions[1])throw std::runtime_error("scene output exceeds device limits");std::cout<<"scene_output_extent="<<outputExtent.width<<"x"<<outputExtent.height<<"\\n";'
assert needle in s;s=s.replace(needle,insert+needle,1)
s=s.replace('presenter.presentImage(image);','presenter.presentImage(image,outputExtent.width,outputExtent.height);',1)
old='},selectNextFrame,frameProvider);'
assert old in s;s=s.replace(old,'},selectNextFrame,frameProvider,outputExtent.width,outputExtent.height);',1);p.write_text(s)
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text();s=s.replace('const RenderFrameProviderCandidate&frameProvider={})','const RenderFrameProviderCandidate&frameProvider={},uint32_t outputWidth=512,uint32_t outputHeight=512)',1)
s=s.replace('VkViewport viewport{0,0,512,512,0,1};VkRect2D scissor{{0,0},{512,512}}','VkViewport viewport{0,0,float(outputWidth),float(outputHeight),0,1};VkRect2D scissor{{0,0},{outputWidth,outputHeight}}',1)
s=s.replace('frameProvider,eyePipeline,eyeLayout);','frameProvider,eyePipeline,eyeLayout,outputWidth,outputHeight);',1);p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s=s.replace('VkPipelineLayout eyeLayout=VK_NULL_HANDLE){','VkPipelineLayout eyeLayout=VK_NULL_HANDLE,uint32_t outputWidth=512,uint32_t outputHeight=512){',1)
s=s.replace('frozenSceneOwner->sample(tick)','frozenSceneOwner->sample(tick,outputWidth,outputHeight)',1)
s=s.replace('frozenSceneOwner->sample(sceneTicks.at(frame))','frozenSceneOwner->sample(sceneTicks.at(frame),outputWidth,outputHeight)',1)
s=s.replace('512*512*4','size_t(outputWidth)*outputHeight*4')
s=s.replace('{512,512,1}','{outputWidth,outputHeight,1}')
s=s.replace('framebufferInfo.width=framebufferInfo.height=512','framebufferInfo.width=outputWidth;framebufferInfo.height=outputHeight')
s=s.replace('{{0,0},{512,512}}','{{0,0},{outputWidth,outputHeight}}')
s=s.replace('(256*512+256)*4','(size_t(outputHeight/2)*outputWidth+outputWidth/2)*4')
s=s.replace('<<" output=512x512 RGBA frame="','<<" output="<<outputWidth<<"x"<<outputHeight<<" RGBA frame="')
p.write_text(s)
p=Path('work/vulkan_swapchain_present_probe_candidate.h');s=p.read_text();s=s.replace('void presentImage(VkImage source=VK_NULL_HANDLE)','void presentImage(VkImage source=VK_NULL_HANDLE,uint32_t sourceWidth=512,uint32_t sourceHeight=512)',1)
s=s.replace('viewportFitCandidate(512,512,extent.width,extent.height)','viewportFitCandidate(sourceWidth,sourceHeight,extent.width,extent.height)',1)
s=s.replace('region.srcOffsets[1]={512,512,1}','region.srcOffsets[1]={int32_t(sourceWidth),int32_t(sourceHeight),1}',1);p.write_text(s)
