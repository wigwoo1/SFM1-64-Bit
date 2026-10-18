#include <windows.h>
#include "angle_basis_products_candidate.h"
#ifdef _WIN64
#include "angle_basis_candidate.h"
#endif
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
 for(uint32_t word:{0x7fc01234u,0x7f801234u,0x7f800000u,0x470ca100u,0xc70ca100u}){bool rejected=false;try{angleBasisCandidate({{word,0,0}});}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("angle domain accepted");}
 std::cout<<"x64_angle_domain_rejections=5\n";
 unsigned cases=0;std::array<uint32_t,18> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){std::array<uint32_t,6> trig{};std::copy_n(record.begin()+3,6,trig.begin());const auto result=angleBasisProductsCandidate(trig);
  std::array<uint32_t,3> angles{};std::copy_n(record.begin(),3,angles.begin());const auto complete=angleBasisCandidate(angles);if(complete!=result)throw std::runtime_error("complete x64 basis mismatch");
  for(unsigned word=0;word<9;++word)if(result[word]!=record[word+9]){std::cerr<<"case="<<cases<<" word="<<word<<" expected="<<std::hex<<record[word+9]<<" actual="<<result[word]<<"\n";throw std::runtime_error("basis mismatch");}++cases;
 }if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_basis_products_exact="<<cases<<"\n";std::cout<<"x64_complete_angle_bases_exact="<<cases<<"\n";
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
 // Original 10498be0 and standalone x87 FSINCOS only; no imports/initialization.
 using Basis=void(__cdecl*)(const void*,void*,void*,void*);auto basis=reinterpret_cast<Basis>(0x10498be0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x129abb42;
 uint32_t scaleWord=0x3c8efa35u;float scale;std::memcpy(&scale,&scaleWord,4);
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,18> record{};std::array<float,3> angles{};
  for(unsigned word=0;word<3;++word){seed=seed*1664525u+1013904223u;angles[word]=i<12?float(int(i)-6)*90.0f:float(int(seed%72000001u)-36000000)*0.001f;std::memcpy(&record[word],&angles[word],4);}
  for(unsigned pair=0;pair<3;++pair){const unsigned component=pair==0?1:pair==1?0:2;
   float radians=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(angles[component]),_mm_set_ss(scale)));float sine,cosine;
   __asm {fld radians}
   __asm {fsincos}
   __asm {fstp cosine}
   __asm {fstp sine}
   std::memcpy(&record[3+pair*2],&sine,4);std::memcpy(&record[4+pair*2],&cosine,4);
  }
  auto before=angles;std::array<uint32_t,5> f{},r{},u{};f.fill(0xa5d49e71u);r.fill(0xa5d49e71u);u.fill(0xa5d49e71u);basis(angles.data(),f.data()+1,r.data()+1,u.data()+1);
  if(angles!=before||f.front()!=0xa5d49e71u||f.back()!=0xa5d49e71u||r.front()!=0xa5d49e71u||r.back()!=0xa5d49e71u||u.front()!=0xa5d49e71u||u.back()!=0xa5d49e71u)throw std::runtime_error("basis bounds or source mutation");
  std::copy_n(f.begin()+1,3,record.begin()+9);std::copy_n(r.begin()+1,3,record.begin()+12);std::copy_n(u.begin()+1,3,record.begin()+15);output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_angle_bases=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
