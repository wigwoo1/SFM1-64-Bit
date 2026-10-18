#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine_projection_sphere_candidate.h"
#ifndef _WIN64
#include "engine_projection_sphere_native_bytes.h"
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);float matrix[16],expected[4],actual[4];unsigned count=0;
 while(f.read(reinterpret_cast<char*>(matrix),64)){if(!f.read(reinterpret_cast<char*>(expected),16))throw std::runtime_error("truncated oracle");engineProjectionSphereCandidate(matrix,actual,actual[3]);if(std::memcmp(actual,expected,16)){for(int k=0;k<4;++k){uint32_t a,b;std::memcpy(&a,actual+k,4);std::memcpy(&b,expected+k,4);std::cerr<<k<<":"<<std::hex<<a<<"/"<<b<<" ";}throw std::runtime_error("native mismatch case "+std::to_string(count));}++count;}
 if(f.gcount()!=0||count!=65536)throw std::runtime_error("oracle count");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sphereNativeAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeProjectionSphereNative(code);DWORD old;if(!VirtualProtect(code,sphereNativeAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,sphereNativeAllocationSize);
 using Generate=void(__cdecl*)(const float*,float*,float*);auto generate=reinterpret_cast<Generate>(code);std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 for(unsigned n=0;n<65536;++n){float matrix[16];for(auto& value:matrix){random=random*1664525u+1013904223u;value=float(int(random>>16)-32768)/32768.0f;}for(int k=0;k<3;++k)matrix[k*4+k]+=3;matrix[12]*=.125f;matrix[13]*=.125f;matrix[14]*=.125f;matrix[15]=1;
 float output[4];generate(matrix,output,output+3);f.write(reinterpret_cast<char*>(matrix),64);f.write(reinterpret_cast<char*>(output),16);}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=65536\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
