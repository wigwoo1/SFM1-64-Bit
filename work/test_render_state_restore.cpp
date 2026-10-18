#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "render_state_restore_native_init.h"
#include "studio_render_state_restore_submit_candidate.h"
static unsigned suppression,directCalls;static HRESULT result;
static HRESULT __stdcall direct(void*,unsigned key,unsigned value){if(key!=8||value!=3)std::abort();++directCalls;return result;}
#ifdef _M_IX86
static unsigned char __fastcall suppressed(void*,void*){return static_cast<unsigned char>(suppression);}
#endif
struct Services{bool queued;unsigned suppressed(){return suppression;}bool queueEnabled(){return queued;}void directState(unsigned key,unsigned value){direct(nullptr,key,value);}void publish(uint32_t*){std::abort();}uint32_t* acquire(unsigned){if(suppression!=999u)std::abort();return nullptr;}void flush(){std::abort();}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeRenderStateRestoreBytes)> bytes{};dll.seekg(nativeRenderStateRestoreOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeRenderStateRestoreBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size()); // Refill is trapped: this grid supplies sufficient capacity.
 code[54]=0xcc;DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,unsigned,unsigned)>(code);std::ofstream oracle("outputs/native-render-state-restore.bin",std::ios::binary|std::ios::trunc);void* ownerTable[37]{};ownerTable[36]=reinterpret_cast<void*>(&suppressed);void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&direct);void* device=deviceTable;
#else
 std::ifstream oracle("outputs/native-render-state-restore.bin",std::ios::binary);
#endif
 unsigned cases=0;for(unsigned gate:{0u,1u,256u})for(bool queued:{false,true})for(bool equal:{false,true})for(HRESULT hr:{S_OK,E_FAIL}){suppression=gate;result=hr;directCalls=0;std::array<uint32_t,8> block{};block.fill(99);std::array<uint32_t,16> cache{};cache[8]=equal?3:77;unsigned remaining=3,cursor=0;
#ifdef _M_IX86
 std::array<uint32_t,2048> original{};original[0]=reinterpret_cast<uintptr_t>(ownerTable);original[0x648/4]=reinterpret_cast<uintptr_t>(&device);original[0x658/4]=queued?1:0;original[0x660/4]=reinterpret_cast<uintptr_t>(block.data());original[0x664/4]=3;original[0x1c98/4+8]=cache[8];native(original.data(),8,3);remaining=original[0x664/4];cursor=(original[0x660/4]-reinterpret_cast<uintptr_t>(block.data()))/4;cache[8]=original[0x1c98/4+8];
#else
 StudioDeviceQueueStateCandidate queue{block.data(),block.data(),3};StudioRenderStateOwnerCandidate owner{cache.data(),&queue};Services services{queued};studioRenderStateRestoreSubmitCandidate(owner,8,3,services);remaining=queue.remaining;cursor=static_cast<unsigned>(queue.cursor-block.data());
#endif
 std::array<uint32_t,12> record{cache[8],directCalls,remaining,cursor};for(unsigned i=0;i<8;++i)record[4+i]=block[i];
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,12> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("render_state_restore_native_cases=%u passed\n",cases);
}

