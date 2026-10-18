#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "reset_texture_native_init.h"
#include "native_reset_texture_recreation_candidate.h"
struct Object{void** table;unsigned id;};static void* textureTable[3]{};static void* deviceTable[24]{};static Object oldTexture{textureTable,1},newTexture{textureTable,2},firstDevice{deviceTable,3},secondDevice{deviceTable,4};static Object* currentDevice;static HRESULT result;static bool replace;static unsigned releases,captures,flushes,receiver;static Object** liveOutput;
static ULONG __stdcall releaseTexture(Object* object){if(object!=&oldTexture||*liveOutput!=object)std::abort();++releases;return 0;}
static HRESULT __stdcall createTexture(Object* device,UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT fmt,D3DPOOL pool,Object** output,HANDLE* shared){if(w!=1||h!=1||levels!=0||usage!=0x200||fmt!=21||pool!=0||shared||*output||output!=liveOutput||flushes!=1)std::abort();receiver=device->id;*output=&newTexture;return result;}
#ifdef _M_IX86
static Object** __cdecl getter(){++captures;return &currentDevice;}
static void __fastcall flush(Object**,void*){++flushes;if(replace)currentDevice=&secondDevice;}
#endif
struct Owner{Object*& resource;};struct Services{void release(Object* object){releaseTexture(object);}Object** commandState(){++captures;return &currentDevice;}void flush(Object**){++flushes;if(replace)currentDevice=&secondDevice;}Object* device(Object** state){return *state;}HRESULT createTexture(Object* device,Object*& output){return ::createTexture(device,1,1,0,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&output,nullptr);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeResetTextureBytes)> bytes{};dll.seekg(nativeResetTextureOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeResetTextureBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());for(auto& call:nativeResetTextureCalls){uintptr_t target=call[1]==0x1000df50?reinterpret_cast<uintptr_t>(&getter):reinterpret_cast<uintptr_t>(&flush);uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-reset-texture.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-reset-texture.bin",std::ios::binary);
#endif
 textureTable[2]=reinterpret_cast<void*>(&releaseTexture);deviceTable[23]=reinterpret_cast<void*>(&createTexture);unsigned cases=0;
 for(bool present:{false,true})for(bool swap:{false,true})for(HRESULT hr:{S_OK,S_FALSE,E_FAIL,static_cast<HRESULT>(0x80000000u),static_cast<HRESULT>(0x7fffffffu)}){result=hr;replace=swap;currentDevice=&firstDevice;releases=captures=flushes=receiver=0;Object* output=present?&oldTexture:nullptr;
#ifdef _M_IX86
 std::array<uint32_t,180> original{};liveOutput=reinterpret_cast<Object**>(original.data()+0xc4/4);*liveOutput=output;native(original.data());output=*liveOutput;
#else
 liveOutput=&output;Owner owner{output};Services services;studioResetTextureRecreationCandidate(owner,services);
#endif
 std::array<uint32_t,5> record{releases,captures,flushes,receiver,output?output->id:0};
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,5> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("reset_texture_native_cases=%u passed\n",cases);
}
