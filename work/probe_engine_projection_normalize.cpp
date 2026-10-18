#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine_projection_normalize_candidate.h"
#include "engine_projection_normalize_native_bytes.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);uint32_t record[7];unsigned count=0;
 while(f.read(reinterpret_cast<char*>(record),28)){
  float vector[3];std::memcpy(vector,record,12);float length=engineProjectionNormalizeCandidate(vector);uint32_t output[4];std::memcpy(output,vector,12);std::memcpy(output+3,&length,4);
  if(std::memcmp(output,record+3,16))throw std::runtime_error("native mismatch case "+std::to_string(count));++count;
 }if(f.gcount()!=0||count!=65536)throw std::runtime_error("oracle size");std::cout<<"native_matches="<<count<<'\n';
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,256,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");std::memcpy(code,normalizeBytes,sizeof(normalizeBytes));
 uint32_t constants[2]={0x3f800000,0x34000000};std::memcpy(code+192,constants,8);
 for(unsigned i=0;i<2;++i){uint32_t address=reinterpret_cast<uint32_t>(code+192+i*4);std::memcpy(code+constantOffsets[i],&address,4);}
 DWORD old;if(!VirtualProtect(code,256,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,256);
 using Normalize=float(__cdecl*)(float*);auto normalize=reinterpret_cast<Normalize>(code);
 std::ofstream f(argv[1],std::ios::binary);uint32_t random=123456789;
 const uint32_t special[]={0,0x80000000,0x34000000,0xb4000000,1,0x80000001,0x3f800000,0xbf800000};
 for(unsigned n=0;n<65536;++n){uint32_t record[7];for(unsigned i=0;i<3;++i){random=random*1664525u+1013904223u;record[i]=n<8?special[(n+i)%8]:((random&0x807fffff)|(((random>>24)%180+30)<<23));}
  float vector[3];std::memcpy(vector,record,12);float length=normalize(vector);std::memcpy(record+3,vector,12);std::memcpy(record+6,&length,4);f.write(reinterpret_cast<char*>(record),28);
 }f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);std::cout<<"native_cases=65536\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

