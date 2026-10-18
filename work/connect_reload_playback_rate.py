from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
s=s.replace('frozenSceneOwner->sample(sceneTicks.at(frame),initialOutputWidth,initialOutputHeight)','selectNextFrame?frozenSceneOwner->samplePlayback(sceneTicks.at(frame),initialOutputWidth,initialOutputHeight):frozenSceneOwner->sample(sceneTicks.at(frame),initialOutputWidth,initialOutputHeight)',1)
s=s.replace('presentImage(image,outputWidth,outputHeight);','presentImage(image,outputWidth,outputHeight,frozenScene?frozenScene->packedFrameRate:0);',1)
s=s.replace('std::function<void(VkImage,uint32_t,uint32_t)>','std::function<void(VkImage,uint32_t,uint32_t,uint32_t)>');p.write_text(s)
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text();s=s.replace('std::function<void(VkImage,uint32_t,uint32_t)>','std::function<void(VkImage,uint32_t,uint32_t,uint32_t)>');p.write_text(s)
p=Path('work/win32_vulkan_playback_candidate.h');s=p.read_text();s=s.replace('bool interactive=false,test=false,playing=false,closed=false;','uint32_t activeRate=0;bool rateOverride=false;bool interactive=false,test=false,playing=false,closed=false;',1)
needle='inline LRESULT CALLBACK'
insert='inline void updatePresentedSceneRateCandidate(uint32_t rate){if(!rate||vulkanPlaybackState.rateOverride||(!vulkanPlaybackState.interactive&&!vulkanPlaybackState.test)||rate==vulkanPlaybackState.activeRate)return;vulkanPlaybackState.clock.setPackedRate(rate);vulkanPlaybackState.clock.reset(playbackNowCandidate(),vulkanPlaybackState.selected);vulkanPlaybackState.activeRate=rate;std::cout<<"playback_scene_rate_applied="<<rate<<" anchor_frame="<<vulkanPlaybackState.selected<<"\\n";}\n'
assert needle in s;s=s.replace(needle,insert+needle,1);p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text()
old='vulkanPlaybackState.clock.reset(playbackNowCandidate(),0);'
new='vulkanPlaybackState.rateOverride=bool(rateSize);vulkanPlaybackState.activeRate=resolvedSessionRate;vulkanPlaybackState.clock.reset(playbackNowCandidate(),0);'
assert old in s;s=s.replace(old,new,1)
old='[&](VkImage image,uint32_t width,uint32_t height){presenter.presentImage(image,width,height);++presented;}'
new='[&](VkImage image,uint32_t width,uint32_t height,uint32_t rate){presenter.presentImage(image,width,height);updatePresentedSceneRateCandidate(rate);++presented;}'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
