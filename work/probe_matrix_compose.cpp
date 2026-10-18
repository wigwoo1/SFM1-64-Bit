#include <windows.h>
#include "matrix_compose_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,36> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  std::array<uint32_t,12> q{};std::array<uint32_t,12> p{};std::copy_n(record.begin(),12,q.begin());std::copy_n(record.begin()+12,12,p.begin());
  auto matrix=matrixComposeCandidate(q,p);
  for(unsigned word=0;word<12;++word)if(matrix[word]!=record[word+24]){std::cerr<<"case="<<cases<<" word="<<word<<" expected="<<std::hex<<record[word+24]<<" actual="<<matrix[word]<<"\n";throw std::runtime_error("matrix mismatch");}
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_composed_matrices_exact="<<cases<<"\n";
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
 // Only original 10498a60 executes; no imports, entrypoint or initialization.
 using Matrix=void(__cdecl*)(const void*,const void*,void*);
 auto matrix=reinterpret_cast<Matrix>(0x10498a60);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x129abb42;
 const uint32_t boundary[]={0,0x80000000u,1,0x807fffffu,0x00800000u,0x3f800000u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7f800001u,0xffbfffffu,0x7fc00001u};
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,36> record{};std::array<uint32_t,12> q{};std::array<uint32_t,12> p{};
  for(unsigned word=0;word<24;++word){seed=seed*1664525u+1013904223u;record[word]=i<216?boundary[(i+word)%12]:seed;}
  std::copy_n(record.begin(),12,q.begin());std::copy_n(record.begin()+12,12,p.begin());auto qb=q;auto pb=p;
  std::array<uint32_t,14> guarded{};guarded.fill(0xa5d49e71u);
  matrix(q.data(),p.data(),guarded.data()+1);
  if(q!=qb||p!=pb||guarded.front()!=0xa5d49e71u||guarded.back()!=0xa5d49e71u)throw std::runtime_error("matrix bounds or source mutation");
  std::copy_n(guarded.begin()+1,12,record.begin()+24);output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");
 std::cout<<"original_x86_composed_matrices=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
