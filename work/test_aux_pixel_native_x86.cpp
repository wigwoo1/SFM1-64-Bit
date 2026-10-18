#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <cstdio>
#include <limits>
#include "aux_pixel_native_init.h"
#include "studio_aux_pixel_constant_candidate.h"
static uint32_t cookie=0x12345678,selected=0;static bool equalBounds=false;
static unsigned uploads=0;static std::array<uint32_t,4> submitted{};
static uint32_t __fastcall modeCallback(void* secondary,void*){
 auto* owner=static_cast<unsigned char*>(secondary)-0x2d0;
 uint32_t index=0;std::memcpy(&index,owner+0x3610,4);if(index!=17)ExitProcess(20);
 float hi=equalBounds?2.f:6.f;std::memcpy(owner+0x2f50,&hi,4);return selected;
}
static void __fastcall cookieCheck(uint32_t value){if(value!=cookie)ExitProcess(21);}
static void forbidden(){ExitProcess(22);}
static HRESULT __stdcall upload(void*,unsigned index,const float* data,unsigned count){
 if(index!=17||count!=1)ExitProcess(23);std::memcpy(submitted.data(),data,16);++uploads;return S_OK;
}
struct Ops{
 float densityValue;void publishIndex(unsigned index){if(index!=17)throw 1;}
 unsigned mode(){return selected;}float lower(){return 2;}float upper(){return equalBounds?2.f:6.f;}
 float second(){return 3;}float density(){return densityValue;}
};
int main(){
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 unsigned char original[518];dll.seekg(nativeAuxPixelFileOffset);dll.read(reinterpret_cast<char*>(original),518);
 if(!dll||std::memcmp(original,nativeAuxPixelBytes,518))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 2;
 std::memcpy(code,original,518);float one=1;uint32_t sentinel=0xff7fffff;
 for(auto& f:nativeAuxPixelFixups){uintptr_t pointer=f[1]==0x1017c370?reinterpret_cast<uintptr_t>(&cookie):f[1]==0x100fa5f8?reinterpret_cast<uintptr_t>(&one):reinterpret_cast<uintptr_t>(&sentinel);std::memcpy(code+f[0],&pointer,4);}
 for(auto& c:nativeAuxPixelCalls){uintptr_t target=c[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):c[1]==0x100df030?reinterpret_cast<uintptr_t>(&std::memcpy):reinterpret_cast<uintptr_t>(&forbidden);
  int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+c[0]+5));std::memcpy(code+c[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);
 std::array<uintptr_t,3> secondaryTable{};secondaryTable[2]=reinterpret_cast<uintptr_t>(&modeCallback);
 std::array<uintptr_t,110> deviceTable{};deviceTable[109]=reinterpret_cast<uintptr_t>(&upload);uintptr_t deviceObject=reinterpret_cast<uintptr_t>(deviceTable.data());
 std::array<unsigned char,0x3700> owner{};std::array<uint32_t,120> current{},comparison{};
 auto word=[&](unsigned offset,uintptr_t value){std::memcpy(owner.data()+offset,&value,4);};
 word(0x2d0,reinterpret_cast<uintptr_t>(secondaryTable.data()));word(0x648,reinterpret_cast<uintptr_t>(&deviceObject));word(0x19ec,reinterpret_cast<uintptr_t>(current.data()));word(0x2864,reinterpret_cast<uintptr_t>(comparison.data()));
 auto native=reinterpret_cast<void(__thiscall*)(void*,uint32_t,uint32_t)>(code);unsigned cases=0;
 std::array<uint32_t,8> packet{};unsigned queuedCases=0;
 std::ofstream oracle("outputs/native-aux-pixel-grid.bin",std::ios::binary|std::ios::trunc);if(!oracle)return 6;
 for(selected=0;selected<2;++selected)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned equal=0;equal<2;++equal)
 for(float density:{-1.f,.5f,2.f,std::numeric_limits<float>::quiet_NaN()}){
  equalBounds=equal!=0;float lo=2,hi=4,second=3;std::memcpy(owner.data()+0x2f4c,&lo,4);std::memcpy(owner.data()+0x2f50,&hi,4);std::memcpy(owner.data()+0x18a0,&second,4);std::memcpy(owner.data()+0x2f54,&density,4);
  comparison.fill(0xcccccccc);current.fill(0);uploads=0;Ops ops{density};const auto expected=studioAuxPixelConstantCandidate(17,enabled,ops);
  native(owner.data(),17,enabled);
  if(uploads!=1||submitted!=expected||std::memcmp(current.data()+68,expected.data(),16)||std::memcmp(comparison.data()+68,expected.data(),16))return 4;
  oracle.write(reinterpret_cast<const char*>(submitted.data()),16);if(!oracle)return 7;
  native(owner.data(),17,enabled);if(uploads!=1)return 5;++cases;
  packet.fill(0xcccccccc);word(0x658,1);word(0x660,reinterpret_cast<uintptr_t>(packet.data()));word(0x664,7);
  comparison.fill(0xcccccccc);current.fill(0);uploads=0;
  native(owner.data(),17,enabled);
  if(uploads||packet[0]!=7||packet[1]!=17||packet[2]!=1||std::memcmp(packet.data()+3,expected.data(),16)||packet[7]!=0xcccccccc)return 8;
  uintptr_t cursor=0;uint32_t remaining=99;std::memcpy(&cursor,owner.data()+0x660,4);std::memcpy(&remaining,owner.data()+0x664,4);
  if(cursor!=reinterpret_cast<uintptr_t>(packet.data()+7)||remaining||std::memcmp(current.data()+68,expected.data(),16)||std::memcmp(comparison.data()+68,expected.data(),16))return 9;
  native(owner.data(),17,enabled);uintptr_t repeat=0;std::memcpy(&repeat,owner.data()+0x660,4);if(repeat!=cursor||uploads)return 10;
  ++queuedCases;word(0x658,0);
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_aux_pixel_cases=%u native_aux_queue_cases=%u callback_index_and_bounds=1 raw_float4_cache_reuse=1\n",cases,queuedCases);
}
