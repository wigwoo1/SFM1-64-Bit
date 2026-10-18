#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include "model_render_record_insert_candidate.h"
#ifndef _WIN64
#include "engine_model_render_insert_native_init.h"
struct RawPool{uint32_t storage;int32_t capacity;uint32_t growth;uint16_t head,tail,free,count,fresh,maximum;uint32_t cache;};
static void write16(unsigned char*p,uint16_t v){std::memcpy(p,&v,2);}static uint16_t read16(const unsigned char*p){uint16_t v;std::memcpy(&v,p,2);return v;}
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");unsigned cases=0;
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);
#else
 std::ofstream f(argv[1],std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,modelRenderInsertAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeModelRenderInsertNative(code);DWORD old;if(!VirtualProtect(code,modelRenderInsertAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,modelRenderInsertAllocationSize);using Insert=void(__thiscall*)(void*,uint32_t,uint32_t);auto insert=reinterpret_cast<Insert>(code);
#endif
 for(unsigned length=1;length<=32;++length)for(unsigned index=0;index<length;++index)for(unsigned target=0;target<=length;++target){if(target==index)continue;uint16_t before=target==length?uint16_t(0xffff):uint16_t(target);uint32_t result[67]={};
#ifdef _WIN64
  std::vector<EngineShadowPoolLinkCandidate> links(length);for(unsigned i=0;i<length;++i)links[i]={i?uint16_t(i-1):uint16_t(0xffff),i+1<length?uint16_t(i+1):uint16_t(0xffff)};EngineShadowPoolStateCandidate pool{links.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};modelRenderRecordInsertCandidate(pool,0xabcd0000u|before,0x12340000u|index);result[0]=pool.head;result[1]=pool.tail;result[2]=pool.count;for(unsigned i=0;i<length;++i){result[3+2*i]=links[i].previous;result[4+2*i]=links[i].next;}
  uint32_t expected[67];if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected))||std::memcmp(result,expected,sizeof(result)))throw std::runtime_error("native insert mismatch "+std::to_string(cases));
#else
  std::vector<std::array<unsigned char,264>> records(length);for(unsigned i=0;i<length;++i){records[i].fill(0xa5);write16(records[i].data()+260,i?uint16_t(i-1):uint16_t(0xffff));write16(records[i].data()+262,i+1<length?uint16_t(i+1):uint16_t(0xffff));}RawPool pool{uint32_t(reinterpret_cast<uintptr_t>(records.data())),int32_t(length),0,0,uint16_t(length-1),0xffff,uint16_t(length),uint16_t(length),uint16_t(length-1),0};insert(&pool,0xabcd0000u|before,0x12340000u|index);result[0]=pool.head;result[1]=pool.tail;result[2]=pool.count;for(unsigned i=0;i<length;++i){result[3+2*i]=read16(records[i].data()+260);result[4+2*i]=read16(records[i].data()+262);for(unsigned j=0;j<260;++j)if(records[i][j]!=0xa5)throw std::runtime_error("native payload changed");}f.write(reinterpret_cast<char*>(result),sizeof(result));
#endif
 ++cases;}
#ifdef _WIN64
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
 if(cases!=11440)throw std::runtime_error("case count");std::cout<<"insert_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
