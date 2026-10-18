#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "extended_vertex_constant_registration_native_init.h"
#include "studio_extended_vertex_constants_candidate.h"
static std::vector<uint32_t> events;static bool mutate;static uint8_t* liveFlags;
static const char warningText[]="Attempted to set too many flex weights! Max is %d\n";
#ifdef _M_IX86
static void callback(){}
#endif
static void __cdecl warning(const char* text,int limit){if(std::strcmp(text,warningText)||limit!=512)std::abort();events.insert(events.end(),{1,512});}
static void* __cdecl copy(void* destination,const void* source,uint32_t bytes){events.insert(events.end(),{2,bytes});return std::memmove(destination,source,bytes);}
static void appendPolicy(){events.push_back(3);if(mutate)*liveFlags|=0x40;}
#ifdef _M_IX86
static uint32_t* completeOwner;
static void __fastcall append(void* owner,void*,uint32_t category,void* target){if(owner!=completeOwner||category||target!=reinterpret_cast<void*>(&callback))std::abort();appendPolicy();}
#endif
struct Services{
 int32_t capacity;float* values;
 int32_t vertexFloatCapacity(){return capacity;}float* staging(){return values;}uint8_t& registrationFlags(){return *liveFlags;}
 void warnClipped(unsigned limit){warning(warningText,static_cast<int>(limit));}void enqueueExtendedConstants(){appendPolicy();}
 void copy(float* destination,const float* source,uint32_t bytes){::copy(destination,source,bytes);}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(extendedVertexRegistrationBytes)> bytes{};dll.seekg(extendedVertexRegistrationFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),extendedVertexRegistrationBytes,bytes.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());std::array<uint32_t,336> capability{};uint32_t* capabilityPointer=capability.data();auto warningFunction=&warning;
 for(auto& fixup:extendedVertexRegistrationFixups){uintptr_t target=fixup[1]==0x1016d130?reinterpret_cast<uintptr_t>(&capabilityPointer):fixup[1]==0x100fd424?reinterpret_cast<uintptr_t>(warningText):fixup[1]==0x100fa244?reinterpret_cast<uintptr_t>(&warningFunction):fixup[1]==0x10011610?reinterpret_cast<uintptr_t>(&callback):0;if(!target)return 3;const uint32_t address=static_cast<uint32_t>(target);std::memcpy(code+fixup[0],&address,4);}
 for(auto& call:extendedVertexRegistrationCalls){const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(&append);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 4;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,int32_t,int32_t,const float*)>(code);
 std::ofstream oracle("outputs/native-extended-vertex-registration.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-extended-vertex-registration.bin",std::ios::binary);
#endif
 unsigned cases=0;for(int32_t capacity:{-1,256,1535,1536,2048})for(int32_t start:{0,1,510,512})for(int32_t count:{0,1,4,512})for(uint8_t initial:{uint8_t{0},uint8_t{2},uint8_t{0x80}})for(bool mutation:{false,true}){
  events.clear();mutate=mutation;std::array<float,6148> values{};values.fill(-7);std::array<float,2048> source{};for(unsigned i=0;i<source.size();++i)source[i]=static_cast<float>(i+1);uint8_t flags=initial;liveFlags=&flags;
#ifdef _M_IX86
  capability[0x534/4]=static_cast<uint32_t>(capacity);std::array<uint32_t,4096> owner{};completeOwner=owner.data();auto* sub=owner.data()+0x2d0/4;sub[0x2588/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(values.data()));liveFlags=reinterpret_cast<uint8_t*>(sub)+0x2bb8;*liveFlags=initial;native(sub,start,count,source.data());flags=*liveFlags;
#else
  Services services{capacity,values.data()};studioExtendedVertexConstantsCandidate(start,count,source.data(),services);
#endif
  std::array<uint32_t,6170> record{};record[0]=flags;record[1]=static_cast<uint32_t>(events.size());std::memcpy(record.data()+2,values.data(),values.size()*4);std::copy(events.begin(),events.end(),record.begin()+6150);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,6170> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch cap=%d start=%d count=%d flags=%u mutation=%u\n",capacity,start,count,initial,mutation);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 std::printf("extended_vertex_registration_original_cases=%u passed\n",cases);
}
