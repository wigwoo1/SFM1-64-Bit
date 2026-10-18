#include <windows.h>
#include <array>
#include <string>
#include "clock_drift_correction_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
static bool demoResult;
static bool __fastcall demoOracle(void*,void*){return demoResult;}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,41>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){std::array<uint32_t,16>v;std::copy_n(r.begin(),16,v.begin());auto duration=clockDriftCorrectionCandidate(v,r[16],r[17],r[18],r[19],r[20],r[21]!=0,r[22]!=0,r[23]!=0);if(duration!=r[40]||!std::equal(v.begin(),v.end(),r.begin()+24))throw std::runtime_error("drift correction mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_drift_correction_exact=65536\n";
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
 std::array<uint32_t,16>low{},high{},amount{},enabled{},debug{};
 std::array<uintptr_t,7>vtable{};vtable[6]=reinterpret_cast<uintptr_t>(&demoOracle);uintptr_t object=reinterpret_cast<uintptr_t>(vtable.data());
 for(uintptr_t address:{uintptr_t(0x1062948c),uintptr_t(0x1062964c),uintptr_t(0x106296a4),uintptr_t(0x1062953c),uintptr_t(0x106295f4),uintptr_t(0x104b60c0),uintptr_t(0x1060d4ec),uintptr_t(0x104083f4),uintptr_t(0x10643ed8)}){DWORD prior;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&prior))throw std::runtime_error("private state protection");}
 *reinterpret_cast<uintptr_t*>(0x1062948c)=reinterpret_cast<uintptr_t>(low.data());*reinterpret_cast<uintptr_t*>(0x1062964c)=reinterpret_cast<uintptr_t>(high.data());*reinterpret_cast<uintptr_t*>(0x106296a4)=reinterpret_cast<uintptr_t>(amount.data());*reinterpret_cast<uintptr_t*>(0x1062953c)=reinterpret_cast<uintptr_t>(enabled.data());*reinterpret_cast<uintptr_t*>(0x106295f4)=reinterpret_cast<uintptr_t>(debug.data());*reinterpret_cast<uintptr_t*>(0x104b60c0)=reinterpret_cast<uintptr_t>(&object);*reinterpret_cast<uint32_t*>(0x1060d4ec)=0;
 using Routine=float(__thiscall*)(void*,float);auto routine=reinterpret_cast<Routine>(0x10143c40);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,41>r{};for(unsigned j=0;j<21;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[21]=i&1;r[22]=(i>>1)&1;r[23]=(i>>2)&1;
 low[11]=r[18];high[11]=r[19];amount[11]=r[20];enabled[12]=r[21];demoResult=r[22]!=0;*reinterpret_cast<uint32_t*>(0x104083f4)=r[17];*reinterpret_cast<uint8_t*>(0x10643ed8)=uint8_t(r[23]);
 std::array<uint32_t,18>guarded{};guarded[0]=0x12345678;guarded[17]=0x87654321;std::copy_n(r.begin(),16,guarded.begin()+1);float duration;std::memcpy(&duration,&r[16],4);float result=routine(guarded.data()+1,duration);std::memcpy(&r[40],&result,4);
 if(guarded[0]!=0x12345678||guarded[17]!=0x87654321)throw std::runtime_error("history bounds");std::copy_n(guarded.begin()+1,16,r.begin()+24);output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_drift_correction=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
