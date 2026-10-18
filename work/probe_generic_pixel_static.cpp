#include <windows.h>
#include "generic_shader_combo_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,50> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  std::array<uint32_t,24> fields;std::copy_n(record.begin(),24,fields.begin());
  if(genericPs20bStaticComboCandidate(fields)!=record[24])throw std::runtime_error("pixel static mismatch");
  auto initial=genericPs20bStaticInitialCandidate(record[25]);
  if(!std::equal(initial.begin(),initial.end(),record.begin()+26))throw std::runtime_error("pixel initializer mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_pixel_static_combos_exact="<<cases<<'\n';
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll)",std::ios::binary);
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
 // Original audited pure arithmetic packer; no external calls or init.
 using Pack=uint32_t(__thiscall*)(const uint32_t*);auto pack=reinterpret_cast<Pack>(0x10053e10);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x194782ab;
 using Init=void*(__thiscall*)(uint32_t*,void*,void*);auto init=reinterpret_cast<Init>(0x10053da0);
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,50> record{};
  for(unsigned j=0;j<24;++j){seed=seed*1664525u+1013904223u;record[j]=i<48?(i/24?UINT32_MAX:1u)*(i%24==j):i<8192?(seed>>23)&1u:seed;}
  auto before=record;record[24]=pack(record.data());if(!std::equal(record.begin(),record.begin()+24,before.begin()))throw std::runtime_error("native input mutation");
  seed=seed*1664525u+1013904223u;record[25]=seed;uint32_t material[3]={0,0,seed};void* params[]={material};
  std::fill(record.begin()+26,record.end(),0xdeadbeefu);
  if(init(record.data()+26,nullptr,params)!=record.data()+26)throw std::runtime_error("pixel initializer return");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_pixel_static_combos=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
