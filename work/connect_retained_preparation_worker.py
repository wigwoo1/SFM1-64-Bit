from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
s='#include "retained_preparation_worker_candidate.h"\n'+s
needle=' bool prefetchFrames=true;'
assert needle in s
# Instantiate only after the option has been parsed, just before GPU cleanup.
needle=' auto cleanup=[&]()'
assert needle in s
s=s.replace(needle,' std::unique_ptr<RetainedPreparationWorkerCandidate>preparationWorker;if(hasFrameInputs&&prefetchFrames&&!selectNextFrame)preparationWorker=std::make_unique<RetainedPreparationWorkerCandidate>();\n'+needle,1)
old='nextPreparedFrame=std::async(std::launch::async,prepareFrame,frame+1)'
assert old in s;s=s.replace(old,'nextPreparedFrame=preparationWorker->submit([&,next=frame+1]{return prepareFrame(next);})',1)
old='nextPreparedScene=std::async(std::launch::async,prepareScene,frame+1)'
assert old in s;s=s.replace(old,'nextPreparedScene=preparationWorker->submit([&,next=frame+1]{return prepareScene(next);})',1)
needle='if(nextPreparedFrame.valid())nextPreparedFrame.wait();if(pending)'
assert needle in s;s=s.replace(needle,'if(nextPreparedFrame.valid())nextPreparedFrame.wait();preparationWorker.reset();if(pending)',1)
needle='std::cout<<"retained_gpu_material_cache_entries="'
assert needle in s;s=s.replace(needle,'std::cout<<"preparation_worker_threads="<<(preparationWorker?1:0)<<" lifetime=retained queue_capacity=1\\n";'+needle,1)
p.write_text(s)
