#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "flex_execute_fixture.h"
static std::array<uint32_t,8> output;static void* destination;static unsigned cursor;
static void* __fastcall resolve(void*,void*,uint32_t handle){output[cursor++]=1;output[cursor++]=handle;return destination;}
static void* __fastcall construct(void* wrapper,void*,void* attr){*reinterpret_cast<void**>(wrapper)=attr;return wrapper;}
static void __fastcall arrayWrite(void* wrapper,void*,uint32_t index,uint32_t type,const uint32_t*bits){if(*reinterpret_cast<void**>(wrapper)!=destination)throw std::runtime_error("wrong wrapper");output[cursor++]=2;output[cursor++]=index;output[cursor++]=type;output[cursor++]=*bits;}
static void __fastcall scalarWrite(void* attr,void*,uint32_t type,const uint32_t*bits){if(attr!=destination)throw std::runtime_error("wrong destination");output[cursor++]=3;output[cursor++]=type;output[cursor++]=*bits;}
static void redirect(uint32_t address,void* function){auto*p=reinterpret_cast<unsigned char*>(address);p[0]=0xe9;int32_t r=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&r,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh oracle required");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(f)),{});if(bytes.size()<4096)throw std::runtime_error("missing ifm");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("mapping failed");auto sections=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(size_t(sections[i].PointerToRawData)+sections[i].SizeOfRawData>bytes.size())throw std::runtime_error("section bounds");std::memcpy(base+sections[i].VirtualAddress,bytes.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);}
 redirect(0x10362c90,reinterpret_cast<void*>(&construct));redirect(0x10362d70,reinterpret_cast<void*>(&arrayWrite));redirect(0x103629e0,reinterpret_cast<void*>(&scalarWrite));std::array<uint32_t,100>vt{};vt[0x168/4]=uint32_t(&resolve);std::array<uint32_t,1>framework{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(framework.data());using Execute=void(__thiscall*)(void*);std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("output unavailable");
 for(unsigned seed=0;seed<32768;++seed){auto fixture=flexExecuteFixture(seed);std::array<uint32_t,8>dest{},source{};dest[4]=fixture.destinationFlags;source[1]=uint32_t(&fixture.bits);source[4]=fixture.sourceFlags;destination=fixture.present?dest.data():nullptr;std::array<uint32_t,32>op{};op[0x68/4]=uint32_t(source.data());op[0x78/4]=fixture.handle;op[0x7c/4]=fixture.index;output.fill(0);cursor=0;reinterpret_cast<Execute>(0x105c24d0)(op.data());output[7]=cursor;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native flex execute cases=32768\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
