#include <windows.h>
#include <array>
#include <string>
#include "material_animation_frame_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef _WIN64
__declspec(naked) void executeMaterialArithmetic(void*,uint32_t*,uint32_t*,uint32_t*,int32_t,uint32_t*){
 __asm {push ebp
 push esi
 push edi
 push ebx
 mov esi,[esp+24]
 mov eax,[esp+28]
 mov ebp,[esp+32]
 add ebp,4
 mov edi,[esp+36]
 call dword ptr [esp+20]
 mov ecx,[esp+40]
 mov [ecx],eax
 mov [ecx+4],edx
 pop ebx
 pop edi
 pop esi
 pop ebp
 ret}
}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,7>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){auto v=materialAnimationFrameCandidate(r[0],r[1],r[2],r[3],int32_t(r[4]),true);if(uint32_t(v.frame)!=r[5]||uint32_t(v.previousFrame)!=r[6])throw std::runtime_error("material arithmetic mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_material_frames_exact=65536\n";
#else
 if(std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");
 std::ifstream input(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll)",std::ios::binary);
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
 const size_t bytes=0x1000f1b0-0x1000f16a;auto copied=VirtualAlloc(nullptr,bytes+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copied)throw std::runtime_error("stub allocation");
 std::unique_ptr<void,void(*)(void*)> stubOwner(copied,[](void*p){VirtualFree(p,0,MEM_RELEASE);});std::memcpy(copied,reinterpret_cast<const void*>(0x1000f16a),bytes);
 const uint8_t tail[]={0x8b,0xc3,0x5b,0xc3};std::memcpy(static_cast<uint8_t*>(copied)+bytes,tail,sizeof(tail));
 if(!VirtualProtect(copied,bytes+sizeof(tail),PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("stub protection");FlushInstructionCache(GetCurrentProcess(),copied,bytes+sizeof(tail));
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,7>r{};for(unsigned j=0;j<4;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[4]=1+i%127;
 std::array<uint32_t,8>object{},clock{};object[3]=r[3];clock[3]=r[0];clock[4]=r[1];auto beforeObject=object,beforeClock=clock;uint32_t start=r[2],out[2]{};executeMaterialArithmetic(copied,object.data(),clock.data(),&start,int32_t(r[4]),out);r[5]=out[0];r[6]=out[1];if(object!=beforeObject||clock!=beforeClock||start!=r[2])throw std::runtime_error("input mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_material_frames=65536\n";
#endif
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
