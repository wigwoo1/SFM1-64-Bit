#include <windows.h>
#include "quaternion_angles_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,7> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){std::array<uint32_t,4> q{};std::copy_n(record.begin(),4,q.begin());const auto result=quaternionAnglesCandidate(q);
  for(unsigned word=0;word<3;++word)if(result[word]!=record[word+4]){std::cerr<<"case="<<cases<<" word="<<word<<" expected="<<std::hex<<record[word+4]<<" actual="<<result[word]<<"\n";throw std::runtime_error("quaternion angle mismatch");}++cases;
 }if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_quaternion_angles_exact="<<cases<<"\n";
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
 // Original 1049e160, bounded math helper10a0e901 and matching cookie check only.
 using Angles=void(__cdecl*)(const void*,void*);auto angles=reinterpret_cast<Angles>(0x1049e160);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x129abb42;
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,7> record{};std::array<float,4> q{};
  for(unsigned word=0;word<4;++word){seed=seed*1664525u+1013904223u;q[word]=i<16?float((int(i+word)%3)-1):float(int(seed%2000001u)-1000000)*0.000001f;std::memcpy(&record[word],&q[word],4);}
  auto before=q;std::array<uint32_t,5> guarded{};guarded.fill(0xa5d49e71u);angles(q.data(),guarded.data()+1);
  if(q!=before||guarded.front()!=0xa5d49e71u||guarded.back()!=0xa5d49e71u)throw std::runtime_error("angle bounds or source mutation");
  std::copy_n(guarded.begin()+1,3,record.begin()+4);output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_quaternion_angles=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
