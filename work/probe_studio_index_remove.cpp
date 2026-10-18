#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_index_pool_remove_all_candidate.h"
#ifndef _WIN64
#include "studio_index_remove_native_init.h"
#endif
static void check(bool v){if(!v)throw std::runtime_error("remove-all mismatch");}
static void put16(unsigned char* p,uint16_t v){std::memcpy(p,&v,2);}
static uint16_t get16(const unsigned char* p){uint16_t v;std::memcpy(&v,p,2);return v;}
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::memcpy(code,studioRemove48NativeBytes,sizeof(studioRemove48NativeBytes));std::memcpy(code+256,studioRemove24NativeBytes,sizeof(studioRemove24NativeBytes));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Remove=void(__thiscall*)(void*);
#endif
for(unsigned stride:{24u,48u})for(unsigned length=0;length<=256;++length)for(unsigned prior=0;prior<=8;++prior){unsigned total=length+prior;std::vector<unsigned char> bytes(total*stride,0xa5);std::vector<EngineShadowPoolLinkCandidate> links(total);for(unsigned i=0;i<total;++i){uint16_t prev=i<length?(i?uint16_t(i-1):uint16_t(0xffff)):uint16_t(i);uint16_t next=(i+1<length||(i>=length&&i+1<total))?uint16_t(i+1):uint16_t(0xffff);links[i]={prev,next};put16(bytes.data()+i*stride+stride-4,prev);put16(bytes.data()+i*stride+stride-2,next);}
uint16_t head=length?0:0xffff,tail=length?uint16_t(length-1):uint16_t(0xffff),freeHead=prior?uint16_t(length):uint16_t(0xffff),count=uint16_t(length),maximum=total?uint16_t(total-1):0;uint16_t result[5];
#ifdef _WIN64
EngineShadowPoolStateCandidate pool{links.data(),int32_t(total),head,tail,count,maximum};studioIndexPoolRemoveAllCandidate(pool,freeHead);result[0]=pool.head;result[1]=pool.tail;result[2]=freeHead;result[3]=pool.count;result[4]=pool.maximumIndex;for(unsigned i=0;i<total;++i){put16(bytes.data()+i*stride+stride-4,links[i].previous);put16(bytes.data()+i*stride+stride-2,links[i].next);}uint16_t expected[5];std::vector<unsigned char> expectedBytes(bytes.size());check(bool(file.read(reinterpret_cast<char*>(expected),sizeof(expected))));check(bool(file.read(reinterpret_cast<char*>(expectedBytes.data()),expectedBytes.size())));check(std::memcmp(result,expected,sizeof(result))==0&&bytes==expectedBytes);
#else
unsigned char pool[24];std::memset(pool,0xa5,sizeof(pool));uint32_t ptr=uint32_t(bytes.data());std::memcpy(pool,&ptr,4);put16(pool+12,head);put16(pool+14,tail);put16(pool+16,freeHead);put16(pool+18,count);put16(pool+22,maximum);reinterpret_cast<Remove>(code+(stride==24?256:0))(pool);for(unsigned i=0;i<4;++i)result[i]=get16(pool+12+i*2);result[4]=get16(pool+22);file.write(reinterpret_cast<char*>(result),sizeof(result));file.write(reinterpret_cast<char*>(bytes.data()),bytes.size());
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"remove_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
