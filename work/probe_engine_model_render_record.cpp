#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "model_render_record_initialize_candidate.h"
#include "engine_model_render_record_native_bytes.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);unsigned count=0;unsigned char actual[296],expected[296];
 while(f.read(reinterpret_cast<char*>(actual),296)){if(!f.read(reinterpret_cast<char*>(expected),296))throw std::runtime_error("truncated oracle");initializeModelRenderRecordBytesCandidate(actual+16);if(std::memcmp(actual,expected,296))throw std::runtime_error("native constructor mismatch "+std::to_string(count));++count;}
 if(f.gcount()!=0||count!=4096)throw std::runtime_error("oracle count");std::cout<<"record_cases="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(modelRenderRecordNativeBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");std::memcpy(code,modelRenderRecordNativeBytes,sizeof(modelRenderRecordNativeBytes));DWORD old;if(!VirtualProtect(code,sizeof(modelRenderRecordNativeBytes),PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,sizeof(modelRenderRecordNativeBytes));using Constructor=void*(__thiscall*)(void*);auto constructor=reinterpret_cast<Constructor>(code);std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 for(unsigned n=0;n<4096;++n){unsigned char actual[296],before[296];for(auto& byte:actual){random=random*1664525u+1013904223u;byte=n<256?uint8_t(n):uint8_t(random>>24);}std::memcpy(before,actual,296);if(constructor(actual+16)!=actual+16)throw std::runtime_error("native returned different this");if(std::memcmp(before,actual,16)||std::memcmp(before+280,actual+280,16))throw std::runtime_error("native guard changed");f.write(reinterpret_cast<char*>(before),296);f.write(reinterpret_cast<char*>(actual),296);}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"record_cases=4096\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
