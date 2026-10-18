#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




static uint32_t* __fastcall metadata(void*,void*,uint32_t*out,uint32_t text){if(text!=0x10b38234)throw std::runtime_error("metadata name");*out=0x12345678;return out;}
int main(){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);

 std::array<uint32_t,82>vt{};vt[0x140/4]=uint32_t(&metadata);vt[0x144/4]=uint32_t(&metadata);std::array<uint32_t,1>model{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());
 using Constructor=void*(__thiscall*)(void*,void*,void*);
 const uint32_t palette[]={0,0x80000000u,0x3f800000u,0x7f800000u,0xff800000u,0x7f801234u,0xff801234u,0x7fc01234u,1,0x807fffffu};
 unsigned checks=0;
 for(unsigned n:{3u,4u})for(unsigned seed=0;seed<1024;++seed){std::array<uint32_t,8>owner{},attr{};std::array<uint32_t,4>before{},after{};std::array<uint32_t,16>record;record.fill(0xabababab);owner[7]=seed*313;attr[2]=uint32_t(owner.data());attr[1]=uint32_t(before.data());unsigned type=seed%32;if(n==4&&type==12)continue;attr[4]=type|((seed/32)<<5);attr[5]=seed&1?0xffffffffu:seed;for(unsigned i=0;i<n;++i){before[i]=palette[(seed+i)%10];after[i]=palette[(seed*3+i+5)%10];}
 auto result=reinterpret_cast<Constructor>(n==3?0x1036a390:0x10368140)(record.data(),attr.data(),after.data());if(result!=record.data()||record[6]!=owner[7]||record[5]!=attr[5]||record[2]!=0x12345678||record[3]!=0x12345678)throw std::runtime_error("constructor fields");
 for(unsigned i=0;i<n;++i)for(unsigned side=0;side<2;++side){uint32_t expected=side?after[i]:before[i];if(!side&&type!=(n==3?10u:13u))expected=(n==4&&i==3)?0x3f800000u:0;if(n==3&&(expected&0x7f800000u)==0x7f800000u&&(expected&0x7fffffu))expected|=0x400000u;if(record[(side?7+n:7)+i]!=expected)throw std::runtime_error("constructor copy");++checks;}}
 std::cout<<"native typed constructor type/flag cases component_checks="<<checks<<" passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
