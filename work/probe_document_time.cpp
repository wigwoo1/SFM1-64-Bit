#include <windows.h>
#include <array>
#include <string>
#include "anchored_document_time_candidate.h"
#include "monotonic_engine_time_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
static bool gateValue;static float timeValue;
static bool __fastcall gateOracle(void*,void*){return gateValue;}
static float __fastcall timeOracle(void*,void*){return timeValue;}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,14>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){AnchoredDocumentTimeState state(r[0]);state.saved684=int32_t(r[1]);state.mode47c=int32_t(r[2]);for(unsigned j=0;j<4;++j)state.modes480[j]=int32_t(r[3+j]);state.flag69c=(r[7]&1)!=0;state.flag228=(r[7]&2)!=0;state.flag2a8=(r[7]&4)!=0;state.flag6c0=false;
 uint32_t retained=r[12];const auto converted=monotonicEngineTicksCandidate(r[11],retained);if(uint32_t(converted)!=r[9]||retained!=r[13])throw std::runtime_error("engine converter mismatch case "+std::to_string(cases));
 auto actual=anchoredDocumentTimeCandidate(state,[&]{return r[8]!=0;},[&]{return converted;},[]()->std::optional<float>{throw std::runtime_error("unexpected rate lookup");});if(uint32_t(actual)!=r[10])throw std::runtime_error("document time mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_document_time_exact=65536\n";
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll)",std::ios::binary);
 std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),{});
 if(file.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("DOS bounds");
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)>file.size()-sizeof(IMAGE_NT_HEADERS32))throw std::runtime_error("PE bounds");
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||nt->OptionalHeader.ImageBase!=0x10000000||nt->OptionalHeader.SizeOfHeaders>file.size())throw std::runtime_error("PE identity");
 auto allocation=VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!allocation)throw std::runtime_error("private preferred-base mapping unavailable");
 std::unique_ptr<void,void(*)(void*)> imageOwner(allocation,[](void* p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(allocation,file.data(),nt->OptionalHeader.SizeOfHeaders);
 auto sections=IMAGE_FIRST_SECTION(nt);
 if(reinterpret_cast<const uint8_t*>(sections)+size_t(nt->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)>file.data()+file.size())throw std::runtime_error("section table bounds");
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto& section=sections[i];
  if(section.PointerToRawData>file.size()||section.SizeOfRawData>file.size()-section.PointerToRawData||section.VirtualAddress>nt->OptionalHeader.SizeOfImage||section.SizeOfRawData>nt->OptionalHeader.SizeOfImage-section.VirtualAddress)throw std::runtime_error("section bounds");
  std::memcpy(static_cast<uint8_t*>(allocation)+section.VirtualAddress,file.data()+section.PointerToRawData,section.SizeOfRawData);
 }
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 std::array<uintptr_t,48>table{};table[29]=reinterpret_cast<uintptr_t>(&gateOracle);table[47]=reinterpret_cast<uintptr_t>(&timeOracle);uintptr_t object=reinterpret_cast<uintptr_t>(table.data());
 for(uintptr_t address:{uintptr_t(0x10fd4398),uintptr_t(0x10f321d0)}){DWORD prior;if(!VirtualProtect(reinterpret_cast<void*>(address),4,PAGE_READWRITE,&prior))throw std::runtime_error("private clock state protection");}
 *reinterpret_cast<uintptr_t*>(0x10fd4398)=reinterpret_cast<uintptr_t>(&object);
 using Getter=int32_t*(__thiscall*)(void*,int32_t*);auto getter=reinterpret_cast<Getter>(0x1029a950);
 using Clock=int32_t*(__cdecl*)(int32_t*);auto clock=reinterpret_cast<Clock>(0x10275640);
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x29a950;const uint32_t floatBounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,14>r{};for(auto&word:r){seed=seed*1664525u+1013904223u;word=seed;}if(i%3==0)r[0]=0x80000000u;r[2]=(i>>1)%3==0?0:r[2];for(unsigned j=0;j<4;++j)r[3+j]=((i>>(j+2))&1)?2:1;r[7]=i%8;r[8]=i&1;if(i<64){r[11]=floatBounds[i%8];r[12]=floatBounds[(i/8)%8];}
 std::array<uint32_t,0x700/4>owner{};owner[0x430/4]=r[0];owner[0x684/4]=r[1];owner[0x47c/4]=r[2];for(unsigned j=0;j<4;++j)owner[0x480/4+j]=r[3+j];auto bytes=reinterpret_cast<uint8_t*>(owner.data());bytes[0x69c]=uint8_t(r[7]&1);bytes[0x228]=uint8_t(r[7]&2);bytes[0x2a8]=uint8_t(r[7]&4);
 gateValue=r[8]!=0;std::memcpy(&timeValue,&r[11],4);*reinterpret_cast<uint32_t*>(0x10f321d0)=r[12];const auto original=owner;
 int32_t result=0;auto returned=getter(owner.data(),&result);if(returned!=&result||owner!=original)throw std::runtime_error("getter ownership");std::memcpy(&r[10],&result,4);
 // Capture the actual original time conversion as an explicit callback result.
 *reinterpret_cast<uint32_t*>(0x10f321d0)=r[12];int32_t converted=0;clock(&converted);std::memcpy(&r[9],&converted,4);r[13]=*reinterpret_cast<uint32_t*>(0x10f321d0);
 output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_document_time=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
