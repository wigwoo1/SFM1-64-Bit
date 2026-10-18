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
 unsigned cases=0;std::array<uint32_t,3> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  int32_t count;std::memcpy(&count,&record[0],4);
  if(genericHasDynamicLightCandidate(count,static_cast<uint8_t>(record[1]))!=bool(record[2]))throw std::runtime_error("light predicate mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 // Signed light-count bits must remain intact in NUM_LIGHTS, even when outside
 // the as-yet-unrecovered legal rendering domain.
 for(int32_t count : {INT32_MIN,-1,0,1,INT32_MAX})for(int32_t bones : {-1,0,1})for(uint8_t flow : {uint8_t(0),uint8_t(1),uint8_t(255)}){
  GenericVs20DynamicInputsCandidate inputs{1,count,255,bones,flow,2};
  auto fields=genericVs20DynamicFieldsCandidate(inputs);
  if(fields[0]!=1||fields[1]!=1||fields[2]!=uint32_t(bones>0)||fields[3]!=0||fields[4]!=(flow?0:static_cast<uint32_t>(count))||fields[5]!=1)throw std::runtime_error("scoped dynamic fields");
 }
 std::cout<<"x64_native_light_predicates_exact="<<cases<<" scoped_field_cases=45\n";
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
 // Original pure predicate only; no interface calls or DLL initialization.
 using Predicate=uint32_t(__thiscall*)(const void*);
 auto predicate=reinterpret_cast<Predicate>(0x10018160);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;const int32_t boundaries[]={INT32_MIN,-1,0,1,INT32_MAX};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,3> record{};
  seed=seed*1664525u+1013904223u;record[0]=seed;
  if(i<1280)std::memcpy(&record[0],&boundaries[i%5],4);
  record[1]=i<1280?i/5:i&255u;
  std::array<uint8_t,8> state{};std::memcpy(state.data(),&record[0],4);state[4]=static_cast<uint8_t>(record[1]);
  auto before=state;record[2]=predicate(state.data());if(state!=before)throw std::runtime_error("predicate mutation");
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_light_predicates=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
