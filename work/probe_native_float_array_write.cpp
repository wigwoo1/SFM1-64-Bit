#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include "float_array_write_fixture.h"
static std::array<uint32_t,32> output{};static unsigned cursor;static uint32_t* current;static uint16_t* flags;static bool allowed;
static void emit(uint32_t code,uint32_t arg){output[cursor++]=code;output[cursor++]=arg;output[cursor++]=floatArrayWriteHash(current);output[cursor++]=*flags;}
static bool __fastcall permission(void*,void*){emit(1,0);if(allowed)*flags|=0x4080;return allowed;}
static bool __fastcall undo(void*,void*,void*){emit(2,0);return false;}
#ifndef TEST_OWNER_GATES
static void __fastcall before(void*,void*,int first,int last){emit(3,uint32_t(first)|(uint32_t(last)<<8));}
static void __fastcall after(void*,void*,int first,int last,int changed){emit(4,uint32_t(first)|(uint32_t(last)<<8)|(uint32_t(changed)<<16));}
#endif
static void __fastcall finish(void*,void*,int a,int b){emit(5,uint32_t(a)|(uint32_t(b)<<8));}
#ifdef TEST_OWNER_GATES
static unsigned gateMode;static uint32_t* currentOwner;
static void __fastcall ownerBefore(void*,void*,void*,int first,int last){
 emit(3,uint32_t(first)|(uint32_t(last)<<8));
 if(gateMode==4)*flags&=0xfeffu;
 if(gateMode==5)reinterpret_cast<uint8_t*>(currentOwner)[0x3e]&=0xfbu;
}
static void __fastcall ownerAfter(void*,void*,void*,int first,int last){emit(4,uint32_t(first)|(uint32_t(last)<<8));}
#endif
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xe9;int32_t r=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&r,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"x86 required");if(argc!=2)throw std::runtime_error("fresh oracle required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite");
 std::ifstream file("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});if(bytes.size()<4096)throw std::runtime_error("missing ifm");
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected base");
 auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("mapping failed");
 auto sections=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(size_t(sections[i].PointerToRawData)+sections[i].SizeOfRawData>bytes.size())throw std::runtime_error("section bounds");std::memcpy(base+sections[i].VirtualAddress,bytes.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);}
 redirect(0x103623b0,reinterpret_cast<void*>(&permission));redirect(0x10362430,reinterpret_cast<void*>(&finish));
#ifndef TEST_OWNER_GATES
 redirect(0x10362eb0,reinterpret_cast<void*>(&before));redirect(0x10362ef0,reinterpret_cast<void*>(&after));
#endif
 std::array<uint32_t,100> vt{};vt[0xf8/4]=uint32_t(&undo);std::array<uint32_t,1> framework{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(framework.data());
 using Write=void(__thiscall*)(void*,void*,int,uint32_t,const uint32_t*);std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("output unavailable");
 unsigned caseCount=20160;
#ifdef TEST_OWNER_GATES
 caseCount*=6;
 std::array<uint32_t,3> ownerVt{0,uint32_t(&ownerAfter),uint32_t(&ownerBefore)};
#endif
 for(unsigned seed=0;seed<caseCount;++seed){auto f=floatArrayWriteFixture(seed%20160);auto values=f.old;current=values.data();allowed=f.allowed;std::array<uint32_t,4> storage{uint32_t(values.data()),4,0,uint32_t(f.count)};std::array<uint32_t,8> attr{};attr[1]=uint32_t(storage.data());attr[4]=17;flags=reinterpret_cast<uint16_t*>(&attr[4]);output.fill(0);cursor=0;
#ifdef TEST_OWNER_GATES
 gateMode=seed/20160;std::array<uint32_t,24> owner{};currentOwner=owner.data();owner[0]=uint32_t(ownerVt.data());
 if(gateMode>=4||(gateMode&1))*flags|=0x100;
 if(gateMode>=4||(gateMode&2))reinterpret_cast<uint8_t*>(owner.data())[0x3e]|=4;
 attr[2]=uint32_t(owner.data());
#endif
 reinterpret_cast<Write>(0x1036f010)(nullptr,attr.data(),f.index,f.sourceType,&f.incoming);
 for(unsigned i=0;i<4;++i)output[24+i]=values[i];output[28]=*flags;output[30]=cursor;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native float-array write cases="<<caseCount<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
