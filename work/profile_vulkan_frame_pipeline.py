from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include <future>','#include <future>\n#include <chrono>')
s=s.replace('struct PreparedDrawFrameCandidate {','using FrameClock=std::chrono::steady_clock;auto frameMs=[](auto start,auto end){return std::chrono::duration<double,std::milli>(end-start).count();};\n struct PreparedDrawFrameCandidate {double preparationMilliseconds=0;')
s=s.replace('auto prepareFrame=[&](size_t frame){PreparedDrawFrameCandidate result;', 'auto prepareFrame=[&](size_t frame){auto preparationStart=FrameClock::now();PreparedDrawFrameCandidate result;')
s=s.replace('return result;};\n std::future<PreparedDrawFrameCandidate>', 'result.preparationMilliseconds=frameMs(preparationStart,FrameClock::now());return result;};\n std::future<PreparedDrawFrameCandidate>')
s=s.replace('for(size_t frame=0;frame<frameCount;++frame){', 'for(size_t frame=0;frame<frameCount;++frame){auto frameStart=FrameClock::now();double cpuPreparationMs=0;auto acquireStart=frameStart;')
s=s.replace('vertices=std::move(prepared.vertices);', 'cpuPreparationMs=prepared.preparationMilliseconds;vertices=std::move(prepared.vertices);')
s=s.replace('if(frame)check(load<PFN_vkResetCommandPool>', 'auto acquireEnd=FrameClock::now();\n if(frame)check(load<PFN_vkResetCommandPool>')
s=s.replace('check(waitFences(dev,1,&frameFence,VK_TRUE,30000000000ull));', 'auto fenceStart=FrameClock::now();check(waitFences(dev,1,&frameFence,VK_TRUE,30000000000ull));auto fenceEnd=FrameClock::now();')
s=s.replace('std::vector<uint8_t>rgba(512*512*4);', 'auto outputStart=FrameClock::now();std::vector<uint8_t>rgba(512*512*4);')
marker='output=512x512 RGBA frame="<<frame<<"\\n";'
assert marker in s
s=s.replace(marker,marker+'if(sequencePathSize){auto frameEnd=FrameClock::now();std::cout<<"frame_profile frame="<<frame<<" cpu_prepare_ms="<<cpuPreparationMs<<" acquire_upload_ms="<<frameMs(acquireStart,acquireEnd)<<" fence_wait_ms="<<frameMs(fenceStart,fenceEnd)<<" readback_output_ms="<<frameMs(outputStart,frameEnd)<<" frame_wall_ms="<<frameMs(frameStart,frameEnd)<<"\\n";}')
p.write_text(s)
