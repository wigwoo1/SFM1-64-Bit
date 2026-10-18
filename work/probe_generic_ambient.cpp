#include <windows.h>
#include "generic_ambient_state_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 unsigned cases=0;std::array<uint32_t,86> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  GenericAmbientStateCandidate state;std::copy_n(record.begin(),24,state.source.begin());std::copy_n(record.begin()+24,18,state.packed.begin());state.control=static_cast<uint8_t>(record[42]);
  std::array<uint32_t,24> supplied;std::copy_n(record.begin()+43,24,supplied.begin());state.set(supplied);
  if(state.source!=supplied||!std::equal(state.packed.begin(),state.packed.end(),record.begin()+67)||state.control!=record[85])throw std::runtime_error("ambient mismatch");
  auto packed=state.packed;auto control=state.control;if(state.set(supplied)||state.packed!=packed||state.control!=control)throw std::runtime_error("ambient repeat");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_ambient_states_exact="<<cases<<'\n';
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
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 // Original setter only, synthetic state, masked x87 exceptions.
 using Set=void(__thiscall*)(void*,const void*);auto set=reinterpret_cast<Set>(0x10010c40);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;const uint32_t boundaries[]={0,0x80000000u,1,0x007fffffu,0x00800000u,0x7f800000u,0xff800000u,0x7f800001u,0xff800001u,0x7fc12345u,0xffffffffu};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,86> record{};
  for(unsigned j=0;j<67;++j){seed=seed*1664525u+1013904223u;record[j]=seed;}
  record[42]&=255;
  if(i<264)for(unsigned j=0;j<24;++j)record[43+j]=boundaries[(i+j)%11];
  if(i%4==0)std::copy_n(record.begin(),24,record.begin()+43);
  std::array<uint8_t,0x1800> object{};std::memcpy(object.data()+0x11b8,record.data(),96);std::memcpy(object.data()+0x13dc,record.data()+24,72);object[0x11b6]=static_cast<uint8_t>(record[42]);
  set(object.data(),record.data()+43);
  if(std::memcmp(object.data()+0x11b8,record.data()+43,96))throw std::runtime_error("source copy");
  std::memcpy(record.data()+67,object.data()+0x13dc,72);record[85]=object[0x11b6];
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_ambient_states=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
