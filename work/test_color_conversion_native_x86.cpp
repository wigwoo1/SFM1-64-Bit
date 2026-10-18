#define NOMINMAX
#include <Windows.h>
#include "color_conversion_native_init.h"
#include "color_lookup_native_bytes.h"
#include "studio_shader_color_conversion_candidate.h"
#include "studio_color_lookup_candidate.h"
#include <fstream>
#include <cstdio>
static uint32_t serviceMode=0;
static uint32_t __fastcall modeService(void*,void*){return serviceMode;}
struct Ops {
 uint8_t a,b;float converted,raw;const std::array<float,256>& table;
 uint8_t flag5aa(){return a;}uint8_t flag5a9(){return b;}uint32_t mode(){return serviceMode;}
 float convertedScale(){return converted;}float rawScale(){return raw;}
 float lookup(float value){return studioColorLookupCandidate(value,table,StudioColorLookupRoundingCandidate::NearestEven);}
};
int main(){
 std::array<float,256> table{};std::ifstream values("outputs/native-full-generator-color-table.bin",std::ios::binary);values.read(reinterpret_cast<char*>(table.data()),1024);if(!values)return 1;
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 unsigned char original[384];dll.seekg(nativeColorConversionFileOffset);dll.read(reinterpret_cast<char*>(original),384);if(!dll||std::memcmp(original,nativeColorConversionBytes,384))return 2;
 unsigned char lookup[71];dll.seekg(colorLookupFileOffset);dll.read(reinterpret_cast<char*>(lookup),71);if(!dll||std::memcmp(lookup,colorLookupBytes,71))return 3;
 std::array<uintptr_t,35> vtable{};vtable[34]=reinterpret_cast<uintptr_t>(&modeService);
 std::array<unsigned char,0x5b0> service{};uintptr_t vt=reinterpret_cast<uintptr_t>(vtable.data());std::memcpy(service.data(),&vt,4);
 uintptr_t servicePointer=reinterpret_cast<uintptr_t>(service.data());
 float normalize=1.f/255.f,one=1.f,scale=255.f,threshold=0.95f;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 4;
 std::memcpy(code,original,384);std::memcpy(code+1024,lookup,71);
 auto patch=[&](unsigned offset,const void* pointer){uint32_t p=reinterpret_cast<uintptr_t>(pointer);std::memcpy(code+offset,&p,4);};
 for(auto& f:nativeColorConversionFixups){const void* p=f[1]==0x1016d130?static_cast<void*>(&servicePointer):f[1]==0x100fc764?&normalize:f[1]==0x100fa5f8?&one:&scale;patch(f[0],p);}
 for(auto offset:nativeColorConversionCalls){int32_t delta=1024-offset-5;std::memcpy(code+offset+1,&delta,4);}
 patch(1024+0x1a,&threshold);patch(1024+0x2a,&scale);patch(1024+0x3f,table.data());
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 5;FlushInstructionCache(GetCurrentProcess(),code,4096);
 std::array<unsigned char,0x3200> owner{};
 auto native=reinterpret_cast<uint32_t(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t)>(code);
 unsigned short originalControl=0,nearest=0;__asm fnstcw originalControl
 nearest=static_cast<unsigned short>(originalControl&~0xc00u);__asm fldcw nearest
 unsigned cases=0;uint64_t digest=1469598103934665603ull;
 for(unsigned config=0;config<4;++config)for(serviceMode=0;serviceMode<3;++serviceMode)for(float factor:{0.5f,1.f,2.f})for(uint32_t flag:{0u,1u,256u,257u})for(unsigned i=0;i<256;++i){
  service[0x5aa]=uint8_t(config&1);service[0x5a9]=uint8_t(config&2?128:0);
  float rawFactor=factor*0.5f;std::memcpy(owner.data()+0x3114,&factor,4);std::memcpy(owner.data()+0x3120,&rawFactor,4);
  const std::array<uint8_t,3> rgb{uint8_t(i),uint8_t(i*17),uint8_t(i*31)};
  const auto expected=native(owner.data(),rgb[0],rgb[1],rgb[2],flag);
  Ops ops{service[0x5aa],service[0x5a9],factor,rawFactor,table};const auto got=studioShaderColorConversionCandidate(rgb,flag,ops);
  if(got!=expected){std::printf("conversion_mismatch case=%u native=%08x candidate=%08x\n",cases,expected,got);return 6;}
  digest=(digest^expected)*1099511628211ull;++cases;
 }
 __asm fldcw originalControl
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_color_conversion_cases=%u digest=%llu controlled_service=1 native_generated_table=1\n",cases,static_cast<unsigned long long>(digest));return 0;
}
