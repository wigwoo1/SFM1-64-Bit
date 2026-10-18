#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_material_buffer_resize_candidate.h"
static uint32_t stats[10];static bool succeeds;
static void check(bool v){if(!v)throw std::runtime_error("resize parity");}
static void put32(unsigned char* p,uint32_t v){std::memcpy(p,&v,4);}
#ifndef _WIN64
#include "material_buffer_resize_native_init.h"
static void* __stdcall commit(void* address,uint32_t bytes,uint32_t type,uint32_t protection){++stats[0];stats[4]=uint32_t(address);stats[5]=bytes;stats[6]=type;stats[7]=protection;return succeeds?address:nullptr;}
static bool __stdcall decommit(void* address,uint32_t bytes,uint32_t type){++stats[1];stats[4]=uint32_t(address);stats[5]=bytes;stats[6]=type;return true;}
static void __cdecl warn(const char*){++stats[2];}
static void __fastcall oom(void*,void*,uint32_t bytes){++stats[3];stats[8]=bytes;}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);uint32_t table[36]={};table[35]=uint32_t(&oom);uint32_t object=uint32_t(table),pointer=uint32_t(&object);initializeMaterialBufferResizeNative(code,uint32_t(&commit),uint32_t(&decommit),uint32_t(&warn),uint32_t(&pointer));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Resize=bool(__thiscall*)(void*,uint32_t);auto resize=reinterpret_cast<Resize>(code);
#endif
for(unsigned oldPages=0;oldPages<=16;++oldPages)for(unsigned requested=0;requested<=34;++requested)for(unsigned offset=0;offset<2;++offset)for(unsigned dirty=0;dirty<2;++dirty)for(unsigned success=0;success<2;++success)for(unsigned position=0;position<3;++position){std::memset(stats,0,sizeof(stats));succeeds=success!=0;uint32_t base=0x100000,alignment=4096,oldEnd=base+oldPages*alignment,cursor=base+(position==0?0:position==1?oldPages/2:oldPages)*alignment,target=base+requested*alignment+offset;unsigned char buffer[40];std::memset(buffer,0xa5,sizeof(buffer));put32(buffer,cursor);put32(buffer+4,oldEnd);put32(buffer+8,base+32*alignment);put32(buffer+12,base);buffer[16]=static_cast<unsigned char>(dirty);put32(buffer+28,alignment);
#ifdef _WIN64
StudioMaterialBufferCandidate b{cursor,oldEnd,base+32*alignment,base,dirty!=0,alignment};stats[9]=studioMaterialBufferResizeCandidate(b,target,[&](uintptr_t address,uintptr_t bytes,int type,int protection){++stats[0];stats[4]=uint32_t(address);stats[5]=uint32_t(bytes);stats[6]=type;stats[7]=protection;return succeeds;},[&](uintptr_t address,uintptr_t bytes,int type){++stats[1];stats[4]=uint32_t(address);stats[5]=uint32_t(bytes);stats[6]=type;return true;},[&]{++stats[2];},[&](uintptr_t bytes){++stats[3];stats[8]=uint32_t(bytes);});put32(buffer+4,uint32_t(b.committedEnd));buffer[16]=static_cast<unsigned char>(b.dirty);uint32_t expected[10];unsigned char expectedBuffer[40];check(bool(file.read(reinterpret_cast<char*>(expected),sizeof(expected))));check(bool(file.read(reinterpret_cast<char*>(expectedBuffer),40)));check(std::memcmp(stats,expected,sizeof(stats))==0&&std::memcmp(buffer,expectedBuffer,40)==0);
#else
stats[9]=resize(buffer,target);file.write(reinterpret_cast<char*>(stats),sizeof(stats));file.write(reinterpret_cast<char*>(buffer),40);
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"buffer_resize_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
