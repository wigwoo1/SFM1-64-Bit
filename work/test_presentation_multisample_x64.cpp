#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "native_presentation_multisample_candidate.h"
#include "studio_multisample_selector_candidate.h"
#include "studio_presentation_windowed_candidate.h"
#include "studio_presentation_builder_candidate.h"
struct Services{
 unsigned vendor,result,quality,calls=0,seen=0,step=0;
 unsigned displayCalls=0,dimensionCalls=0,materialReads=0,conditionCalls=0;
 void displayMode(std::array<uint32_t,6>& display,unsigned adapter){if(adapter!=3||display[0]!=1)throw 10;++displayCalls;display={1,1280,720,0,0,0};}
 void dimensions(std::array<uint32_t,6>& display,StudioPresentationDescriptorCandidate&,int32_t& width,int32_t& height){if(display[1]!=1280||display[2]!=720)throw 11;++dimensionCalls;width=1280;height=720;}
 float aspectThreshold(){return 1.5f;}
 Services& material(){++materialReads;return *this;}
 void setCondition(const char* name,int value){const char* names[]={"WIN32WIDE","WIN32HIDEF","WIN32LODEF"};const unsigned index=conditionCalls%3;if(std::strcmp(name,names[index])||value!=(index==2?0:1))throw 12;++conditionCalls;}
 bool forceNoVsync(){++calls;return false;}bool tripleBuffered(){++calls;return false;}
 unsigned colorChoice(unsigned adapter,unsigned type,unsigned prior,unsigned requested,unsigned windowed){if(adapter!=3||type!=1||prior!=22||requested!=22||windowed!=1)throw 4;return 22;}
 unsigned depthFormat(unsigned adapter,unsigned prior,unsigned color,unsigned preferred){if(adapter!=3||prior!=22||color!=22||preferred!=77)throw 5;return 75;}
 unsigned translateFormat(unsigned color){if(color!=22)throw 6;return 22;}
 unsigned sampleType(int32_t request){if(++step!=1)throw 1;return studioMultisampleSelectorCandidate(request);}
 unsigned adapterVendor(unsigned adapter){if(++step!=2||adapter!=3)throw 2;return vendor;}
 HRESULT CheckDeviceMultiSampleType(UINT adapter,D3DDEVTYPE type,D3DFORMAT format,BOOL windowed,D3DMULTISAMPLE_TYPE sample,DWORD* output){if(++step!=3||adapter!=3||type!=D3DDEVTYPE_HAL||format!=D3DFMT_X8R8G8B8||windowed!=1||!output)throw 3;++calls;seen=sample;*output=quality;return static_cast<HRESULT>(result);}
};
template struct NativePresentationMultisampleServicesCandidateT<IDirect3D9,Services>;
int main(){try{std::ifstream oracle("outputs/native-presentation-multisample.bin",std::ios::binary),fullOracle("outputs/native-presentation-combined.bin",std::ios::binary);unsigned cases=0;
 for(int32_t request:{1,2,4,16,17})for(unsigned vendor:{0x10deu,0x8086u})for(unsigned result:{0u,1u,0x80000000u,0xffffffffu})for(unsigned quality:{0u,5u,16u}){
 D3DPRESENT_PARAMETERS p{};p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.Windowed=1;p.BackBufferFormat=D3DFMT_X8R8G8B8;Services services{vendor,result,quality};Services* api=&services;NativePresentationMultisampleServicesCandidateT<Services,Services> bridge{api,services};studioPresentationMultisampleCandidate(p,request,3,bridge);
 std::array<unsigned,4> record{static_cast<unsigned>(p.MultiSampleType),p.MultiSampleQuality,services.calls,services.seen},expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!oracle||record!=expected)return 1;++cases;
 Services combinedServices{vendor,result,quality};Services* combinedApi=&combinedServices;NativePresentationMultisampleServicesCandidateT<Services,Services> combinedBridge{combinedApi,combinedServices};StudioPresentationDescriptorCandidate descriptor{800,600,22,0,1,0,0,request};unsigned char flags=0;unsigned depthBits=0;const auto before=p;studioPresentationBuilderCandidate(p,flags,depthBits,reinterpret_cast<HWND>(0x1234),3,22,descriptor,true,combinedBridge);if(std::memcmp(&p,&before,sizeof(p))||flags!=0||depthBits!=0||combinedServices.calls!=0)return 8;studioPresentationBuilderCandidate(p,flags,depthBits,reinterpret_cast<HWND>(0x1234),3,22,descriptor,false,combinedBridge);if(combinedServices.displayCalls!=2||combinedServices.dimensionCalls!=2||combinedServices.materialReads!=6||combinedServices.conditionCalls!=6)return 9;
 std::array<unsigned,16> fullRecord{p.BackBufferWidth,p.BackBufferHeight,static_cast<unsigned>(p.BackBufferFormat),p.BackBufferCount,static_cast<unsigned>(p.MultiSampleType),p.MultiSampleQuality,static_cast<unsigned>(p.SwapEffect),static_cast<unsigned>(reinterpret_cast<uintptr_t>(p.hDeviceWindow)),static_cast<unsigned>(p.Windowed),static_cast<unsigned>(p.EnableAutoDepthStencil),static_cast<unsigned>(p.AutoDepthStencilFormat),p.Flags,p.FullScreen_RefreshRateInHz,p.PresentationInterval,flags,depthBits},fullExpected{};fullOracle.read(reinterpret_cast<char*>(fullExpected.data()),sizeof(fullExpected));if(!fullOracle||fullRecord!=fullExpected)return 7;
 }if(oracle.peek()!=std::char_traits<char>::eof()||fullOracle.peek()!=std::char_traits<char>::eof())return 2;std::printf("x64_presentation_multisample_native_matches=%u combined_parameter_records=passed gpu_executed=0\n",cases);
 }catch(...){return 3;}}




