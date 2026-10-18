#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_association_release_candidate.h"
#include "studio_decal_association_frontier_candidate.h"
static uint32_t calls;
static void check(bool v){if(!v)throw std::runtime_error("association parity");}
#ifndef _WIN64
#include "studio_association_native_init.h"
static void* expectedPool;
using Frontier=bool(__thiscall*)(void*,void*,void*);static Frontier nativeFrontier;
static bool __fastcall classify(void* pool,void*,void* node,void* frontier){check(pool==expectedPool&&frontier==static_cast<unsigned char*>(pool)+32);++calls;return nativeFrontier(pool,node,frontier);}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);initializeStudioAssociationNative(code,uint32_t(&classify));std::memcpy(code+256,studioAssociationFrontierNativeBytes,sizeof(studioAssociationFrontierNativeBytes));nativeFrontier=reinterpret_cast<Frontier>(code+256);DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Release=void(__thiscall*)(void*,uint32_t);auto release=reinterpret_cast<Release>(code);std::vector<uint32_t> owner((0x7c0914+64)/4);
#endif
for(unsigned length=1;length<=128;++length){
#ifdef _WIN64
std::vector<StudioDecalAssociationNodeCandidate> nodes(length);StudioDecalAssociationPoolCandidate pool{nodes.data(),&nodes.back(),nullptr,length};for(unsigned i=0;i<length;++i){nodes[i].resource=0x1000+i%4;nodes[i].payload=0x2000+i;nodes[i].previous=i?&nodes[i-1]:nullptr;nodes[i].next=i+1<length?&nodes[i+1]:nullptr;}auto normalize=[&](auto* p){return p?uint32_t(p-nodes.data()):0xffffffffu;};
#else
std::vector<uint32_t> raw(length*4+2);raw[0]=0;raw[1]=length;auto* nodes=raw.data()+2;auto pointer=[&](unsigned i){return uint32_t(nodes+i*4);};auto* pool=owner.data()+0x7c0914/4;std::memset(pool,0,64);pool[3]=pointer(0);pool[4]=pointer(length-1);pool[6]=length;expectedPool=pool;pool[8]=uint32_t(raw.data());pool[9]=length-1;for(unsigned i=0;i<length;++i){nodes[i*4]=0x1000+i%4;nodes[i*4+1]=0x2000+i;nodes[i*4+2]=i?pointer(i-1):0;nodes[i*4+3]=i+1<length?pointer(i+1):0;}auto normalize=[&](uint32_t p){return p?(p-uint32_t(nodes))/16:0xffffffffu;};
#endif
for(unsigned key=0;key<5;++key){calls=0;std::vector<uint32_t> result;
#ifdef _WIN64
StudioDecalAssociationBlockCandidate block{nullptr,int32_t(length),nodes.data()};studioDecalAssociationReleaseCandidate(pool,0x1000+key,[&](auto&,auto* node){++calls;return studioDecalAssociationBeyondFrontierCandidate(&block,int32_t(length-1),node,[](uintptr_t a,uintptr_t b){return a>b;});});result={normalize(pool.head),normalize(pool.tail),normalize(pool.freeHead),pool.count,calls};for(auto& n:nodes)result.insert(result.end(),{uint32_t(n.resource),uint32_t(n.payload),normalize(n.previous),normalize(n.next)});std::vector<uint32_t> expected(result.size());check(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size()*4)));check(expected==result);
#else
release(owner.data(),0x1000+key);result={normalize(pool[3]),normalize(pool[4]),normalize(pool[5]),pool[6],calls};for(unsigned i=0;i<length;++i)result.insert(result.end(),{nodes[i*4],nodes[i*4+1],normalize(nodes[i*4+2]),normalize(nodes[i*4+3])});file.write(reinterpret_cast<char*>(result.data()),result.size()*4);
#endif
++cases;}}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"association_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
