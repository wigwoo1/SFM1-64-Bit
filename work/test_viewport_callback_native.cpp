
#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "viewport_callback_init.h"
#include "studio_shader_viewport_submit_candidate.h"
static std::array<unsigned,8> command;
static std::array<unsigned,16> queue;
static StudioShaderViewportWordsCandidate* active;
static bool mutate;
static unsigned calls,cookie;
static void submit(const unsigned* words){if(words!=active->data())std::abort();++calls;if(mutate)(*active)[0]=42;}
#ifdef _M_IX86
static HRESULT __stdcall direct(void*,const unsigned* words){submit(words);return E_FAIL;}
static void __fastcall cookieCheck(unsigned value,void*){if(value!=cookie)std::abort();}
static void __fastcall refill(void*,void*){std::abort();}
#endif
struct Ops{
 unsigned cursor=0;
 bool queueEnabled(){return command[4]!=0;}
 void queueViewport(const StudioShaderViewportWordsCandidate& words){queue[cursor++]=20;for(auto w:words)queue[cursor++]=w;command[6]+=28;command[7]-=7;}
 void directViewport(const StudioShaderViewportWordsCandidate& words){submit(words.data());}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);unsigned char raw[249]{};dll.seekg(viewportOffset);dll.read(reinterpret_cast<char*>(raw),249);if(!dll||std::memcmp(raw,viewportBytes,249))return 1;
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,raw,249);
 for(auto fix:viewportFixups){auto address=fix[1]==0x1017c370?&cookie:command.data()+(fix[1]-0x10180aa8)/4;unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(address));std::memcpy(code+fix[0],&value,4);}
 for(auto call:viewportCalls){auto target=call[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):reinterpret_cast<uintptr_t>(&refill);unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD protection;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection)||!FlushInstructionCache(GetCurrentProcess(),code,249))return 3;
 auto callback=reinterpret_cast<void(__cdecl*)(void*,void*,void*,unsigned)>(code);
 void* table[48]{};table[47]=reinterpret_cast<void*>(&direct);void** device=table;
 std::ofstream records("outputs/native-viewport-callback.bin",std::ios::binary);
#else
 std::ifstream records("outputs/native-viewport-callback.bin",std::ios::binary);
#endif
 unsigned cases=0;
 for(unsigned force:{0u,256u,1u,257u})for(bool same:{false,true})for(bool zeroWidth:{false,true})for(bool zeroHeight:{false,true})for(bool queued:{false,true})for(bool mutation:{false,true}){
 StudioShaderViewportWordsCandidate live{9,7,zeroWidth?0u:512u,zeroHeight?0u:256u,0x80000000,0x3f800000};auto cache=live;if(!same)cache[4]=0;auto expectedLive=live,expectedCache=cache;active=&live;mutate=mutation;calls=0;
 queue.fill(0xcccccccc);auto expectedQueue=queue;command.fill(0xabababab);command[0]=0;command[4]=queued;command[6]=0;command[7]=16;auto expectedCommand=command;
 bool execute=(static_cast<unsigned char>(force)||!same)&&!zeroWidth&&!zeroHeight;
 if(execute){if(queued){expectedQueue[0]=20;for(unsigned i=0;i<6;++i)expectedQueue[i+1]=live[i];expectedCommand[6]=28;expectedCommand[7]=9;}else if(mutate)expectedLive[0]=42;expectedCache=expectedLive;}
#ifdef _M_IX86
 command[0]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(&device));command[6]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(queue.data()));
 callback(nullptr,live.data(),cache.data(),force);
 command[0]=0;command[6]-=static_cast<unsigned>(reinterpret_cast<uintptr_t>(queue.data()));
#else
 Ops ops;studioShaderViewportSubmitCandidate(live,cache,force,ops);
#endif
 if(live!=expectedLive||cache!=expectedCache||queue!=expectedQueue||command!=expectedCommand||calls!=(execute&&!queued?1u:0u))return 4;
 std::array<unsigned,37> record{};unsigned i=0;for(auto w:live)record[i++]=w;for(auto w:cache)record[i++]=w;for(auto w:queue)record[i++]=w;for(auto w:command)record[i++]=w;record[i++]=calls;
#ifdef _M_IX86
 records.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<unsigned,37> original{};records.read(reinterpret_cast<char*>(original.data()),sizeof(original));if(original!=record)return 5;
#endif
 if(!records)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(records.peek()!=std::char_traits<char>::eof())return 7;
#endif
 std::printf("viewport_callback_cases=%u passed",cases);return 0;
}
