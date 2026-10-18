#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "shader_binding_native_init.h"
#include "native_shader_binding_state_candidate.h"
#include "manager_unbind_prefix_native_init.h"
#include "shader_manager_restore_reset_native_init.h"
struct Shader{};
static uint32_t counter,captures,directCalls,shaderValue,stageValue;static HRESULT result;
static std::array<uint32_t,8> fixtureCommand,block;
static HRESULT observe(bool vertex,Shader* shader){++directCalls;stageValue=vertex?1u:2u;shaderValue=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(shader));return result;}
#ifdef _M_IX86
static uint32_t* __cdecl getter(){++captures;return fixtureCommand.data();}
static void __fastcall forbiddenRefill(void*,void*){std::abort();}
static void __cdecl forbiddenExit(){std::abort();}
static HRESULT __stdcall directVertex(void*,Shader* shader){return observe(true,shader);}
static HRESULT __stdcall directPixel(void*,Shader* shader){return observe(false,shader);}
#endif
struct Device{HRESULT SetVertexShader(Shader* shader){return observe(true,shader);}HRESULT SetPixelShader(Shader* shader){return observe(false,shader);}};
struct Lifecycle{
 bool queued;Device object;uint32_t remaining=2,cursor=1;
 void recordBind(bool){++counter;}Device* commandState(){++captures;return &object;}
 bool queueEnabled(Device*){return queued;}Device* device(Device* p){return p;}
 void queueShader(Device*,bool vertex,Shader* shader){if(remaining<2)std::abort();remaining-=2;block[cursor++]=vertex?6:5;block[cursor++]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(shader));}
};
int main(){
#ifdef _M_IX86
 std::array<unsigned char*,2> code{};std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);static uint32_t initialized=1;static uint32_t* counterPointer=&counter;static uint32_t unused=0;
 for(unsigned i=0;i<2;++i){const auto& body=shaderBindingBodies[i];std::array<unsigned char,178> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),body.bytes,bytes.size()))return 1;code[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code[i])return 2;std::memcpy(code[i],body.bytes,body.size);
  for(unsigned f=0;f<body.fixupCount;++f){const auto& fixup=body.fixups[f];const void* target=fixup[1]==0x10184218||fixup[1]==0x10184220?static_cast<void*>(&initialized):fixup[1]==0x10184214||fixup[1]==0x1018421c?static_cast<void*>(&counterPointer):static_cast<void*>(&unused);const uint32_t address=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target));std::memcpy(code[i]+fixup[0],&address,4);}
  for(unsigned c=0;c<body.callCount;++c){const auto& call=body.calls[c];const uintptr_t target=call[1]==0x1000df50?reinterpret_cast<uintptr_t>(&getter):call[1]==0x10002da0?reinterpret_cast<uintptr_t>(&forbiddenRefill):reinterpret_cast<uintptr_t>(&forbiddenExit);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code[i]+call[0]+5));std::memcpy(code[i]+call[0]+1,&delta,4);}
  // Initialization flag is fixed true; trap entry to its external-services path.
  code[i][31]=0x0f;code[i][32]=0x0b;
  DWORD previous;if(!VirtualProtect(code[i],4096,PAGE_EXECUTE_READ,&previous))return 3;FlushInstructionCache(GetCurrentProcess(),code[i],4096);
 }
 std::array<unsigned char*,2> prefixCode{};
 for(unsigned i=0;i<2;++i){const auto& body=managerUnbindPrefixBodies[i];std::array<unsigned char,38> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),body.size);if(!dll||std::memcmp(bytes.data(),body.bytes,body.size))return 7;prefixCode[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!prefixCode[i])return 8;std::memcpy(prefixCode[i],bytes.data(),body.size);prefixCode[i][body.size]=0x0f;prefixCode[i][body.size+1]=0x0b;const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(code[i])-reinterpret_cast<uintptr_t>(prefixCode[i]+body.callOffset+5));std::memcpy(prefixCode[i]+body.callOffset+1,&delta,4);DWORD previous;if(!VirtualProtect(prefixCode[i],4096,PAGE_EXECUTE_READ,&previous))return 9;FlushInstructionCache(GetCurrentProcess(),prefixCode[i],4096);}
 std::array<unsigned char,sizeof(managerRestoreResetBytes)> managerBytes{};dll.seekg(managerRestoreResetFileOffset);dll.read(reinterpret_cast<char*>(managerBytes.data()),managerBytes.size());if(!dll||std::memcmp(managerBytes.data(),managerRestoreResetBytes,managerBytes.size()))return 10;
 auto* managerCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!managerCode)return 11;std::memcpy(managerCode,managerBytes.data(),managerBytes.size());DWORD previous;if(!VirtualProtect(managerCode,4096,PAGE_EXECUTE_READ,&previous))return 12;FlushInstructionCache(GetCurrentProcess(),managerCode,4096);void* managerTable[12]{};managerTable[9]=prefixCode[0];managerTable[10]=prefixCode[1];
 void* table[108]{};table[92]=reinterpret_cast<void*>(&directVertex);table[107]=reinterpret_cast<void*>(&directPixel);void** object=table;
 std::ofstream oracle("outputs/native-shader-binding.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-shader-binding.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool vertex:{false,true})for(bool queued:{false,true})for(bool nonnull:{false,true})for(unsigned initial=0;initial<3;++initial)for(uint32_t counterInitial:{0u,0xffffffffu})for(HRESULT hr:{S_OK,E_FAIL}){
  counter=counterInitial;captures=directCalls=shaderValue=stageValue=0;result=hr;block.fill(0);fixtureCommand.fill(0);const uint32_t desired=nonnull?0x1234u:0u;const uint32_t oldValue=initial==0?desired:initial==1?0x5678u:0xffffffffu;uint32_t finalValue=oldValue,remaining=2,cursor=1;auto* shader=reinterpret_cast<Shader*>(static_cast<uintptr_t>(desired));
#ifdef _M_IX86
  std::array<uint32_t,41> owner{};owner[(vertex?0x9c:0xa0)/4]=oldValue;fixtureCommand[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&object));fixtureCommand[4]=queued?1:0;fixtureCommand[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data()+1));fixtureCommand[7]=2;auto native=reinterpret_cast<void(__thiscall*)(void*,Shader*,uint32_t)>(code[vertex?0:1]);native(owner.data(),shader,0);finalValue=owner[(vertex?0x9c:0xa0)/4];remaining=fixtureCommand[7];cursor=(fixtureCommand[6]-static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data())))/4;
