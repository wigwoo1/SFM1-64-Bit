#include <windows.h>
#include <array>
#include "projected_light_texture_frame_candidate.h"
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
 unsigned cases=0;std::array<uint32_t,4> record{};
 if(projectedLightTextureFrameCandidate(0xffffffffu,0x7f801234u,false,0)!=0)throw std::runtime_error("no texture frame");
 for(int count:{0,-1}){bool rejected=false;try{projectedLightTextureFrameCandidate(0,0,true,count);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("invalid frame count accepted");}
 while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){int32_t count;std::memcpy(&count,&record[2],4);const auto result=projectedLightTextureFrameCandidate(record[0],record[1],true,count);uint32_t word;std::memcpy(&word,&result,4);if(word!=record[3])throw std::runtime_error("texture frame mismatch");++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_texture_frames_exact="<<cases<<"\n";
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
 // Copy only verified arithmetic107238a0..107238c4, append RET.
 const size_t bytes=0x107238c4-0x107238a0;auto copied=VirtualAlloc(nullptr,bytes+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copied)throw std::runtime_error("frame stub allocation");
 std::unique_ptr<void,void(*)(void*)> stubOwner(copied,[](void* p){VirtualFree(p,0,MEM_RELEASE);});std::memcpy(copied,reinterpret_cast<const void*>(0x107238a0),bytes);static_cast<uint8_t*>(copied)[bytes]=0xc3;
 if(!VirtualProtect(copied,bytes+1,PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("frame stub protection");FlushInstructionCache(GetCurrentProcess(),copied,bytes+1);
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");uint32_t seed=0x129abb42;
 const uint32_t boundary[]={0,0x80000000u,0xffffffffu,0x7fffffffu,0x3f800000u,0x7f800001u,0x7fc01234u,0x7f800000u};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,4> record{};std::array<uint32_t,80> object{};
  for(unsigned word=0;word<2;++word){seed=seed*1664525u+1013904223u;record[word]=i<64?boundary[(i+word)%8]:seed;}
  const int32_t count=int32_t(i%127)+1;record[2]=uint32_t(count);object[0x134/4]=record[0];object[0x13c/4]=record[1];auto before=object;auto source=object.data();int32_t result;
  __asm {push edi}
  __asm {mov edi,source}
  __asm {mov eax,count}
  __asm {call copied}
  __asm {mov result,edx}
  __asm {pop edi}
  if(object!=before)throw std::runtime_error("frame source mutated");std::memcpy(&record[3],&result,4);output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
 }output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_texture_frames=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
