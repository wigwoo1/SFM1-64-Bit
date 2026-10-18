#include <windows.h>
#include "queued_vertex_metadata_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,72> record{};
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
  const unsigned width=record[0]?4:1,index=record[1],count=record[2];
  VertexMetadataUploadCandidate command{record[0]?VertexMetadataKindCandidate::Integer:VertexMetadataKindCandidate::Boolean,index,{record.begin()+4,record.begin()+4+count*width},record[3]!=0};
  bool changed=command.force||!std::equal(command.words.begin(),command.words.end(),record.begin()+20+index*width);
  auto queue=changed?queuedVertexMetadataCandidate(command):std::vector<uint32_t>{};
  if(queue.size()!=record[36]||!std::equal(queue.begin(),queue.end(),record.begin()+37))throw std::runtime_error("native queue words mismatch");
  auto cache=std::vector<uint32_t>(record.begin()+20,record.begin()+36);
  if(changed)std::copy(command.words.begin(),command.words.end(),cache.begin()+index*width);
  if(!std::equal(cache.begin(),cache.end(),record.begin()+56))throw std::runtime_error("native queue cache mismatch");
  ++cases;
 }
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");
 std::cout<<"x64_native_metadata_queues_exact="<<cases<<"\n";
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
 // Original metadata methods and private memcpy only. Queue space avoids refill.
 using Set=void(__thiscall*)(void*,uint32_t,const uint32_t*,uint32_t,uint32_t);
 auto booleanSet=reinterpret_cast<Set>(0x100100a0),integerSet=reinterpret_cast<Set>(0x100101e0);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");
 uint32_t seed=0x714abcd3;
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,72> record{};record[0]=i&1;const unsigned width=record[0]?4:1;
  record[2]=(i/2)%5;record[1]=(i/10)%(16/width+1-record[2]);record[3]=(i/50)%2;
  for(unsigned word=0;word<16;++word){seed=seed*1664525u+1013904223u;record[4+word]=seed;seed=seed*1664525u+1013904223u;record[20+word]=seed;}
  if(i%3==0)std::copy_n(record.begin()+20+record[1]*width,record[2]*width,record.begin()+4);
  auto sourceBefore=record;
  std::array<uint32_t,16> cache{},mirror{};std::copy_n(record.begin()+20,16,cache.begin());mirror=cache;
  std::array<uint32_t,32> queue{};queue.fill(0xdab18743u);
  std::vector<uint32_t> state(0x2600/4);
  state[0x388/4]=1;state[0x390/4]=reinterpret_cast<uint32_t>(queue.data());state[0x394/4]=32;
  state[(record[0]?0x1718:0x1714)/4]=reinterpret_cast<uint32_t>(mirror.data());
  state[(record[0]?0x2590:0x258c)/4]=reinterpret_cast<uint32_t>(cache.data());
  (record[0]?integerSet:booleanSet)(state.data(),record[1],record.data()+4,record[2],record[3]);
  const auto used=(state[0x390/4]-reinterpret_cast<uint32_t>(queue.data()))/4;
  if(used>19||state[0x394/4]!=32-used||cache!=mirror||!std::equal(record.begin()+4,record.begin()+20,sourceBefore.begin()+4))throw std::runtime_error("native queue bounds or cache mutation");
  for(unsigned n=used;n<32;++n)if(queue[n]!=0xdab18743u)throw std::runtime_error("queue tail overwritten");
  record[36]=used;std::copy_n(queue.begin(),used,record.begin()+37);std::copy(cache.begin(),cache.end(),record.begin()+56);
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_metadata_queues=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