#else
  NativeShaderBindingStateCandidateT<Shader> cache{reinterpret_cast<Shader*>(static_cast<uintptr_t>(oldValue)),initial!=2};Lifecycle lifecycle{queued};if(vertex)nativeShaderBindingCandidate<true>(cache,shader,lifecycle);else nativeShaderBindingCandidate<false>(cache,shader,lifecycle);finalValue=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cache.shader));remaining=lifecycle.remaining;cursor=lifecycle.cursor;
#endif
  std::array<uint32_t,16> record{counter,captures,directCalls,stageValue,finalValue,remaining,cursor,shaderValue};std::memcpy(record.data()+8,block.data(),32);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,16> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
  if(!oracle)return 5;++cases;
 }
 unsigned restoreCases=0;
 for(bool queued:{false,true})for(unsigned initial=0;initial<3;++initial)for(uint32_t counterInitial:{0u,0xffffffffu})for(HRESULT hr:{S_OK,E_FAIL}){
  counter=counterInitial;captures=directCalls=shaderValue=stageValue=0;result=hr;block.fill(0);fixtureCommand.fill(0);const uint32_t oldValue=initial==0?0u:initial==1?0x1234u:0xffffffffu;uint32_t remaining=4,cursor=1;
#ifdef _M_IX86
  std::array<uint32_t,41> owner{};owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(managerTable));owner[0x9c/4]=owner[0xa0/4]=oldValue;fixtureCommand[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&object));fixtureCommand[4]=queued?1:0;fixtureCommand[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data()+1));fixtureCommand[7]=4;auto native=reinterpret_cast<void(__thiscall*)(void*)>(managerCode);native(owner.data());if(owner[0x9c/4]||owner[0xa0/4])return 13;remaining=fixtureCommand[7];cursor=(fixtureCommand[6]-static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data())))/4;
#else
  NativeShaderBindingStateCandidateT<Shader> vertex{reinterpret_cast<Shader*>(static_cast<uintptr_t>(oldValue)),initial!=2},pixel=vertex;Lifecycle lifecycle{queued};lifecycle.remaining=4;nativeShaderRestoreUnbindCandidate(vertex,pixel,lifecycle);if(!vertex.valid||!pixel.valid||vertex.shader||pixel.shader)return 13;remaining=lifecycle.remaining;cursor=lifecycle.cursor;
#endif
  std::array<uint32_t,16> record{counter,captures,directCalls,stageValue,0,remaining,cursor,shaderValue};std::memcpy(record.data()+8,block.data(),32);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,16> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 14;
#endif
  if(!oracle)return 15;++restoreCases;
 }
#ifdef _M_IX86
 for(auto* body:code)VirtualFree(body,0,MEM_RELEASE);for(auto* body:prefixCode)VirtualFree(body,0,MEM_RELEASE);VirtualFree(managerCode,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("shader_binding_native_cases=%u restore_chain_cases=%u passed\n",cases,restoreCases);
}

