#define NOMINMAX
#include <windows.h>
#include "win32_vulkan_playback_candidate.h"
#include <iostream>
void check(bool ok){if(!ok)throw std::runtime_error("idle playback assertion");}
void CALLBACK advance(HWND w,UINT,UINT_PTR id,DWORD){KillTimer(w,id);SendMessage(w,WM_KEYDOWN,VK_RIGHT,0);}
void CALLBACK closeWindow(HWND w,UINT,UINT_PTR id,DWORD){KillTimer(w,id);SendMessage(w,WM_CLOSE,0,0);}
int main(){HWND w=nullptr;try{WNDCLASSA cls{};cls.lpfnWndProc=vulkanPlaybackWindowProc;cls.hInstance=GetModuleHandleA(nullptr);cls.lpszClassName="Sfm64IdleClockTest";check(RegisterClassA(&cls)!=0);w=CreateWindowA(cls.lpszClassName,"idle test",0,0,0,100,100,nullptr,nullptr,cls.hInstance,nullptr);check(w!=nullptr);vulkanPlaybackState.window=w;vulkanPlaybackState.count=3;check(SetTimer(w,1,80,advance)!=0);auto start=playbackNowCandidate();check(nextWin32VulkanFrameCandidate(0)==1);auto waited=playbackNowCandidate()-start;check(waited>=40000000&&waited<5000000000ull);SendMessage(w,WM_KEYDOWN,VK_SPACE,0);check(nextWin32VulkanFrameCandidate(1)==2);SendMessage(w,WM_KEYDOWN,VK_SPACE,0);auto paused=vulkanPlaybackState.selected;check(SetTimer(w,2,80,closeWindow)!=0);start=playbackNowCandidate();check(nextWin32VulkanFrameCandidate(paused)==3);check(playbackNowCandidate()-start>=40000000);DestroyWindow(w);std::cout<<"Win32 playback idle test passed: paused wait, input wake, timed playback boundary, pause, close wake\n";return 0;}catch(const std::exception&e){if(w)DestroyWindow(w);std::cerr<<e.what()<<'\n';return 1;}}
