from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text(encoding='utf-8')
old='std::unique_ptr<RetainedSessionSceneCandidate>frozenSceneOwner;'
new='char blankPolicy[8]{};auto blankSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_TIMELINE_BLANKS",blankPolicy,sizeof(blankPolicy));if(blankSize&&(blankSize!=1||(blankPolicy[0]!=\'0\'&&blankPolicy[0]!=\'1\')))throw std::runtime_error("timeline blank policy invalid");const bool timelineBlanks=blankSize&&blankPolicy[0]==\'1\';if(timelineBlanks&&!sceneRequested)throw std::runtime_error("timeline blank policy requires scene");'+old
assert old in s;s=s.replace(old,new,1)
old='frozenSceneOwner->sample(tick,outputWidth,outputHeight)';assert old in s;s=s.replace(old,'timelineBlanks?frozenSceneOwner->sampleTimelineOutput(tick,outputWidth,outputHeight):'+old,1)
old='auto scene=movieOutput?';assert old in s;s=s.replace(old,'auto scene=timelineBlanks?frozenSceneOwner->sampleTimelineOutput(sceneTicks.at(frame),initialOutputWidth,initialOutputHeight,movieOutput):movieOutput?',1)
p.write_text(s,encoding='utf-8')
