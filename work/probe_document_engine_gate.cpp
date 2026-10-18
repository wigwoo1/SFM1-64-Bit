#define NOMINMAX
#include <windows.h>
#include <array>
#include <fstream>
#include <iostream>
#include "document_engine_gate_candidate.h"
#include "document_engine_gate_bytes.h"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 std::array<uint32_t,3> record{};unsigned checked=0;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t state;std::memcpy(&state,&record[1],4);
  if(record[0]>255||record[2]>1||documentEngineGateCandidate(uint8_t(record[0]),state)!=(record[2]!=0))throw std::runtime_error("native gate mismatch");++checked;
 }
 if(input.gcount()!=0||checked!=67328)throw std::runtime_error("oracle length");
 std::cout<<"native_engine_gate_matches="<<checked<<"\n";
#else
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,128,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(!memory)throw std::runtime_error("allocation");
 std::memcpy(memory,nativeGate,sizeof(nativeGate));std::memcpy(memory+32,nativeAccessor,sizeof(nativeAccessor));
 std::memcpy(memory+80,nativeSetter,sizeof(nativeSetter));
 int32_t relative=32-11;std::memcpy(memory+7,&relative,4);
 uint32_t pointerAddress=reinterpret_cast<uint32_t>(memory+64);std::memcpy(memory+33,&pointerAddress,4);
 std::array<unsigned char,128> client{};uint32_t wrapper=reinterpret_cast<uint32_t>(client.data());std::memcpy(memory+64,&wrapper,4);
 DWORD old;if(!VirtualProtect(memory,128,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),memory,128);
 using Native=uint32_t(__thiscall*)(void*);auto call=reinterpret_cast<Native>(memory);
 using Setter=void(__thiscall*)(void*,uint32_t);auto setFlag=reinterpret_cast<Setter>(memory+80);
 std::array<unsigned char,8> tool{};std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle write open");
 auto sample=[&](uint32_t flag,uint32_t state){auto expectedTool=tool;expectedTool[4]=uint8_t(flag);setFlag(tool.data(),flag|0xabcdef00u);if(tool!=expectedTool)throw std::runtime_error("native setter ownership/low byte");std::memcpy(client.data()+0x68,&state,4);auto before=client;auto toolBefore=tool;
  uint32_t result=call(tool.data());if(client!=before||tool!=toolBefore||result>1)throw std::runtime_error("native ownership/result");
  std::array<uint32_t,3> record{flag,state,result};output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));};
 for(uint32_t flag=0;flag<256;++flag)for(uint32_t state:{0x80000000u,0xffffffffu,0u,1u,2u,3u,0x7fffffffu})sample(flag,state);
 uint32_t random=0x1938abcd;for(unsigned i=0;i<65536;++i){random=random*1664525u+1013904223u;sample(i&255,random);}
 output.flush();if(!output)throw std::runtime_error("oracle write");VirtualFree(memory,0,MEM_RELEASE);
 std::cout<<"native_engine_gate_cases=67328\n";
#endif
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
