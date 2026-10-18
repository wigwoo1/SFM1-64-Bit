#include <windows.h>
#include "dynamic_shader_commands_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <array>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 unsigned cases=0;std::array<uint32_t,21> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  auto state=dynamicShaderCommandsCandidate({record[0],record[1]},record.data()+2,7);
  if(state.vertex!=record[9]||state.pixel!=record[10])throw std::runtime_error("original dispatcher state mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 for(auto bad:std::vector<std::vector<uint32_t>>{{},{12},{11,1},{0,1},{1,0,0},{17,0}}){bool failed=false;
  try{dynamicShaderCommandsCandidate({123,456},bad.data(),bad.size());}catch(const std::runtime_error&){failed=true;}
  if(!failed)throw std::runtime_error("invalid dynamic stream accepted");}
 std::cout<<"x64_native_dynamic_streams_exact="<<cases<<" invalid_rejected=6\n";
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
 std::array<uint8_t,64> manager{},object{};auto managerPointer=manager.data();
 std::memcpy(reinterpret_cast<void*>(0x1016e20c),&managerPointer,4);
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 // Original dispatcher, restricted to audited commands11/12/0. Manager
 // global points only to owned synthetic storage; no DLL entrypoint runs.
 using Execute=void(__thiscall*)(void*,const uint32_t*);
 auto execute=reinterpret_cast<Execute>(0x10016ac0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x194782ab;
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,21> record{};
  auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
  record[0]=next();record[1]=next();
  for(unsigned j=0;j<3;++j){record[2+j*2]=((i>>j)&1)?11:12;record[3+j*2]=next();}
  record[8]=0;manager.fill(0xa5);std::memcpy(manager.data()+4,&record[0],4);std::memcpy(manager.data()+8,&record[1],4);
  auto before=manager;execute(object.data(),record.data()+2);
  for(size_t j=0;j<manager.size();++j)if((j<4||j>=12)&&manager[j]!=before[j])throw std::runtime_error("native command unrelated storage changed");
  std::memcpy(&record[9],manager.data()+4,4);std::memcpy(&record[10],manager.data()+8,4);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_dynamic_streams=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
