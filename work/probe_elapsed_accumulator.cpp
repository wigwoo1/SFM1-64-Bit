#include <windows.h>
#include <array>
#include <string>
#include "engine_tick_accumulator_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
__declspec(naked) void executeAccumulator(void*,uint32_t,uint32_t*){
 __asm {push ebp
 push ebx
 sub esp,16
 mov ebp,esp
 add ebp,8
 movd xmm0,[esp+32]
 call dword ptr [esp+28]
 mov ecx,[esp+36]
 mov [ecx],ebx
 movss [ecx+4],xmm0
 add esp,16
 pop ebx
 pop ebp
 ret}
}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,8>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){int32_t current;std::memcpy(&current,&r[3],4);auto v=engineTickAccumulatorCandidate(r[0],r[4],r[1],r[5]!=0,r[2]!=0,current);if(uint32_t(v.ticks)!=r[6]||v.remainderWord!=r[7])throw std::runtime_error("accumulator mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_tick_accumulator_exact=65536\n";
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll)",std::ios::binary);
 std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),{});
 if(file.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("DOS bounds");
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)>file.size()-sizeof(IMAGE_NT_HEADERS32))throw std::runtime_error("PE bounds");
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||nt->OptionalHeader.ImageBase!=0x10000000||nt->OptionalHeader.SizeOfHeaders>file.size())throw std::runtime_error("PE identity");
 auto allocation=VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!allocation)throw std::runtime_error("private preferred-base mapping unavailable");
 std::unique_ptr<void,void(*)(void*)> owner(allocation,[](void* p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(allocation,file.data(),nt->OptionalHeader.SizeOfHeaders);
 auto sections=IMAGE_FIRST_SECTION(nt);
 if(reinterpret_cast<const uint8_t*>(sections)+size_t(nt->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)>file.data()+file.size())throw std::runtime_error("section table bounds");
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto& section=sections[i];
  if(section.PointerToRawData>file.size()||section.SizeOfRawData>file.size()-section.PointerToRawData||section.VirtualAddress>nt->OptionalHeader.SizeOfImage||section.SizeOfRawData>nt->OptionalHeader.SizeOfImage-section.VirtualAddress)throw std::runtime_error("section bounds");
  std::memcpy(static_cast<uint8_t*>(allocation)+section.VirtualAddress,file.data()+section.PointerToRawData,section.SizeOfRawData);
 }
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 const size_t bytes=0x1018248e-0x1018243a;auto copied=VirtualAlloc(nullptr,bytes+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copied)throw std::runtime_error("stub allocation");
 std::unique_ptr<void,void(*)(void*)> stubOwner(copied,[](void*p){VirtualFree(p,0,MEM_RELEASE);});std::memcpy(copied,reinterpret_cast<const void*>(0x1018243a),bytes);
 const uint8_t tail[]={0xc3};std::memcpy(static_cast<uint8_t*>(copied)+bytes,tail,sizeof(tail));
 if(!VirtualProtect(copied,bytes+sizeof(tail),PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("stub protection");FlushInstructionCache(GetCurrentProcess(),copied,bytes+sizeof(tail));
 const size_t prefixBytes=0x10182430-0x10182418;
 auto combined=VirtualAlloc(nullptr,prefixBytes+bytes+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!combined)throw std::runtime_error("combined stub allocation");
 std::unique_ptr<void,void(*)(void*)> combinedOwner(combined,[](void*p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(combined,reinterpret_cast<void*>(0x10182418),prefixBytes);
 std::memcpy(static_cast<uint8_t*>(combined)+prefixBytes,copied,bytes+1);
 if(!VirtualProtect(combined,prefixBytes+bytes+1,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("combined protection");FlushInstructionCache(GetCurrentProcess(),combined,prefixBytes+bytes+1);
 auto writable=[](uintptr_t address){DWORD old;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&old))throw std::runtime_error("private data protection");};
 for(uintptr_t address:{uintptr_t(0x104083f4),uintptr_t(0x106285cc),uintptr_t(0x104024e0),uintptr_t(0x106460f4),uintptr_t(0x10644ea4)})writable(address);
 std::array<uint32_t,16>convar{};*reinterpret_cast<uintptr_t*>(0x106285cc)=reinterpret_cast<uintptr_t>(convar.data());
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,8>r{};for(unsigned j=0;j<2;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[2]=i&1;seed=seed*1664525u+1013904223u;r[3]=seed;seed=seed*1664525u+1013904223u;r[4]=i<64?bounds[(i+2)%8]:seed;r[5]=(i>>1)&1;
 convar[12]=r[2];*reinterpret_cast<uint32_t*>(0x104083f4)=r[1];*reinterpret_cast<uint32_t*>(0x104024e0)=r[3];uint32_t out[2]{};*reinterpret_cast<uint32_t*>(0x106460f4)=r[0];*reinterpret_cast<uint32_t*>(0x10644ea4)=r[4];executeAccumulator(r[5]?copied:combined,r[0],out);r[6]=out[0];r[7]=out[1];if(*reinterpret_cast<uint32_t*>(0x10644ea4)!=r[4]||convar[12]!=r[2]||*reinterpret_cast<uint32_t*>(0x104083f4)!=r[1]||*reinterpret_cast<uint32_t*>(0x104024e0)!=r[3])throw std::runtime_error("input mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_tick_accumulator=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
