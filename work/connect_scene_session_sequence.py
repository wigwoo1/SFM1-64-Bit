from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
needle=' if(frameProvider.sample&&frameProvider.count!=frameCount)'
insert=''' std::vector<std::string>sceneSessionFiles;char sessionSequencePath[32768]{};auto sessionSequenceSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE",sessionSequencePath,sizeof(sessionSequencePath));if(sessionSequenceSize){if(sessionSequenceSize>=sizeof(sessionSequencePath)||sceneTicks.empty())throw std::runtime_error("scene session sequence conflict");std::ifstream input(sessionSequencePath);std::string path;while(input>>path)sceneSessionFiles.push_back(path);if(sceneSessionFiles.size()!=sceneTicks.size())throw std::runtime_error("scene session sequence count");char initialPath[32768]{};auto size=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION",initialPath,sizeof(initialPath));if(!size||size>=sizeof(initialPath)||std::filesystem::path(initialPath).lexically_normal()!=std::filesystem::path(sceneSessionFiles.front()).lexically_normal())throw std::runtime_error("scene initial session mismatch");}
'''
assert needle in s;s=s.replace(needle,insert+needle,1)
needle='auto prepareScene=[&](size_t frame){auto start=FrameClock::now();auto scene='
new='auto prepareScene=[&](size_t frame){auto start=FrameClock::now();if(!sceneSessionFiles.empty()&&frame&&sceneSessionFiles.at(frame)!=sceneSessionFiles.at(frame-1))frozenSceneOwner->replaceSession(loadGraph(sceneSessionFiles.at(frame)),2237);auto scene='
assert needle in s;s=s.replace(needle,new,1)
p.write_text(s)
