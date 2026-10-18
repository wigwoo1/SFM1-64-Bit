#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine_projection_box_planes_candidate.h"
#include "engine_projection_box_planes_native_bytes.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);EngineProjectionPlanesCandidate planes;float bounds[8];uint32_t expected;unsigned count=0;
 while(f.read(reinterpret_cast<char*>(&planes),320)){if(!f.read(reinterpret_cast<char*>(bounds),32)||!f.read(reinterpret_cast<char*>(&expected),4))throw std::runtime_error("truncated oracle");if(uint32_t(engineProjectionBoxOutsidePlanesCandidate(planes,bounds,bounds+4))!=expected)throw std::runtime_error("native mismatch "+std::to_string(count));++count;}
 if(f.gcount()!=0||count!=65536)throw std::runtime_error("oracle count");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(boxPlanesNativeBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");std::memcpy(code,boxPlanesNativeBytes,sizeof(boxPlanesNativeBytes));DWORD old;if(!VirtualProtect(code,sizeof(boxPlanesNativeBytes),PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,sizeof(boxPlanesNativeBytes));
 using Test=bool(__thiscall*)(const void*,const float*,const float*);auto test=reinterpret_cast<Test>(code);std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 for(unsigned n=0;n<65536;++n){uint32_t bits[24];float bounds[8];auto next=[&]{random=random*1664525u+1013904223u;return random;};for(auto& b:bits)b=next();for(auto& b:bounds){uint32_t v=next();std::memcpy(&b,&v,4);}EngineProjectionPlanesCandidate planes;engineProjectionPackPlanesCandidate(bits,planes);uint32_t result=test(&planes,bounds,bounds+4)?1:0;f.write(reinterpret_cast<char*>(&planes),320);f.write(reinterpret_cast<char*>(bounds),32);f.write(reinterpret_cast<char*>(&result),4);}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=65536\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
