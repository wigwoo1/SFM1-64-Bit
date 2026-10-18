#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include "win32_vulkan_playback_candidate.h"
int main(){try{WNDCLASSA cls{};cls.hInstance=GetModuleHandleA(nullptr);cls.lpfnWndProc=vulkanPlaybackWindowProc;cls.lpszClassName="Sfm64SliderTest";RegisterClassA(&cls);HWND parent=CreateWindowExA(0,cls.lpszClassName,"test",WS_OVERLAPPED,0,0,640,640,nullptr,nullptr,cls.hInstance,nullptr);if(!parent)throw std::runtime_error("test window");createTimelineSliderCandidate(parent);vulkanPlaybackState.count=20400;for(int position:{0,10000,20399}){updateTimelineSliderCandidate(20400,position);vulkanPlaybackState.playing=true;SendMessage(parent,WM_HSCROLL,TB_THUMBTRACK,reinterpret_cast<LPARAM>(vulkanTimelineSlider));if(vulkanPlaybackState.selected!=size_t(position)||vulkanPlaybackState.playing)throw std::runtime_error("slider selection");}if(timelineSliderFrameCandidate(0,1)!=0||timelineSliderFrameCandidate(1000000,10000000000ull)!=9999999999ull)throw std::runtime_error("slider limits");DestroyWindow(parent);vulkanTimelineSlider=nullptr;std::cout<<"timeline slider passed Win32_control_selection=0,10000,20399 pauses_playback single_frame large_range_endpoint\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
