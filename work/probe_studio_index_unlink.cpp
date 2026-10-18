#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_index_unlink_candidate.h"
#ifndef _WIN64
#include "studio_decal_index_unlink_native_init.h"
#endif
static void check(bool v){if(!v)throw std::runtime_error("unlink parity");}
static void put16(unsigned char* p,uint16_t v){std::memcpy(p,&v,2);}
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::memcpy(code,studioDecalIndexUnlinkNativeBytes,sizeof(studioDecalIndexUnlinkNativeBytes));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Unlink=void(__thiscall*)(void*,uint32_t);auto unlink=reinterpret_cast<Unlink>(code);
#endif
for(unsigned length=1;length<=32;++length)for(unsigned guard=0;guard<4;++guard)for(unsigned self=0;self<2;++self)for(unsigned alias=0;alias<2;++alias)for(unsigned selected=0;selected<=length+1;++selected){
uint32_t argument=(selected==length+1?0xffff:selected)|(alias?0xabcd0000:0);int32_t capacity=guard==1?-1:guard==2?int32_t(length-1):int32_t(length);uint16_t maximum=guard==3?0:uint16_t(length-1);std::vector<unsigned char> bytes(length*84,0xa5);std::vector<EngineShadowPoolLinkCandidate> links(length);
for(unsigned i=0;i<length;++i){uint16_t previous=self&&i==selected?uint16_t(i):i?uint16_t(i-1):uint16_t(0xffff);uint16_t next=i+1<length?uint16_t(i+1):uint16_t(0xffff);links[i]={previous,next};put16(bytes.data()+i*84+80,previous);put16(bytes.data()+i*84+82,next);}
unsigned char metadata[24];std::memset(metadata,0xa5,sizeof(metadata));std::memcpy(metadata+4,&capacity,4);put16(metadata+12,0);put16(metadata+14,uint16_t(length-1));put16(metadata+18,uint16_t(length));put16(metadata+22,maximum);
#ifdef _WIN64
EngineShadowPoolStateCandidate pool{links.data(),capacity,0,uint16_t(length-1),uint16_t(length),maximum};studioDecalIndexUnlinkCandidate(pool,argument);put16(metadata+12,pool.head);put16(metadata+14,pool.tail);put16(metadata+18,pool.count);for(unsigned i=0;i<length;++i){put16(bytes.data()+i*84+80,links[i].previous);put16(bytes.data()+i*84+82,links[i].next);}unsigned char expected[24];std::vector<unsigned char> expectedBytes(bytes.size());check(bool(file.read(reinterpret_cast<char*>(expected),24)));check(bool(file.read(reinterpret_cast<char*>(expectedBytes.data()),expectedBytes.size())));check(std::memcmp(metadata,expected,24)==0&&bytes==expectedBytes);
#else
uint32_t address=uint32_t(bytes.data());std::memcpy(metadata,&address,4);unlink(metadata,argument);std::memset(metadata,0xa5,4);file.write(reinterpret_cast<char*>(metadata),24);file.write(reinterpret_cast<char*>(bytes.data()),bytes.size());
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"unlink_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
