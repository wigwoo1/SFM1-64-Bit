#include <windows.h>
#include "generic_light_array_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
static unsigned flushes;static void __fastcall fakeFlush(void*,void*) {++flushes;}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 unsigned cases=0;std::array<uint32_t,270> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  GenericLightArrayCandidate state;std::copy_n(record.begin()+3,88,state.words.begin());state.control=static_cast<uint8_t>(record[2]);
  std::array<uint32_t,88> supplied;std::copy_n(record.begin()+91,88,supplied.begin());int32_t requested,configured;std::memcpy(&requested,&record[0],4);std::memcpy(&configured,&record[1],4);unsigned flushCount=0;
  state.set(requested,configured,supplied,[&]{++flushCount;});uint32_t countBits;std::memcpy(&countBits,&state.count,4);
  if(!std::equal(state.words.begin(),state.words.end(),record.begin()+179)||countBits!=record[267]||state.control!=record[268]||flushCount!=record[269])throw std::runtime_error("light array mismatch");
  unsigned expected=state.count!=0?2:1;state.clear([&]{++flushCount;});if(state.count||flushCount!=expected)throw std::runtime_error("owned clear");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_light_arrays_exact="<<cases<<'\n';
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
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
 std::array<uint8_t,0x548> config{};*reinterpret_cast<void**>(static_cast<uint8_t*>(allocation)+0x16d130)=config.data();
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 // Synthetic flush callback/config, original memcpy and setter code only.
 using Set=void(__thiscall*)(void*,int32_t,const void*);auto set=reinterpret_cast<Set>(0x10010bb0);
 using Clear=void(__thiscall*)(void*);auto clear=reinterpret_cast<Clear>(0x10010c10);
 std::array<void*,85> table{};table[84]=reinterpret_cast<void*>(&fakeFlush);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x194782ab;
 const int32_t boundaries[]={INT32_MIN,-1,0,1,3,4,5,INT32_MAX};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,270> record{};for(unsigned j=0;j<179;++j){seed=seed*1664525u+1013904223u;record[j]=seed;}record[2]&=255;
  if(i<16384){std::memcpy(&record[0],&boundaries[i%8],4);std::memcpy(&record[1],&boundaries[(i/8)%8],4);record[2]=(i/64)&255;}
  std::memcpy(config.data()+0x544,&record[1],4);std::array<uint8_t,0x1800> object{};auto tp=table.data();std::memcpy(object.data(),&tp,4);object[0x11b6]=static_cast<uint8_t>(record[2]);std::memcpy(object.data()+0x1434,record.data()+3,352);
  int32_t requested;std::memcpy(&requested,&record[0],4);flushes=0;set(object.data(),requested,record.data()+91);
  std::memcpy(record.data()+179,object.data()+0x1434,352);std::memcpy(&record[267],object.data()+0x1430,4);record[268]=object[0x11b6];record[269]=flushes;
  clear(object.data());uint32_t cleared;std::memcpy(&cleared,object.data()+0x1430,4);if(cleared||flushes!=(record[267]?2u:1u))throw std::runtime_error("native clear");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_arrays=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
