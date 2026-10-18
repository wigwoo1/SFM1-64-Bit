#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "engine_projection_planes_candidate.h"
#include "engine_projection_storage_native_bytes.h"
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);uint32_t input[24];EngineProjectionPlanesCandidate expected,actual;unsigned count=0;
 while(f.read(reinterpret_cast<char*>(input),96)){
  if(!f.read(reinterpret_cast<char*>(&expected),320))throw std::runtime_error("truncated oracle");
  engineProjectionPackPlanesCandidate(input,actual);if(std::memcmp(&actual,&expected,320))throw std::runtime_error("native mismatch");++count;
 }
 if(count!=65536)throw std::runtime_error("case count");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,512,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");
 std::memcpy(code,derivedBytes,sizeof(derivedBytes));std::memcpy(code+128,fourBytes,sizeof(fourBytes));std::memcpy(code+256,twoBytes,sizeof(twoBytes));
 std::memset(code+400,0,16);uint32_t mask[4]={0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff};std::memcpy(code+416,mask,16);
 auto absolute=[&](int offset,int target){uint32_t address=reinterpret_cast<uint32_t>(code+target);std::memcpy(code+offset,&address,4);};
 for(int offset:{3,24,46})absolute(offset,400);for(int offset:{68,82,99})absolute(offset,416);
 absolute(256+26,400);absolute(256+37,400);
 for(int offset:{128+111,256+117}){int32_t displacement=-(offset+4);std::memcpy(code+offset,&displacement,4);}
 DWORD old;if(!VirtualProtect(code,512,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,512);
 using Pack=void(__thiscall*)(void*,const uint32_t*);auto four=reinterpret_cast<Pack>(code+128),two=reinterpret_cast<Pack>(code+256);
 std::ofstream f(argv[1],std::ios::binary);uint32_t input[24],random=123456789;EngineProjectionPlanesCandidate output;
 for(unsigned n=0;n<65536;++n){for(auto& value:input){random=random*1664525u+1013904223u;value=random;}four(&output.first,input);two(&output.last,input+16);f.write(reinterpret_cast<char*>(input),96);f.write(reinterpret_cast<char*>(&output),320);}
 f.flush();if(!f)throw std::runtime_error("oracle write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=65536\n";
#endif
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
