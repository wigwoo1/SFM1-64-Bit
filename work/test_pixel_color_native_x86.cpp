#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <cstdio>
#include "pixel_color_native_init.h"
#include "pixel_queue_refill_native_init.h"
#include "studio_pixel_color_candidate.h"
#include "pixel_color_native_math_image.h"
#include <cmath>
static unsigned selected=0,service=0,uploads=0;
static float exposureValue=2;
static unsigned powerEnabled=0;
static uint32_t cookie=0x12345678;
static std::array<uint32_t,4097> replacementBlock{};
static uint32_t* expectedPublished=nullptr;
static unsigned publishes=0,acquires=0,ringObject=0;
static void __fastcall publishBlock(void* ring,void*,uint32_t* block){
 if(ring!=&ringObject||block!=expectedPublished||block[0]!=2||block[3]!=0)ExitProcess(23);
 ++publishes;
}
static uint32_t* __fastcall acquireBlock(void*,void*,unsigned state){
 if(state!=1||publishes!=1)ExitProcess(24);
 ++acquires;replacementBlock[0]=state;return replacementBlock.data();
}
static std::array<float,4> submitted{};
static float __fastcall exposureCallback(void*,void*,unsigned){return exposureValue;}
static unsigned __fastcall modeCallback(void*,void*){return selected;}
static unsigned __fastcall serviceCallback(void*,void*){return service;}
static unsigned __fastcall noPower(void*,void*){return powerEnabled;}
static float __fastcall transformCallback(void*,void*,float x){return x+1;}
static void __fastcall cookieCheck(uint32_t value){if(value!=cookie)ExitProcess(20);}
static void forbidden(){ExitProcess(21);}
static HRESULT __stdcall upload(void*,unsigned index,const float* data,unsigned count){
 if(index!=29||count!=1)ExitProcess(22);
 std::memcpy(submitted.data(),data,16);++uploads;return S_OK;
}
struct Ops{
 bool captured,suppress;float factor;
 StudioPixelExposureCandidate exposure(){return {exposureValue,exposureValue>0};}
 unsigned mode(){return selected;}float channel(unsigned i){return float(i+1);}
 bool transformCaptured(){return captured;}bool usePower(){return powerEnabled!=0;}
 float transform(float x){return x+1;}double power(double x,double y){return std::pow(x,y);}
 bool suppressScale(){return suppress;}unsigned serviceMode(){return service;}float scale(){return factor;}
};
int main(){
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 unsigned char original[712];dll.seekg(nativePixelColorFileOffset);dll.read(reinterpret_cast<char*>(original),712);
 if(!dll||std::memcmp(original,nativePixelColorBytes,712))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 2;
 std::memcpy(code,original,712);
 unsigned char refill[76];dll.seekg(nativePixelQueueRefillFileOffset);dll.read(reinterpret_cast<char*>(refill),76);
 if(!dll||std::memcmp(refill,nativePixelQueueRefillBytes,76))return 13;
 std::memcpy(code+1024,refill,76);
 uintptr_t ringAddress=reinterpret_cast<uintptr_t>(&ringObject);std::memcpy(code+1024+40,&ringAddress,4);
 auto refillCall=[&](unsigned offset,uintptr_t target){int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+1024+offset+5));std::memcpy(code+1024+offset+1,&delta,4);};
 refillCall(44,reinterpret_cast<uintptr_t>(&publishBlock));refillCall(53,reinterpret_cast<uintptr_t>(&acquireBlock));
 auto* math=pixelColorNativeMathImage();if(!math)return 8;
 std::array<uintptr_t,204> secondaryTable{};secondaryTable[2]=reinterpret_cast<uintptr_t>(&modeCallback);
 secondaryTable[28]=reinterpret_cast<uintptr_t>(&exposureCallback);secondaryTable[203]=reinterpret_cast<uintptr_t>(&transformCallback);
 std::array<uintptr_t,42> globalTable{};globalTable[34]=reinterpret_cast<uintptr_t>(&serviceCallback);globalTable[41]=reinterpret_cast<uintptr_t>(&noPower);
 uintptr_t globalObject=reinterpret_cast<uintptr_t>(globalTable.data()),globalPointer=reinterpret_cast<uintptr_t>(&globalObject);
 std::array<uintptr_t,110> deviceTable{};deviceTable[109]=reinterpret_cast<uintptr_t>(&upload);
 uintptr_t deviceObject=reinterpret_cast<uintptr_t>(deviceTable.data());
 float one=1;double exponent=2.200000047683716;
 for(auto& f:nativePixelColorFixups){const void* target=f[1]==0x1017c370?static_cast<void*>(&cookie):f[1]==0x1016d130?static_cast<void*>(&globalPointer):f[1]==0x100fa5f8?static_cast<void*>(&one):static_cast<void*>(&exponent);
  uintptr_t p=reinterpret_cast<uintptr_t>(target);std::memcpy(code+f[0],&p,4);}
 for(auto& c:nativePixelColorCalls){uintptr_t target=c[1]==0x10002da0?reinterpret_cast<uintptr_t>(code+1024):c[1]==0x100e01aa?reinterpret_cast<uintptr_t>(math+0x1aa):c[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):c[1]==0x100df030?reinterpret_cast<uintptr_t>(&std::memcpy):reinterpret_cast<uintptr_t>(&forbidden);
  int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+c[0]+5));std::memcpy(code+c[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);
 std::array<unsigned char,0x3200> owner{};std::array<float,120> current{},comparison{};
 auto word=[&](unsigned offset,uintptr_t value){std::memcpy(owner.data()+offset,&value,4);};
 word(0x2d0,reinterpret_cast<uintptr_t>(secondaryTable.data()));word(0x648,reinterpret_cast<uintptr_t>(&deviceObject));
 word(0x19ec,reinterpret_cast<uintptr_t>(current.data()));word(0x2864,reinterpret_cast<uintptr_t>(comparison.data()));
 auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);unsigned cases=0;
 unsigned queueCases=0,rolloverCases=0;std::array<uint32_t,16> packet{};
 std::ofstream oracle("outputs/native-pixel-color-grid.bin",std::ios::binary|std::ios::trunc);if(!oracle)return 6;
 for(selected=0;selected<3;++selected)for(service=0;service<2;++service)
 for(unsigned flags=0;flags<4;++flags)for(float e:{0.f,2.f,-2.f})for(powerEnabled=0;powerEnabled<2;++powerEnabled){
  exposureValue=e;owner[0x2004]=static_cast<unsigned char>(flags&1);owner[0x1890]=static_cast<unsigned char>(flags&2);
  for(unsigned i=0;i<3;++i){float value=float(i+1);std::memcpy(owner.data()+0x1880+i*4,&value,4);}
  float factor=2;std::memcpy(owner.data()+0x3114,&factor,4);comparison.fill(-99);current.fill(-98);uploads=0;
  Ops ops{bool(flags&1),bool(flags&2),factor};const auto expected=studioPixelColorCandidate(ops);
  native(owner.data());
  if(uploads!=1||std::memcmp(submitted.data(),expected.data(),16)||std::memcmp(current.data()+116,expected.data(),16)||std::memcmp(comparison.data()+116,expected.data(),16))return 4;
  oracle.write(reinterpret_cast<const char*>(submitted.data()),16);if(!oracle)return 7;
  native(owner.data());if(uploads!=1)return 5;++cases;
  // Seven available words exercises the native exact-capacity queue branch.
  // Consumer ownership/transport is not simulated.
  packet.fill(0xcccccccc);word(0x658,1);word(0x660,reinterpret_cast<uintptr_t>(packet.data()));word(0x664,7);
  comparison.fill(-99);current.fill(-98);uploads=0;
  native(owner.data());
  if(uploads||packet[0]!=7||packet[1]!=29||packet[2]!=1||std::memcmp(packet.data()+3,expected.data(),16))return 9;
  uint32_t remaining=99;uintptr_t cursor=0;std::memcpy(&remaining,owner.data()+0x664,4);std::memcpy(&cursor,owner.data()+0x660,4);
  if(remaining||cursor!=reinterpret_cast<uintptr_t>(packet.data()+7)||packet[7]!=0xcccccccc)return 10;
  if(std::memcmp(current.data()+116,expected.data(),16)||std::memcmp(comparison.data()+116,expected.data(),16))return 11;
  native(owner.data());uintptr_t repeated=0;std::memcpy(&repeated,owner.data()+0x660,4);if(repeated!=cursor||uploads)return 12;
  ++queueCases;
  packet.fill(0xcccccccc);packet[0]=1;expectedPublished=packet.data();publishes=acquires=0;replacementBlock.fill(0xcccccccc);
  word(0x65c,reinterpret_cast<uintptr_t>(packet.data()));word(0x660,reinterpret_cast<uintptr_t>(packet.data()+3));word(0x664,6);
  comparison.fill(-99);current.fill(-98);native(owner.data());
  std::memcpy(&remaining,owner.data()+0x664,4);std::memcpy(&cursor,owner.data()+0x660,4);
  if(publishes!=1||acquires!=1||uploads||remaining!=4088||cursor!=reinterpret_cast<uintptr_t>(replacementBlock.data()+8))return 14;
  if(replacementBlock[0]!=1||replacementBlock[1]!=7||replacementBlock[2]!=29||replacementBlock[3]!=1||std::memcmp(replacementBlock.data()+4,expected.data(),16)||replacementBlock[8]!=0xcccccccc)return 15;
  if(std::memcmp(current.data()+116,expected.data(),16)||std::memcmp(comparison.data()+116,expected.data(),16))return 16;
  ++rolloverCases;word(0x658,0);
 }
 VirtualFree(math,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);std::printf("native_pixel_color_cases=%u native_queue_packet_cases=%u native_refill_rollover_cases=%u original_power_branch=1 repeat_skipped=1\n",cases,queueCases,rolloverCases);
}
