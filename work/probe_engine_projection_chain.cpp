#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine_projection_generate_candidate.h"
#ifndef _WIN64
#include "engine_projection_chain_native_init.h"
#endif
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);float matrix[16];EngineProjectionPlanesCandidate expected,actual;unsigned count=0;
 while(f.read(reinterpret_cast<char*>(matrix),64)){if(!f.read(reinterpret_cast<char*>(&expected),320))throw std::runtime_error("truncated oracle");engineProjectionGeneratePlanesCandidate(matrix,actual);if(std::memcmp(&actual,&expected,320))throw std::runtime_error("native mismatch case "+std::to_string(count));++count;}
 if(f.gcount()!=0||count!=65536)throw std::runtime_error("oracle count");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,projectionChainAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeProjectionChainNative(code);DWORD old;if(!VirtualProtect(code,projectionChainAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,projectionChainAllocationSize);
 using Generate=void(__cdecl*)(const float*,void*);auto generate=reinterpret_cast<Generate>(code);std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 for(unsigned n=0;n<65536;++n){float matrix[16];for(auto& value:matrix){random=random*1664525u+1013904223u;value=float(int(random>>16)-32768)/32768.0f;}for(int k=0;k<3;++k)matrix[k*4+k]+=3;matrix[12]*=0.125f;matrix[13]*=0.125f;matrix[14]*=0.125f;matrix[15]=1;
 EngineProjectionPlanesCandidate output;generate(matrix,&output);f.write(reinterpret_cast<char*>(matrix),64);f.write(reinterpret_cast<char*>(&output),320);}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=65536\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
