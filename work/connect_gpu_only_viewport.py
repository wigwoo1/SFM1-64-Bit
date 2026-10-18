from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
needle=' bool asyncOutput=true;'
config=''' bool captureFrames=true;char captureText[8]{};auto captureSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_CAPTURE_FRAMES",captureText,sizeof(captureText));if(captureSize){if(captureSize!=1||(captureText[0]!='0'&&captureText[0]!='1'))throw std::runtime_error("invalid frame capture mode");captureFrames=captureText[0]=='1';}if(!captureFrames&&(!presentImage||blendMode))throw std::runtime_error("GPU-only mode requires presentation and no pixel diagnostic");size_t readbackCopies=0;
'''
assert s.count(needle)==1;s=s.replace(needle,config+needle)
s=s.replace('if(hasFrameInputs&&asyncOutput)frameWriter=', 'if(captureFrames&&hasFrameInputs&&asyncOutput)frameWriter=')
s=s.replace('if(!std::filesystem::is_directory(outputDirectory))', 'if(captureFrames&&!std::filesystem::is_directory(outputDirectory))')
s=s.replace('VkDeviceMemory vertexMemory,indexMemory,readbackMemory;', 'VkDeviceMemory vertexMemory,indexMemory,readbackMemory=VK_NULL_HANDLE;')
s=s.replace('auto readback=buffer(512*512*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,nullptr,readbackMemory);', 'VkBuffer readback=VK_NULL_HANDLE;if(captureFrames)readback=buffer(512*512*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,nullptr,readbackMemory);')
s=s.replace('VkBufferImageCopy region{};', 'if(captureFrames){VkBufferImageCopy region{};')
s=s.replace('0,nullptr,0,nullptr);check(end(command));VkSubmitInfo', '0,nullptr,0,nullptr);++readbackCopies;}check(end(command));VkSubmitInfo')
s=s.replace('auto outputStart=FrameClock::now();std::vector<uint8_t>rgba', 'auto outputStart=FrameClock::now();if(captureFrames){std::vector<uint8_t>rgba')
s=s.replace('<<frame<<"\\n";if(hasFrameInputs)', '<<frame<<"\\n";}else std::cout<<"Vulkan GPU-only model frame="<<frame<<" pixel_verification=disabled\\n";if(hasFrameInputs)')
s=s.replace('<<" async_output_writes="<<asyncWrites', '<<" gpu_readback_copies="<<readbackCopies<<" capture_frames="<<captureFrames<<" async_output_writes="<<asyncWrites')
assert '++readbackCopies;}check(end(command))' in s
assert 'pixel_verification=disabled' in s
p.write_text(s)
run=Path('work/run_session_rate_grid_probe.cmd').read_text().replace('setlocal','setlocal\nset SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0')
Path('work/run_gpu_only_session_probe.cmd').write_text(run)
for name in ['open_native_frame_grid_vulkan.cmd','open_live_tick_grid_vulkan.cmd','open_live_session_vulkan.cmd','open_vulkan_playback.cmd']:
 p=Path('work',name);s=p.read_text();s=s.replace('setlocal','setlocal\nset SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0');p.write_text(s)
