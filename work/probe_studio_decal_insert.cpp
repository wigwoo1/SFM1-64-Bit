#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "studio_decal_pointer_insert_candidate.h"
#include "studio_decal_pointer_frontier_candidate.h"
#ifndef _WIN64
#include "studio_decal_insert_native_init.h"
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");unsigned cases=0;
#ifdef _WIN64
std::ifstream f(argv[1],std::ios::binary);
#else
std::ofstream f(argv[1],std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,studioDecalInsertAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeStudioDecalInsertNative(code);DWORD old;if(!VirtualProtect(code,studioDecalInsertAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,studioDecalInsertAllocationSize);using Insert=void(__thiscall*)(void*,void*,void*);auto insert=reinterpret_cast<Insert>(code);
#endif
for(unsigned length=1;length<=16;++length)for(unsigned index=0;index<length;++index)for(unsigned target=0;target<=length;++target){if(target==index)continue;uint32_t result[35]={};
#ifdef _WIN64
std::vector<StudioDecalPointerNodeCandidate> nodes(length);for(unsigned i=0;i<length;++i){nodes[i].previous=i?&nodes[i-1]:nullptr;nodes[i].next=i+1<length?&nodes[i+1]:nullptr;}
StudioDecalPointerBlockCandidate block{nullptr,int32_t(length),nodes.data()};StudioDecalPointerPoolCandidate pool;pool.first=&block;pool.frontier={&block,int32_t(length-1)};pool.head=nodes.data();pool.tail=&nodes[length-1];pool.linkedCount=length;
studioDecalPointerInsertCandidate(pool,target==length?nullptr:&nodes[target],&nodes[index],[](auto& state,auto* node){return studioDecalPointerBeyondFrontierCandidate(state,node,[](uintptr_t a,uintptr_t b){return a>b;});});
auto id=[&](auto* node)->uint32_t{return node?uint32_t(node-nodes.data()):0xffffffffu;};result[0]=id(pool.head);result[1]=id(pool.tail);result[2]=pool.linkedCount;for(unsigned i=0;i<length;++i){result[3+2*i]=id(nodes[i].previous);result[4+2*i]=id(nodes[i].next);}
uint32_t expected[35];if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected))||std::memcmp(result,expected,sizeof(result)))throw std::runtime_error("insert mismatch "+std::to_string(cases));
#else
std::vector<uint32_t> raw(2+5*length,0xa5a5a5a5);raw[0]=0;raw[1]=length;auto address=[&](unsigned i){return uint32_t(reinterpret_cast<uintptr_t>(raw.data()+2+5*i));};for(unsigned i=0;i<length;++i){raw[2+5*i+3]=i?address(i-1):0;raw[2+5*i+4]=i+1<length?address(i+1):0;}
uint32_t pool[10]={};pool[0]=uint32_t(reinterpret_cast<uintptr_t>(raw.data()));pool[1]=length;pool[3]=address(0);pool[4]=address(length-1);pool[6]=length;pool[8]=pool[0];pool[9]=length-1;insert(pool,target==length?nullptr:reinterpret_cast<void*>(address(target)),reinterpret_cast<void*>(address(index)));
auto id=[&](uint32_t pointer)->uint32_t{if(!pointer)return 0xffffffffu;uint32_t delta=pointer-address(0);if(delta%20||delta/20>=length)throw std::runtime_error("foreign link");return delta/20;};result[0]=id(pool[3]);result[1]=id(pool[4]);result[2]=pool[6];for(unsigned i=0;i<length;++i){result[3+2*i]=id(raw[2+5*i+3]);result[4+2*i]=id(raw[2+5*i+4]);for(unsigned j=0;j<3;++j)if(raw[2+5*i+j]!=0xa5a5a5a5)throw std::runtime_error("payload changed");}f.write(reinterpret_cast<char*>(result),sizeof(result));
#endif
++cases;}
#ifdef _WIN64
if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
if(cases!=1496)throw std::runtime_error("case count");std::cout<<"insert_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
