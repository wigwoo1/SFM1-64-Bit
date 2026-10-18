#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "owner_resource_cleanup_native_init.h"
#include "studio_owner_resource_cleanup_candidate.h"
struct Object{void** table;unsigned id;};static void* table[3]{};static std::array<Object,21> objects{};static Object** liveSlots;static unsigned char* active;static unsigned count,mutation;static std::vector<unsigned> trace;static unsigned observations;
static unsigned __stdcall releaseObject(Object* object){trace.push_back(object->id);observations=observations*2+(*active!=0);if(object->id==1&&mutation){liveSlots[count-1]=mutation==1?nullptr:&objects[20];liveSlots[0]=&objects[20];}return 0;}
struct Services{void release(Object* object){releaseObject(object);}};
struct Owner{unsigned char resourceQueueActive;std::array<Object*,20> queuedResources;std::array<Object*,5> fixedResources;};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 1;
 for(unsigned which=0;which<2;++which){const auto* bytes=which?ownerResourceSecondBytes:ownerResourceFirstBytes;const unsigned size=which?sizeof(ownerResourceSecondBytes):sizeof(ownerResourceFirstBytes),offset=which?ownerResourceSecondOffset:ownerResourceFirstOffset;dll.seekg(offset);dll.read(reinterpret_cast<char*>(code+which*256),size);if(!dll||std::memcmp(code+which*256,bytes,size))return 2;}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);std::ofstream oracle("outputs/native-owner-resource-cleanup.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-owner-resource-cleanup.bin",std::ios::binary);
#endif
 table[2]=reinterpret_cast<void*>(&releaseObject);for(unsigned i=0;i<21;++i)objects[i]={table,i+1};unsigned cases=0;
 for(unsigned which:{0u,1u})for(unsigned pattern:{0u,1u,2u,3u})for(unsigned change:{0u,1u,2u}){
 count=which?5u:20u;mutation=change;trace.clear();observations=0;Owner owner{};owner.resourceQueueActive=1;auto* slots=which?owner.fixedResources.data():owner.queuedResources.data();for(unsigned i=0;i<count;++i)slots[i]=(pattern==0||(pattern==1&&i%2==0)||(pattern==2&&i==0))?&objects[i]:nullptr;
#ifdef _M_IX86
 std::array<uint32_t,180> original{};original[0xc8/4]=1;auto* originalSlots=reinterpret_cast<Object**>(original.data()+(which?0x178:0xcc)/4);for(unsigned i=0;i<count;++i)originalSlots[i]=slots[i];liveSlots=originalSlots;active=reinterpret_cast<unsigned char*>(original.data())+0xc8;auto native=reinterpret_cast<void(__thiscall*)(void*)>(code+which*256);native(original.data());owner.resourceQueueActive=*active;for(unsigned i=0;i<count;++i)slots[i]=originalSlots[i];
#else
 liveSlots=slots;active=&owner.resourceQueueActive;Services services;if(which)studioOwnerSecondResourceCleanupCandidate(owner,services);else studioOwnerFirstResourceCleanupCandidate(owner,services);
#endif
 std::array<uint32_t,44> record{owner.resourceQueueActive,observations,static_cast<unsigned>(trace.size())};for(unsigned i=0;i<trace.size();++i)record[3+i]=trace[i];for(unsigned i=0;i<count;++i)record[24+i]=slots[i]?slots[i]->id:0;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,44> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch=%u\n",cases);return 4;}
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("owner_resource_cleanup_cases=%u passed\n",cases);
}
