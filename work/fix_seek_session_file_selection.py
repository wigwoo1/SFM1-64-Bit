from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='std::future<std::pair<PreparedSessionSceneCandidate,double>>nextPreparedScene;auto prepareScene='
new='std::future<std::pair<PreparedSessionSceneCandidate,double>>nextPreparedScene;std::string preparedSessionFile=sceneSessionFiles.empty()?std::string{}:sceneSessionFiles.front();auto prepareScene='
assert old in s;s=s.replace(old,new,1)
old='!sceneSessionFiles.empty()&&frame&&sceneSessionFiles.at(frame)!=sceneSessionFiles.at(frame-1)'
new='!sceneSessionFiles.empty()&&sceneSessionFiles.at(frame)!=preparedSessionFile'
assert old in s;s=s.replace(old,new,1)
old='frozenSceneOwner->replaceSession(std::move(graph),root);}auto scene=movieOutput?'
new='frozenSceneOwner->replaceSession(std::move(graph),root);preparedSessionFile=sceneSessionFiles.at(frame);}auto scene=movieOutput?'
assert old in s;s=s.replace(old,new,1)
# Frame zero is initially cached, but a later backward seek must resample it.
old='if(frame){auto prepared=nextPreparedScene.valid()?'
new='if(frame||iteration){auto prepared=nextPreparedScene.valid()?'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
