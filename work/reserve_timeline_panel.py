from pathlib import Path
p=Path('work/vulkan_swapchain_present_probe_candidate.h');s=p.read_text(encoding='utf-8')
old='VkExtent2D extent;';assert old in s;s=s.replace(old,'VkExtent2D extent;uint32_t reservedBottom=0;',1)
old='VkExtent2D size):device';assert old in s;s=s.replace(old,'VkExtent2D size,uint32_t bottom=0):device',1)
old='extent(size){auto get=';assert old in s;s=s.replace(old,'extent(size),reservedBottom(bottom){if(bottom>=size.height&&bottom)throw std::runtime_error("timeline panel exceeds presentation height");auto get=',1)
old='viewportFitCandidate(sourceWidth,sourceHeight,extent.width,extent.height)';assert old in s;s=s.replace(old,'viewportFitCandidate(sourceWidth,sourceHeight,extent.width,extent.height-reservedBottom)',1)
old='<<" background=black\\n"';assert old in s;s=s.replace(old,'<<" background=black reserved_bottom="<<reservedBottom<<"\\n"',1);p.write_text(s,encoding='utf-8')
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text(encoding='utf-8');old='RetainedSwapchainPresenterCandidate presenter(library,device,family,swapchain,images,extent);';assert old in s;s=s.replace(old,'RetainedSwapchainPresenterCandidate presenter(library,device,family,swapchain,images,extent,vulkanTimelineSlider?40:0);',1);p.write_text(s,encoding='utf-8')
