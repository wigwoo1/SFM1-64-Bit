#define NOMINMAX
#include <windows.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_material_index_upload_candidate.h"
#include "studio_material_mesh_upload_candidate.h"
#include "studio_material_position_vertex_upload_candidate.h"
static void check(bool value){if(!value)throw std::runtime_error("index upload parity");}
static unsigned copies,locks,unlocks,count;static uint32_t offset;static uintptr_t destination,vertexDestination;static unsigned vertexMode;static uint32_t stride;
#ifndef _WIN64
#include "material_index_upload_native_init.h"
static void __cdecl copy(void* to,const void* from,size_t bytes){++copies;std::memcpy(to,from,bytes);}
static void __fastcall security(uint32_t,void*){}
static void __fastcall lock(void*,void*,int vertices,int indices,void* descriptor){check(vertices==int(vertexMode?count:0)&&indices==int(count));++locks;std::memset(descriptor,0,184);auto* fields=static_cast<uint32_t*>(descriptor);fields[0]=1;fields[0x54/4]=uint32_t(vertexDestination);fields[0x48/4]=stride;fields[0x9c/4]=offset;fields[0xa4/4]=uint32_t(destination);fields[0xb0/4]=1;}
static void __fastcall unlock(void*,void*,int vertices,int indices,void*){check(vertices==int(vertexMode?count:0)&&indices==int(count));++unlocks;}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);initializeIndexUploadNative(code,uint32_t(&copy),uint32_t(&security));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Upload=void(__thiscall*)(void*,uint32_t,int,int,uint32_t,int);auto upload=reinterpret_cast<Upload>(code);uint32_t table[21]={};table[17]=uint32_t(&lock);table[20]=uint32_t(&unlock);uint32_t underlying=uint32_t(table),mesh[20]={};mesh[2]=uint32_t(&underlying);
#endif
for(count=1;count<=257;++count)for(unsigned alignment: {0u,2u})for(uint32_t value: {0u,1u,32767u,65535u,65536u})for(vertexMode=0;vertexMode<5;++vertexMode){offset=value;copies=locks=unlocks=0;std::vector<unsigned char> source(count*2+64,0xa5),result(count*2+64,0xa5);auto pointer=reinterpret_cast<uintptr_t>(source.data()+16);destination=reinterpret_cast<uintptr_t>(result.data()+16+alignment);check((destination&3)==alignment);for(unsigned i=0;i<count;++i){uint16_t index=uint16_t(i*977+65500);std::memcpy(reinterpret_cast<void*>(pointer+i*2),&index,2);}auto before=source;uint32_t storedOffset;
stride=vertexMode==0?1:vertexMode==3?0:12;uint32_t vertexBytes=vertexMode==4?0:count*12;
std::vector<unsigned char> vertexSource(count*12+64,0xa5),vertexResult(count*12+64,0xa5);for(unsigned i=0;i<count*12;++i)vertexSource[16+i]=static_cast<unsigned char>((i*31+count)%251);
auto beforeVertex=vertexSource;uintptr_t vertexPointer=vertexMode?reinterpret_cast<uintptr_t>(vertexSource.data()+16):0;
vertexDestination=vertexMode==2?0:reinterpret_cast<uintptr_t>(vertexResult.data()+16);

#ifdef _WIN64
StudioMaterialMeshUploadSnapshotCandidate saved{vertexPointer,uint32_t(vertexMode?count:0),vertexBytes,pointer,count};
studioMaterialMeshUploadCandidate(saved,storedOffset,0x10110468,[&](uint32_t vertices,uint32_t indices,auto& descriptor){check(vertices==uint32_t(vertexMode?count:0)&&indices==count);++locks;descriptor.stride=stride;descriptor.vertexOffset=offset;descriptor.positionPresent=true;descriptor.position=vertexDestination;descriptor.indexPresent=true;descriptor.indices=destination;},[](uintptr_t to,uintptr_t from,size_t bytes){++copies;std::memcpy(reinterpret_cast<void*>(to),reinterpret_cast<void*>(from),bytes);},[&](uint32_t vertices,uint32_t indices,const auto&){check(vertices==uint32_t(vertexMode?count:0)&&indices==count);++unlocks;});

#else
mesh[3]=0xa5a5a5a5;upload(mesh,uint32_t(vertexPointer),int(vertexMode?count:0),int(vertexBytes),uint32_t(pointer),int(count));storedOffset=mesh[3];
#endif
check(source==before&&vertexSource==beforeVertex);result.insert(result.end(),vertexResult.begin(),vertexResult.end());std::vector<uint32_t> metadata{copies,locks,unlocks,storedOffset};
#ifdef _WIN64
std::vector<uint32_t> expected(4);check(bool(file.read(reinterpret_cast<char*>(expected.data()),16)));check(expected==metadata);std::vector<unsigned char> bytes(result.size());check(bool(file.read(reinterpret_cast<char*>(bytes.data()),bytes.size())));check(bytes==result);
#else
file.write(reinterpret_cast<char*>(metadata.data()),16);file.write(reinterpret_cast<char*>(result.data()),result.size());
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"native_position_upload_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
