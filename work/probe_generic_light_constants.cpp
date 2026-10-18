#include <windows.h>
#include "generic_light_constants_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,363> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t count;std::memcpy(&count,&record[0],4);
  std::array<uint32_t,88> lights{};std::copy_n(record.begin()+1,88,lights.begin());
  std::array<uint32_t,137> destination{};std::copy_n(record.begin()+89,137,destination.begin());
  genericLightConstantsCandidate(destination,count,lights);
  if(!std::equal(destination.begin(),destination.end(),record.begin()+226))throw std::runtime_error("light constants mismatch");
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::array<uint32_t,137> d{};d.fill(7);auto before=d;std::array<uint32_t,88> lights{};
 try{genericLightConstantsCandidate(d,5,lights);throw std::runtime_error("missing capacity guard");}catch(const std::invalid_argument&){}
 if(d!=before)throw std::runtime_error("capacity failure mutation");
 std::cout<<"x64_native_light_constants_exact="<<cases<<"\n";
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
 // Only original leaf10012210 executes; no DLL initialization or imports.
 using Pack=void(__thiscall*)(void*,void*,int32_t,const void*);
 auto pack=reinterpret_cast<Pack>(0x10012210);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0xabc49e71;
 const int32_t counts[]={INT32_MIN,-1,0,1,2,3,4};
 const uint32_t boundary[]={0,0x80000000u,1,0x807fffffu,0x00800000u,0x3f800000u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7f800001u,0xffbfffffu,0x7fc00001u};
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,363> record{};int32_t count=counts[i%7];std::memcpy(&record[0],&count,4);
  std::array<uint32_t,110> payload{};std::memcpy(&payload[21],&count,4);
  for(unsigned word=0;word<88;++word){seed=seed*1664525u+1013904223u;payload[22+word]=i<216?boundary[(i+word)%12]:seed;}
  for(unsigned light=0;light<4;++light)payload[22+light*22]=(i+light)%5;
  std::copy_n(payload.begin()+22,88,record.begin()+1);auto payloadBefore=payload;
  std::array<uint32_t,139> guarded{};guarded.front()=guarded.back()=0xb794d1e5u;
  for(unsigned word=0;word<137;++word){seed=seed*1664525u+1013904223u;guarded[1+word]=seed;record[89+word]=seed;}
  pack(nullptr,guarded.data()+1,123,payload.data());
  if(payload!=payloadBefore||guarded.front()!=0xb794d1e5u||guarded.back()!=0xb794d1e5u)throw std::runtime_error("light constants bounds or source mutation");
  std::copy_n(guarded.begin()+1,137,record.begin()+226);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");
 std::cout<<"original_x86_light_constants=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
