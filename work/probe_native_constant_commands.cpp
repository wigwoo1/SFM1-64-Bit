#include <windows.h>
#include "native_shader_command_plan_candidate.h"
#include "retained_shader_constants_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <array>
#ifndef _WIN64
std::array<uint32_t,32> hardwareVs{},hardwarePs{};unsigned calls=0;uint32_t lastIndex=0,lastCount=0;bool invalidCallback=false;
template<bool Pixel> HRESULT __stdcall upload(void*,UINT index,const float* values,UINT count){++calls;lastIndex=index;lastCount=count;if(index>8||count>8-index){invalidCallback=true;return E_FAIL;}auto& output=Pixel?hardwarePs:hardwareVs;std::memcpy(output.data()+index*4,values,count*16);return S_OK;}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle open");
 unsigned cases=0;std::array<uint32_t,152> record;
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  std::vector<uint32_t> stream(record.begin()+65,record.begin()+65+record[64]);
  auto plan=nativeShaderCommandPlanCandidate(stream,8,8);std::array<uint32_t,32> vs,ps;
  std::copy_n(record.begin(),32,vs.begin());std::copy_n(record.begin()+32,32,ps.begin());
  RetainedShaderConstantStageCandidate vertex(false,{vs.begin(),vs.end()},true),pixel(true,{ps.begin(),ps.end()},true);
  unsigned calls=0;uint32_t lastIndex=UINT32_MAX,lastCount=UINT32_MAX;
  for(const auto& upload:plan.uploads){auto& cache=upload.pixel?pixel:vertex;
   cache.apply(upload.index,upload.words,[&](const NativeShaderConstantWordsCandidate& command){++calls;lastIndex=command.index;lastCount=static_cast<uint32_t>(command.words.size()/4);auto& output=command.pixel?ps:vs;std::copy(command.words.begin(),command.words.end(),output.begin()+command.index*4);});}
  if(calls!=record[149]||lastIndex!=record[150]||lastCount!=record[151])throw std::runtime_error("native upload callback range mismatch");
  if(!std::equal(vertex.words().begin(),vertex.words().end(),vs.begin())||!std::equal(pixel.words().begin(),pixel.words().end(),ps.begin()))throw std::runtime_error("retained constant cache mismatch");
  if(!std::equal(vs.begin(),vs.end(),record.begin()+85)||!std::equal(ps.begin(),ps.end(),record.begin()+117))throw std::runtime_error("native constant state mismatch");++cases;
 }
 if(input.gcount()||!input.eof()||cases!=16384)throw std::runtime_error("oracle length");
 RetainedShaderConstantStageCandidate unknown(false,std::vector<uint32_t>(4),false);unsigned callbacks=0;
 unknown.apply(0,std::vector<uint32_t>(4),[&](const auto&){++callbacks;});unknown.apply(0,std::vector<uint32_t>(4),[&](const auto&){++callbacks;});unknown.invalidate();unknown.apply(0,std::vector<uint32_t>(4),[&](const auto&){++callbacks;});
 if(callbacks!=2)throw std::runtime_error("unknown/invalidation upload policy");
 auto before=unknown.words();bool failed=false;try{unknown.apply(0,{1,2,3,4},[](const auto&){throw std::runtime_error("injected device failure");});}catch(const std::runtime_error&){failed=true;}
 if(!failed||unknown.words()!=before)throw std::runtime_error("failed upload committed cache");
 std::cout<<"x64_native_constant_states_exact="<<cases<<" callback_ranges_exact=passed invalidation=passed failure_atomicity=passed\n";
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
 std::array<uint8_t,64> manager{};auto managerPointer=manager.data();
 std::memcpy(reinterpret_cast<void*>(0x1016e20c),&managerPointer,4);
 DWORD oldProtect;if(!VirtualProtect(allocation,nt->OptionalHeader.SizeOfImage,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("code protection");
 FlushInstructionCache(GetCurrentProcess(),allocation,nt->OptionalHeader.SizeOfImage);
 using Execute=void(__thiscall*)(void*,const uint32_t*);auto execute=reinterpret_cast<Execute>(0x10016ac0);
 std::array<uintptr_t,110> table{};table[94]=reinterpret_cast<uintptr_t>(&upload<false>);table[109]=reinterpret_cast<uintptr_t>(&upload<true>);
 auto tablePointer=table.data();auto device=&tablePointer;
 std::vector<uint8_t> object(0x3000);std::array<uint32_t,32> vs{},ps{},mirrorVs{},mirrorPs{};
 auto pointer=[&](size_t offset,void* value){std::memcpy(object.data()+offset,&value,4);};
 pointer(0x378,device);pointer(0x1710,vs.data());pointer(0x171c,ps.data());pointer(0x2588,mirrorVs.data());pointer(0x2594,mirrorPs.data());
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x594092ab;
 for(unsigned i=0;i<16384;++i){std::array<uint32_t,152> record{};auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
  for(unsigned j=0;j<64;++j)record[j]=next();std::copy_n(record.begin(),32,vs.begin());std::copy_n(record.begin()+32,32,ps.begin());mirrorVs=vs;mirrorPs=ps;hardwareVs=vs;hardwarePs=ps;
  bool pixel=i&1;uint32_t count=1+(i/2)%4,index=next()%(9-count);record[64]=4+count*4;record[65]=pixel?3:4;record[66]=index;record[67]=count;
  auto& before=pixel?ps:vs;
  for(uint32_t j=0;j<count*4;++j)record[68+j]=(i%5==0||(i%5==1&&j<4)||i%5==4)?before[index*4+j]:next();
  if(i%5==4)record[68+count*4-1]^=0x80000000u;
  record[68+count*4]=0;calls=0;lastIndex=lastCount=UINT32_MAX;invalidCallback=false;
  execute(object.data(),record.data()+65);
  if(invalidCallback||vs!=mirrorVs||ps!=mirrorPs||vs!=hardwareVs||ps!=hardwarePs)throw std::runtime_error("original constant storage/device disagreement");
  std::copy(vs.begin(),vs.end(),record.begin()+85);std::copy(ps.begin(),ps.end(),record.begin()+117);record[149]=calls;record[150]=lastIndex;record[151]=lastCount;
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_constant_streams=16384\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
