from pathlib import Path
p=Path('work/win32_vulkan_playback_candidate.h');s=p.read_text()
needle='inline LRESULT CALLBACK'
insert='inline std::string playbackWindowTitleCandidate(){return "SFM64 prototype - Frame "+std::to_string(vulkanPlaybackState.selected+1)+" / "+std::to_string(vulkanPlaybackState.count)+" - "+(vulkanPlaybackState.playing?"Playing":"Paused");}\ninline void updatePlaybackWindowTitleCandidate(){if(!vulkanPlaybackState.window||(!vulkanPlaybackState.interactive&&!vulkanPlaybackState.test))return;auto title=playbackWindowTitleCandidate();if(!SetWindowTextA(vulkanPlaybackState.window,title.c_str()))throw std::runtime_error("playback title update failed");if(vulkanPlaybackState.test)std::cout<<"playback_window_title="<<title<<"\\n";}\n'
assert needle in s;s=s.replace(needle,insert+needle,1)
old='if(key==VK_SPACE){auto now='
new='if(key==VK_SPACE){if(value&(LPARAM(1)<<30))return 0;auto now='
assert old in s;s=s.replace(old,new,1)
old='}else if(key==VK_ESCAPE)vulkanPlaybackState.closed=true;'
new='}else if(key==VK_HOME){vulkanPlaybackState.playing=false;vulkanPlaybackState.selected=0;}else if(key==VK_END){vulkanPlaybackState.playing=false;vulkanPlaybackState.selected=vulkanPlaybackState.count-1;}else if(key==VK_ESCAPE)vulkanPlaybackState.closed=true;'
assert old in s;s=s.replace(old,new,1)
old='const UINT actions[]={VK_RIGHT,VK_LEFT,0,VK_SPACE,VK_ESCAPE};auto key=actions[vulkanPlaybackState.testStep++];'
new='const UINT actions[]={VK_RIGHT,VK_LEFT,0,VK_SPACE,VK_ESCAPE};const UINT boundaries[]={VK_END,VK_HOME,VK_RIGHT,VK_SPACE,VK_ESCAPE};bool boundaryTest=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST",nullptr,0)!=0;auto key=(boundaryTest?boundaries:actions)[vulkanPlaybackState.testStep++];'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text();old='updatePresentedSceneRateCandidate(rate);++presented;';new='updatePresentedSceneRateCandidate(rate);updatePlaybackWindowTitleCandidate();++presented;';assert old in s;s=s.replace(old,new,1);p.write_text(s)
