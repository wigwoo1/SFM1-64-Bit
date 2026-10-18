#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "engine_receiver_insert_candidate.h"
#include "engine_receiver_link_native_bytes.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream oracle(argv[1],std::ios::binary);
#else
 std::ofstream oracle(argv[1],std::ios::binary);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,512,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");std::memcpy(code,unlinkReceiverBytes,sizeof(unlinkReceiverBytes));std::memcpy(code+192,insertReceiverBytes,sizeof(insertReceiverBytes));int32_t displacement=-(192+13+4);std::memcpy(code+192+13,&displacement,4);DWORD old;if(!VirtualProtect(code,512,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,512);
 using Insert=void(__thiscall*)(void*,uint32_t,uint32_t);using Unlink=void(__thiscall*)(void*,int32_t);auto insert=reinterpret_cast<Insert>(code+192);auto unlink=reinterpret_cast<Unlink>(code);
#endif
 unsigned cases=0;
 for(unsigned length=0;length<=64;++length)for(unsigned position=0;position<=length;++position)for(unsigned mode=0;mode<(length?1u:2u);++mode){
  std::vector<EngineReceiverLinkCandidate> links(length+1);for(unsigned i=0;i<length;++i)links[i]={uint16_t(i?i-1:0xffff),uint16_t(i+1<length?i+1:0xffff)};links[length]={uint16_t(mode?0xffff:length),uint16_t(mode?0xffff:length)};
  EngineReceiverPoolCandidate pool{links.data(),int32_t(length+1),int32_t(length),length?0u:0xffffu,length?length-1:0xffffu,length};
#ifndef _WIN64
  std::vector<unsigned char> raw((length+1)*20,0xa5);for(unsigned i=0;i<=length;++i){std::memcpy(raw.data()+i*20+16,&links[i].previous,2);std::memcpy(raw.data()+i*20+18,&links[i].next,2);}
  uint32_t owner[10]={reinterpret_cast<uint32_t>(raw.data()),length+1,0,pool.head,pool.tail,0xffff,pool.count,0,length,0};
#endif
  for(unsigned phase=0;phase<3;++phase){
   uint32_t target=phase==0&&position<length?position:0xffff;
#ifdef _WIN64
   if(phase<2)engineReceiverInsertCandidate(pool,target,length);else engineReceiverUnlinkCandidate(pool,int32_t(length));
#else
   if(phase<2)insert(owner,target,length);else unlink(owner,int32_t(length));
#endif
   uint32_t state[133]{};
#ifdef _WIN64
   state[0]=pool.head;state[1]=pool.tail;state[2]=pool.count;for(unsigned i=0;i<=length;++i){state[3+i*2]=links[i].previous;state[4+i*2]=links[i].next;}
   uint32_t expected[133];if(!oracle.read(reinterpret_cast<char*>(expected),sizeof(expected)))throw std::runtime_error("short oracle");if(std::memcmp(state,expected,sizeof(state)))throw std::runtime_error("native receiver-link mismatch");
#else
   state[0]=owner[3];state[1]=owner[4];state[2]=owner[6];for(unsigned i=0;i<=length;++i){for(unsigned byte=0;byte<16;++byte)if(raw[i*20+byte]!=0xa5)throw std::runtime_error("native receiver payload changed");uint16_t p,n;std::memcpy(&p,raw.data()+i*20+16,2);std::memcpy(&n,raw.data()+i*20+18,2);state[3+i*2]=p;state[4+i*2]=n;}oracle.write(reinterpret_cast<char*>(state),sizeof(state));
#endif
   ++cases;
  }
 }
#ifdef _WIN64
 if(oracle.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
 oracle.flush();if(!oracle)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"receiver_link_cases="<<cases<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
