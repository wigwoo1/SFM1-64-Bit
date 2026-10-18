#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include "studio_presentation_windowed_candidate.h"
struct Services{
 unsigned depth,calls=0,expectedWindowed=1;
 bool force=false;int triple=0;unsigned cached=0,commands=0,initializations=0;
 D3DPRESENT_PARAMETERS* observed=nullptr;
 void checkPending(){if(!observed||observed->BackBufferCount!=0||observed->PresentationInterval!=0||observed->BackBufferFormat!=D3DFMT_X8R8G8B8)throw 9;}
 bool forceNoVsync(){checkPending();++commands;return force;}
 bool tripleBuffered(){checkPending();if(!(cached&1u)){cached|=1u;++initializations;}return triple!=0;}
 unsigned sampleType(int32_t){++calls;return 0;}unsigned adapterVendor(unsigned){++calls;return 0;}
 HRESULT sampleSupport(unsigned,unsigned,D3DFORMAT,BOOL,D3DMULTISAMPLE_TYPE,unsigned&){++calls;return E_FAIL;}
 unsigned colorChoice(unsigned adapter,unsigned type,unsigned prior,unsigned requested,unsigned windowed){if(adapter!=3||type!=1||prior!=22||requested!=22||windowed!=expectedWindowed)throw 1;++calls;return 22;}
 unsigned depthFormat(unsigned adapter,unsigned prior,unsigned color,unsigned preferred){if(adapter!=3||prior!=22||color!=22||(preferred!=75&&preferred!=77))throw 2;++calls;return depth;}
 unsigned translateFormat(unsigned color){if(color!=22)throw 3;++calls;return 22;}
};
int main(){try{std::ifstream oracle("outputs/native-presentation-windowed.bin",std::ios::binary);unsigned cases=0;
 for(unsigned flags:{1u,3u,11u,5u,65u,129u})for(unsigned depth:{73u,75u,79u,22u}){
 StudioPresentationDescriptorCandidate descriptor{800,600,22,0,static_cast<unsigned char>(flags),640,480,0};D3DPRESENT_PARAMETERS p{};unsigned char ownerFlags=0x55;unsigned depthBits=0x55555555;Services services{depth};
 studioPresentationWindowedCandidate(p,ownerFlags,depthBits,reinterpret_cast<HWND>(0x1234),3,22,descriptor,1280,720,services);
 std::array<unsigned,16> expected{},record{p.BackBufferWidth,p.BackBufferHeight,static_cast<unsigned>(p.BackBufferFormat),p.BackBufferCount,static_cast<unsigned>(p.MultiSampleType),p.MultiSampleQuality,static_cast<unsigned>(p.SwapEffect),static_cast<unsigned>(reinterpret_cast<uintptr_t>(p.hDeviceWindow)),static_cast<unsigned>(p.Windowed),static_cast<unsigned>(p.EnableAutoDepthStencil),static_cast<unsigned>(p.AutoDepthStencilFormat),p.Flags,p.FullScreen_RefreshRateInHz,p.PresentationInterval,0x55555500u|ownerFlags,depthBits};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!oracle||record!=expected||services.calls!=3){std::printf("windowed_mismatch=%u\n",cases);return 1;}
 Services wideServices{depth};studioPresentationWindowedCandidate(p,ownerFlags,depthBits,reinterpret_cast<HWND>(uintptr_t{0x100001234ull}),3,22,descriptor,1280,720,wideServices);if(reinterpret_cast<uintptr_t>(p.hDeviceWindow)!=0x100001234ull)return 2;++cases;
 }if(oracle.peek()!=std::char_traits<char>::eof())return 3;
 std::ifstream fullscreenOracle("outputs/native-presentation-fullscreen.bin",std::ios::binary);unsigned fullscreenCases=0;
 for(unsigned flags:{0u,4u})for(int requestedWidth:{0,800,-1})for(int denominator:{0,1001}){
 StudioPresentationDescriptorCandidate descriptor{requestedWidth,600,22,0,static_cast<unsigned char>(flags),0,0,0,60000,denominator};D3DPRESENT_PARAMETERS p{};unsigned char ownerFlags=0;unsigned depthBits=0;Services services{75,0,0};studioPresentationWindowedCandidate(p,ownerFlags,depthBits,reinterpret_cast<HWND>(0x1234),3,22,descriptor,1280,720,services);
 std::array<unsigned,16> expected{},record{p.BackBufferWidth,p.BackBufferHeight,static_cast<unsigned>(p.BackBufferFormat),p.BackBufferCount,static_cast<unsigned>(p.MultiSampleType),p.MultiSampleQuality,static_cast<unsigned>(p.SwapEffect),static_cast<unsigned>(reinterpret_cast<uintptr_t>(p.hDeviceWindow)),static_cast<unsigned>(p.Windowed),static_cast<unsigned>(p.EnableAutoDepthStencil),static_cast<unsigned>(p.AutoDepthStencilFormat),p.Flags,p.FullScreen_RefreshRateInHz,p.PresentationInterval,ownerFlags,depthBits};fullscreenOracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!fullscreenOracle||record!=expected||services.calls!=3)return 5;++fullscreenCases;
 }if(fullscreenOracle.peek()!=std::char_traits<char>::eof())return 6;
 std::ifstream vsyncOracle("outputs/native-presentation-vsync.bin",std::ios::binary);unsigned vsyncCases=0;
 for(unsigned flags:{16u,20u})for(bool force:{false,true})for(int triple:{-1,0,1})for(unsigned cached:{0u,1u}){
 StudioPresentationDescriptorCandidate descriptor{800,600,22,0,static_cast<unsigned char>(flags),0,0,0,60000,1001};D3DPRESENT_PARAMETERS p{};unsigned char ownerFlags=0;unsigned depthBits=0;Services services{75,0,0};services.force=force;services.triple=triple;services.cached=cached;services.observed=&p;
 studioPresentationWindowedCandidate(p,ownerFlags,depthBits,reinterpret_cast<HWND>(0x1234),3,22,descriptor,1280,720,services);
 std::array<unsigned,19> expected{},record{p.BackBufferWidth,p.BackBufferHeight,static_cast<unsigned>(p.BackBufferFormat),p.BackBufferCount,static_cast<unsigned>(p.MultiSampleType),p.MultiSampleQuality,static_cast<unsigned>(p.SwapEffect),static_cast<unsigned>(reinterpret_cast<uintptr_t>(p.hDeviceWindow)),static_cast<unsigned>(p.Windowed),static_cast<unsigned>(p.EnableAutoDepthStencil),static_cast<unsigned>(p.AutoDepthStencilFormat),p.Flags,p.FullScreen_RefreshRateInHz,p.PresentationInterval,ownerFlags,depthBits,services.commands,services.initializations,services.cached};vsyncOracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!vsyncOracle||record!=expected)return 7;++vsyncCases;
 }if(vsyncOracle.peek()!=std::char_traits<char>::eof())return 8;std::printf("x64_fullscreen_vsync_matches=%u\n",vsyncCases);
 std::printf("x64_windowed_presentation_native_matches=%u fullscreen_native_matches=%u fullwidth_hwnd=passed gpu_executed=0\n",cases,fullscreenCases);
 }catch(...){return 4;}}



