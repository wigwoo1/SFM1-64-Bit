from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='std::future<PreparedDrawFrameCandidate>nextPreparedFrame;size_t preparationTasks=0;'
new='''std::future<PreparedDrawFrameCandidate>nextPreparedFrame;std::future<std::pair<PreparedSessionSceneCandidate,double>>nextPreparedScene;auto prepareScene=[&](size_t frame){auto start=FrameClock::now();auto scene=frozenSceneOwner->sample(sceneTicks.at(frame));double elapsed=frameMs(start,FrameClock::now());return std::make_pair(std::move(scene),elapsed);};size_t preparationTasks=0;'''
assert old in s;s=s.replace(old,new,1)
old='auto cleanup=[&](){if(nextPreparedFrame.valid())nextPreparedFrame.wait();'
new='auto cleanup=[&](){if(nextPreparedScene.valid())nextPreparedScene.wait();if(nextPreparedFrame.valid())nextPreparedFrame.wait();'
assert old in s;s=s.replace(old,new,1)
old='if(!sceneTicks.empty()){auto prepareStart=FrameClock::now();if(frame){auto next=frozenSceneOwner->sample(sceneTicks.at(frame));'
new='if(!sceneTicks.empty()){double scenePreparationMs=0;auto prepareStart=FrameClock::now();if(frame){auto prepared=nextPreparedScene.valid()?nextPreparedScene.get():prepareScene(frame);scenePreparationMs=prepared.second;auto next=std::move(prepared.first);prepareStart=FrameClock::now();'
assert old in s;s=s.replace(old,new,1)
old='cpuPreparationMs=frameMs(prepareStart,FrameClock::now());std::cout<<"multi_model_scene_frame='
new='cpuPreparationMs=scenePreparationMs+frameMs(prepareStart,FrameClock::now());std::cout<<"multi_model_scene_frame='
assert old in s;s=s.replace(old,new,1)
old='pending=true;if(hasFrameInputs&&sceneTicks.empty()&&prefetchFrames&&!selectNextFrame&&frame+1<frameCount){nextPreparedFrame=std::async(std::launch::async,prepareFrame,frame+1);++preparationTasks;}'
new='pending=true;if(hasFrameInputs&&prefetchFrames&&!selectNextFrame&&frame+1<frameCount){if(sceneTicks.empty())nextPreparedFrame=std::async(std::launch::async,prepareFrame,frame+1);else nextPreparedScene=std::async(std::launch::async,prepareScene,frame+1);++preparationTasks;}'
assert old in s;s=s.replace(old,new,1)
p.write_text(s)
