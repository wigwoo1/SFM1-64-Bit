from pathlib import Path
for name in ('vulkan_model_draw_candidate.h','vulkan_graphics_pipeline_candidate.h'):
 p=Path('work',name);s=p.read_text();s=s.replace('std::function<void(VkImage)>','std::function<void(VkImage,uint32_t,uint32_t)>');p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text();old='[&](VkImage image){presenter.presentImage(image,outputExtent.width,outputExtent.height);';new='[&](VkImage image,uint32_t width,uint32_t height){presenter.presentImage(image,width,height);';assert old in s;s=s.replace(old,new,1);p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='if(GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_OUTPUT",nullptr,0)){auto dimensions=sessionMovieDimensionsCandidate(graph,root);if(dimensions.width!=outputWidth||dimensions.height!=outputHeight)throw std::runtime_error("movie dimensions changed; target rebuild required");}'
assert old in s;s=s.replace(old,'',1)
old='auto scene=frozenSceneOwner->sample(sceneTicks.at(frame),outputWidth,outputHeight);'
new='auto scene=movieOutput?frozenSceneOwner->sampleMovie(sceneTicks.at(frame)):frozenSceneOwner->sample(sceneTicks.at(frame),initialOutputWidth,initialOutputHeight);'
assert old in s;s=s.replace(old,new,1)
old='std::future<PreparedDrawFrameCandidate>nextPreparedFrame;'
new='const bool movieOutput=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_OUTPUT",nullptr,0)!=0;const uint32_t initialOutputWidth=outputWidth,initialOutputHeight=outputHeight;std::future<PreparedDrawFrameCandidate>nextPreparedFrame;'
assert old in s;s=s.replace(old,new,1)
s=s.replace('geometryAllocations=1;','geometryAllocations=1,renderTargetAllocations=1;',1)
old='}vertices.resize(frozenScene->upload.vertices.size());'
new='''}if(frozenScene->outputWidth!=outputWidth||frozenScene->outputHeight!=outputHeight){uint32_t width=frozenScene->outputWidth,height=frozenScene->outputHeight;VkPhysicalDeviceProperties targetProperties{};load<PFN_vkGetPhysicalDeviceProperties>(library,"vkGetPhysicalDeviceProperties")(gpu,&targetProperties);if(width>targetProperties.limits.maxViewportDimensions[0]||height>targetProperties.limits.maxViewportDimensions[1])throw std::runtime_error("scene viewport exceeds device limits");auto target=std::make_unique<VulkanOffscreenTargetCandidate>(library,dev,gpu,pass,width,height);VkBuffer nextReadback=VK_NULL_HANDLE;VkDeviceMemory nextReadbackMemory=VK_NULL_HANDLE;if(captureFrames)nextReadback=buffer(size_t(width)*height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,nullptr,nextReadbackMemory);if(readback){destroyBuffer(dev,readback,nullptr);buffers.erase(std::remove(buffers.begin(),buffers.end(),readback),buffers.end());freeMemory(dev,readbackMemory,nullptr);memories.erase(std::remove(memories.begin(),memories.end(),readbackMemory),memories.end());}readback=nextReadback;readbackMemory=nextReadbackMemory;offscreenTarget=std::move(target);image=offscreenTarget->color;depthImage=offscreenTarget->depth;view=offscreenTarget->colorView;depthView=offscreenTarget->depthView;framebuffer=offscreenTarget->framebuffer;outputWidth=width;outputHeight=height;++renderTargetAllocations;std::cout<<"scene_target_rebuilt="<<outputWidth<<"x"<<outputHeight<<" preceding_submission=completed\\n";}vertices.resize(frozenScene->upload.vertices.size());'''
assert old in s;s=s.replace(old,new,1)
s=s.replace('if(presentImage)presentImage(image);','if(presentImage)presentImage(image,outputWidth,outputHeight);',1)
old='<<" render_targets=1 gpu_wait='
new='<<" render_targets="<<renderTargetAllocations<<" gpu_wait='
assert old in s;s=s.replace(old,new,1);p.write_text(s)
