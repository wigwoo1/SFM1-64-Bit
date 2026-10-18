#include <windows.h>
#include "generic_light_state_candidate.h"
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
 std::array<uint32_t,18> baseline;
 input.read(reinterpret_cast<char*>(baseline.data()),sizeof(baseline));if(!input)throw std::runtime_error("baseline");
 unsigned cases=0;std::array<uint32_t,99> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  if(record[0]>4)throw std::runtime_error("record count");std::vector<GenericLightRecordCandidate> records;
  for(unsigned j=0;j<record[0];++j){auto p=record.data()+1+j*24;GenericLightRecordCandidate r;r.present=p[0]!=0;r.secondFlagWord=p[1];std::copy_n(p+2,18,r.ambientWords.begin());std::memcpy(&r.lightCount,p+23,4);records.push_back(r);}
  RetainedGenericLightStateCandidate retained;retained.replace(records,baseline);
  auto result=retained.query();auto repeated=retained.query();
  if(retained.aggregations()!=1||repeated.lightCount!=result.lightCount||repeated.firstFlag!=result.firstFlag||repeated.secondFlag!=result.secondFlag)throw std::runtime_error("retained light state");
  retained.invalidate();retained.query();if(retained.aggregations()!=2)throw std::runtime_error("light invalidation");
  retained.replace({},baseline);auto empty=retained.query();if(empty.lightCount||empty.firstFlag||empty.secondFlag||retained.aggregations()!=3)throw std::runtime_error("replacement invalidation");
  uint32_t count;std::memcpy(&count,&result.lightCount,4);
  if(count!=record[97]||(uint32_t(result.firstFlag)|(uint32_t(result.secondFlag)<<8)) != record[98])throw std::runtime_error("light state mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_light_states_exact="<<cases<<'\n';
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
 // Original aggregation over synthetic records; static baseline is read from
 // the private image. No DLL runtime or imports are initialized.
 using Aggregate=void(__thiscall*)(void*);auto aggregate=reinterpret_cast<Aggregate>(0x10014160);
 std::array<uint32_t,18> baseline;std::memcpy(baseline.data(),reinterpret_cast<void*>(0x10180158),sizeof(baseline));
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");output.write(reinterpret_cast<const char*>(baseline.data()),sizeof(baseline));
 uint32_t seed=0x194782ab;
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,99> record{};record[0]=i%5;
  std::array<uint8_t,0x3000> object{};std::array<std::array<uint32_t,19>,4> nativeRecords{};
  for(unsigned j=0;j<4;++j){auto p=record.data()+1+j*24;seed=seed*1664525u+1013904223u;p[0]=(seed>>22)&1u;p[1]=(seed>>23)&1u;std::copy(baseline.begin(),baseline.end(),p+2);
   if((seed>>24)&1u)p[2+(i+j)%18]^=1u<<((i/18+j)%32);
   p[23]=i<1280?std::array<uint32_t,5>{0,1,0xffffffffu,0x80000000u,0x7fffffffu}[(i/5+j)%5]:seed;
   nativeRecords[j][6]=p[0]?reinterpret_cast<uint32_t>(p+2):0;nativeRecords[j][11]=p[1];
  }
  std::memcpy(object.data()+0x2eb8,&record[0],4);auto pointer=reinterpret_cast<uint32_t>(nativeRecords.data());std::memcpy(object.data()+0x2ebc,&pointer,4);
  auto before=record;aggregate(object.data());if(record!=before)throw std::runtime_error("record mutation");
  std::memcpy(&record[97],object.data()+0x1864,4);uint16_t flags;std::memcpy(&flags,object.data()+0x1868,2);record[98]=flags;
  using Query=void(__thiscall*)(void*,void*);auto query=reinterpret_cast<Query>(0x10014260);
  object[0x186c]=0;std::array<uint8_t,8> cold{},warm{},fresh{};
  query(object.data()+0x2d0,cold.data());if(object[0x186c]!=1)throw std::runtime_error("native cache valid");
  uint32_t zero=0;std::memcpy(object.data()+0x2eb8,&zero,4);
  query(object.data()+0x2d0,warm.data());if(warm!=cold)throw std::runtime_error("native warm cache");
  object[0x186c]=0;query(object.data()+0x2d0,fresh.data());if(fresh!=std::array<uint8_t,8>{})throw std::runtime_error("native invalidation");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_states=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
