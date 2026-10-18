#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include "document_datamodel_scope_candidate.h"
#include "document_propagation_flag_bytes.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);std::array<uint32_t,4>r{};unsigned count=0;
 while(f.read(reinterpret_cast<char*>(r.data()),sizeof(r))){uint8_t flags=uint8_t(r[0]);setDocumentChangePropagationSuppressedCandidate(flags,r[1]|0xabcdef00u);if(flags!=r[2]||documentChangePropagationSuppressedCandidate(flags)!=(r[3]!=0))throw std::runtime_error("native mismatch");++count;}
 if(f.gcount()!=0||count!=65536)throw std::runtime_error("oracle size");std::cout<<"native_propagation_flag_matches="<<count<<'\n';
#else
 auto mem=static_cast<unsigned char*>(VirtualAlloc(nullptr,128,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!mem)throw std::runtime_error("allocate");
 std::memcpy(mem,setter,sizeof(setter));std::memcpy(mem+48,getter,sizeof(getter));
 DWORD old;if(!VirtualProtect(mem,128,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),mem,128);
 using Set=void(__thiscall*)(void*,uint32_t);using Get=uint8_t(__thiscall*)(void*);auto set=reinterpret_cast<Set>(mem);auto get=reinterpret_cast<Get>(mem+48);
 std::vector<unsigned char>owner(0xc190,0xa5);std::ofstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("output");
 for(uint32_t initial=0;initial<256;++initial)for(uint32_t arg=0;arg<256;++arg){owner[0xc18c]=uint8_t(initial);auto before=owner;set(owner.data()+0xc118,arg|0xabcdef00u);const auto result=owner[0xc18c];before[0xc18c]=result;if(before!=owner)throw std::runtime_error("setter changed other bytes");const auto bit=get(owner.data()+0xc118);if(before!=owner)throw std::runtime_error("getter mutation");std::array<uint32_t,4>r{initial,arg,result,bit};f.write(reinterpret_cast<char*>(r.data()),sizeof(r));}
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(mem,0,MEM_RELEASE);std::cout<<"native_propagation_flag_cases=65536\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
