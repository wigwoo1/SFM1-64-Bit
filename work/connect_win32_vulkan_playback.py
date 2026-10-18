from pathlib import Path
p=Path('work/probe_vulkan_win32_surface_x64.cpp');s=p.read_text().replace('using Handle=void*;','#include "win32_vulkan_playback_candidate.h"\nusing Handle=void*;')
s=s.replace('int main(){static_assert', 'int main(int argc,char**argv){if(argc>2){std::cerr<<"optional --interactive or --control-test required\\n";return 1;}if(argc==2){playback.interactive=std::string(argv[1])=="--interactive";playback.test=std::string(argv[1])=="--control-test";if(!playback.interactive&&!playback.test){std::cerr<<"unknown window mode\\n";return 1;}}static_assert')
s=s.replace('cls.lpfnWndProc=DefWindowProcA;', 'cls.lpfnWndProc=vulkanPlaybackWindowProc;')
s=s.replace('"SFM64 Vulkan surface probe",WS_OVERLAPPEDWINDOW', '"SFM64 Vulkan playback | Space: play/pause | Left/Right: step",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX')
s=s.replace('if(!window)throw std::runtime_error("window creation");', 'if(!window)throw std::runtime_error("window creation");playback.window=window;if(playback.interactive){ShowWindow(window,SW_SHOW);UpdateWindow(window);}')
s=s.replace('if(selectedDevice>=0&&n!=size_t(selectedDevice))continue;', 'if(selectedDevice>=0&&n!=size_t(selectedDevice))continue;if((playback.interactive||playback.test)&&usable)break;')
s=s.replace('window_visible=0 swapchain=created', 'window_visible="<<playback.interactive<<" swapchain=created')
p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text()
marker='testVulkanGraphicsPipelineCandidate(library,'
assert marker in s
control='''if(playback.interactive||playback.test){if(selectNextFrame)throw std::runtime_error("conflicting frame control modes");char countText[32]{};auto countSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FRAME_COUNT",countText,sizeof(countText));size_t count=0;if(!countSize||countSize>=sizeof(countText))throw std::runtime_error("playback frame count missing");auto parsed=std::from_chars(countText,countText+countSize,count);if(parsed.ec!=std::errc{}||parsed.ptr!=countText+countSize||!count||count>10000)throw std::runtime_error("playback frame count");playback.count=count;playback.selected=0;selectNextFrame=nextWin32VulkanFrameCandidate;}'''
s=s.replace(marker,control+marker,1)
p.write_text(s)
