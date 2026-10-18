#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine_projection_inverse_candidate.h"
#ifndef _WIN64
#include "engine_projection_inverse_native_init.h"
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);float matrix[16],expected[16],actual[16];uint32_t expectedReturn,mode;unsigned count=0;
 while(f.read(reinterpret_cast<char*>(matrix),64)){if(!f.read(reinterpret_cast<char*>(&mode),4)||!f.read(reinterpret_cast<char*>(&expectedReturn),4)||!f.read(reinterpret_cast<char*>(expected),64))throw std::runtime_error("truncated oracle");
 if(mode)std::memcpy(actual,matrix,64);else std::memset(actual,0xa5,64);
 uint32_t result=engineProjectionInverseCandidate(mode?actual:matrix,actual)?1:0;
 if(result!=expectedReturn||std::memcmp(actual,expected,64))throw std::runtime_error("native mismatch case "+std::to_string(count));++count;}
 if(f.gcount()!=0||count!=131072)throw std::runtime_error("oracle count");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,projectionInverseAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeProjectionInverseNative(code);DWORD old;if(!VirtualProtect(code,projectionInverseAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,projectionInverseAllocationSize);
 using Inverse=bool(__cdecl*)(const float*,float*);auto inverse=reinterpret_cast<Inverse>(code);std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 for(unsigned n=0;n<65536;++n){float matrix[16];for(auto& value:matrix){random=random*1664525u+1013904223u;value=float(int(random>>16)-32768)/32768.0f;}for(int k=0;k<4;++k)matrix[k*4+k]+=3;
 if(n%8==0)std::memset(matrix,0,64);
 if(n%8==1)std::memcpy(matrix+12,matrix+8,16);
 if(n%8==2){std::memset(matrix,0,64);for(int k=0;k<4;++k)matrix[k*4+k]=1;uint32_t bits=0x358637bdu+(n%3)-1;std::memcpy(matrix+15,&bits,4);}
 for(uint32_t mode=0;mode<2;++mode){float output[16];if(mode)std::memcpy(output,matrix,64);else std::memset(output,0xa5,64);uint32_t result=inverse(mode?output:matrix,output)?1:0;f.write(reinterpret_cast<char*>(matrix),64);f.write(reinterpret_cast<char*>(&mode),4);f.write(reinterpret_cast<char*>(&result),4);f.write(reinterpret_cast<char*>(output),64);}}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=131072\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
