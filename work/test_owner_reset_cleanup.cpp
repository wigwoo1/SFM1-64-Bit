#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "owner_reset_cleanup_native_init.h"
#include "studio_owner_reset_cleanup_candidate.h"
static bool owning,mainThread;static unsigned notificationMode;static std::vector<unsigned> trace;static uint32_t warningDepth;static void* notification;
struct Interface{void** table;};static void* tableA[250]{};static void* tableB[38]{};static void* tableC[3]{};static Interface a{tableA},b{tableB},c{tableC};static void* dispatch=&b;
static unsigned char __cdecl mainGate(){trace.push_back(2);return mainThread;}
static void __cdecl warning(const char*,uint32_t depth){warningDepth=depth;trace.push_back(4);}
static void __cdecl diagnostic(const char*){trace.push_back(5);}
#ifdef _M_IX86
static unsigned char __fastcall ownerGate(void*,void*){trace.push_back(1);return owning;}
static void __fastcall begin(void*,void*,uint32_t value,const char*){if(value!=0xfff5940f)std::abort();trace.push_back(6);if(notificationMode==2)notification=nullptr;}
static void __fastcall end(void*,void*){trace.push_back(7);}
static void __fastcall query(void*,void*){trace.push_back(8);}
static void __fastcall first(void*,void*){trace.push_back(9);}
static void __fastcall second(void*,void*){trace.push_back(10);notification=&a;}
static void __fastcall mode(void*,void*,unsigned value){trace.push_back(11+value);}
static void __fastcall finish(void*,void*){trace.push_back(14);}
static void __fastcall publish(void*,void*,unsigned value){trace.push_back(15+value);}
static void __fastcall defer(void*,void*,unsigned value){if(value!=1)std::abort();trace.push_back(17);}
static void* __cdecl getter(){return &c;}
#endif
struct Owner{unsigned char flags;uint32_t cleanupDepth;};
struct Services{
 bool inOwningThread(Owner&){trace.push_back(1);return owning;}bool inMainThread(){return mainGate()!=0;}
 void deferCleanup(bool){trace.push_back(17);}void nestedWarning(uint32_t d){warning("",d);}void nestedDiagnostic(){diagnostic("");}
 bool notificationAvailable(){return notification!=nullptr;}void beginNotification(uint32_t value){if(value!=0xfff5940f)std::abort();trace.push_back(6);if(notificationMode==2)notification=nullptr;}void endNotification(){trace.push_back(7);}
 void queryCleanup(Owner&){trace.push_back(8);}void cleanupFirst(Owner&){trace.push_back(9);}void cleanupSecond(Owner&){trace.push_back(10);notification=&a;}
 void setCleanupMode(unsigned v){trace.push_back(11+v);}void finishCommandState(){trace.push_back(14);}void publishCleanupEnabled(bool v){trace.push_back(15+static_cast<unsigned>(v));}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeOwnerResetCleanupBytes)> installed{};dll.seekg(nativeOwnerResetCleanupOffset);dll.read(reinterpret_cast<char*>(installed.data()),installed.size());if(!dll||std::memcmp(installed.data(),nativeOwnerResetCleanupBytes,installed.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed.data(),installed.size());
 void* mainCell=reinterpret_cast<void*>(&mainGate);void* warningCell=reinterpret_cast<void*>(&warning);void* diagnosticCell=reinterpret_cast<void*>(&diagnostic);const char* label="controlled";
 for(auto& fix:nativeOwnerResetCleanupFixups){uintptr_t value=0;switch(fix[1]){
 case 0x100fa20c:value=reinterpret_cast<uintptr_t>(&mainCell);break;case 0x100fa244:value=reinterpret_cast<uintptr_t>(&warningCell);break;case 0x100fa1f4:value=reinterpret_cast<uintptr_t>(&diagnosticCell);break;
 case 0x10183b4c:value=reinterpret_cast<uintptr_t>(&notification);break;case 0x10183b58:value=reinterpret_cast<uintptr_t>(&dispatch);break;
 case 0x100fe808:case 0x100fe7b0:case 0x100fe79c:value=reinterpret_cast<uintptr_t>(label);break;default:return 3;}
 std::memcpy(code+fix[0],&value,4);}
 for(auto& call:nativeOwnerResetCleanupCalls){uintptr_t target=0;switch(call[1]){case 0x100283e0:target=reinterpret_cast<uintptr_t>(&ownerGate);break;case 0x1002b300:target=reinterpret_cast<uintptr_t>(&query);break;case 0x1002aaa0:target=reinterpret_cast<uintptr_t>(&first);break;case 0x1002abe0:target=reinterpret_cast<uintptr_t>(&second);break;case 0x10005bc0:target=reinterpret_cast<uintptr_t>(&getter);break;default:return 4;}const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 tableA[185]=reinterpret_cast<void*>(&begin);tableA[186]=reinterpret_cast<void*>(&end);tableA[249]=reinterpret_cast<void*>(&publish);tableB[14]=reinterpret_cast<void*>(&mode);tableB[37]=reinterpret_cast<void*>(&defer);tableC[2]=reinterpret_cast<void*>(&finish);
 DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 5;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,unsigned)>(code);std::ofstream oracle("outputs/native-owner-reset-cleanup.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-owner-reset-cleanup.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool own:{false,true})for(bool main:{false,true})for(uint32_t depth:{0u,1u,0x7fffffffu,0xffffffffu})for(bool enabled:{false,true})for(unsigned notify:{0u,1u,2u}){
 owning=own;mainThread=main;notificationMode=notify;notification=notify?&a:nullptr;trace.clear();warningDepth=0;Owner owner{0x55,depth};
#ifdef _M_IX86
 std::array<uint32_t,180> original{};original[0xac/4]=0x55;original[0x2cc/4]=depth;native(original.data(),enabled?1u:0u);owner.flags=static_cast<unsigned char>(original[0xac/4]);owner.cleanupDepth=original[0x2cc/4];
#else
 Services services;studioOwnerResetCleanupCandidate(owner,services,enabled);
#endif
 std::array<uint32_t,20> record{owner.flags,owner.cleanupDepth,warningDepth,notification?1u:0u,static_cast<uint32_t>(trace.size())};if(trace.size()>15)return 6;for(unsigned i=0;i<trace.size();++i)record[5+i]=trace[i];
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,20> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch case=%u\n",cases);return 7;}
#endif
 if(!oracle)return 8;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 9;
#endif
 std::printf("owner_reset_cleanup_cases=%u passed\n",cases);
}

