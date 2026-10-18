from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
s=s.replace('const std::function<void(VkImage)>&presentImage={}){','const std::function<void(VkImage)>&presentImage={},const std::function<size_t(size_t)>&selectNextFrame={}){',1)
s=s.replace('for(size_t frame=0;frame<frameCount;++frame){','size_t renderedFrameCount=0;auto advanceFrame=[&](size_t frame){auto next=selectNextFrame?selectNextFrame(frame):frame+1;if(next>frameCount)throw std::runtime_error("frame selection outside range");return next;};\n for(size_t iteration=0,frame=0;frame<frameCount;++iteration,frame=advanceFrame(frame)){')
s=s.replace('if(frame)check(load<PFN_vkResetCommandPool>', 'if(iteration)check(load<PFN_vkResetCommandPool>')
s=s.replace('if(!frame)materialImages.upload(command);if(frame){','if(!iteration)materialImages.upload(command);if(iteration){')
s=s.replace('sequencePathSize&&prefetchFrames&&frame+1<frameCount','sequencePathSize&&prefetchFrames&&!selectNextFrame&&frame+1<frameCount')
s=s.replace('++submissionFenceWaits;pending=false;', '++submissionFenceWaits;++renderedFrameCount;pending=false;')
s=s.replace('retained Vulkan sequence passed frames="<<frameCount', 'retained Vulkan sequence passed frames="<<renderedFrameCount<<" available_frames="<<frameCount')
p.write_text(s)
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text().replace('const std::function<void(VkImage)>&presentImage={}){','const std::function<void(VkImage)>&presentImage={},const std::function<size_t(size_t)>&selectNextFrame={}){',1)
s=s.replace('base,presentImage);','base,presentImage,selectNextFrame);');p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text()
old='size_t presented=0;testVulkanGraphicsPipelineCandidate('
assert old in s
s=s.replace(old,'size_t presented=0;std::function<size_t(size_t)>selectNextFrame;size_t selectionStep=0;char seekTest[8]{};auto seekSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SEEK_TEST",seekTest,sizeof(seekTest));if(seekSize){if(seekSize!=1||seekTest[0]!=\'1\')throw std::runtime_error("seek test option");selectNextFrame=[&](size_t){const size_t sequence[]={0,1,0,0,2};++selectionStep;return selectionStep<5?sequence[selectionStep]:size_t(3);};}testVulkanGraphicsPipelineCandidate(')
s=s.replace('++presented;});','++presented;},selectNextFrame);')
p.write_text(s)
