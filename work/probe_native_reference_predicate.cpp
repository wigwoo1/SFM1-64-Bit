#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<std::array<uint32_t,64>,3> slaves{},dags{},transforms{};
static std::array<float,3> target{},pole{};
static void* __fastcall resolve(void*,void*,uint32_t h){if(h>=1&&h<=3)return slaves[h-1].data();if(h>=11&&h<=13)return dags[h-11].data();return nullptr;}
static void* __fastcall getTransform(void*self,void*){for(unsigned i=0;i<3;++i)if(self==dags[i].data())return transforms[i].data();return nullptr;}
static void* __fastcall getControl(void*,void*){return nullptr;}
static void __fastcall getFrame(void*,void*,float*out){std::memset(out,0,48);out[0]=out[5]=out[10]=1;}
static float __fastcall getAverage(void*,void*,float*out){std::memcpy(out,target.data(),12);return .75f;}
static void __fastcall getPole(void*,void*,float*out){std::memcpy(out,pole.data(),12);}
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}



static std::array<uint32_t,32> attribute{},owner{};static bool attributePresent=false,classAccepted=false;
static void* __fastcall getAttribute(void*,void*,uint32_t){return attributePresent?attribute.data():nullptr;}
static bool __fastcall isType(void*,void*,uint32_t){return classAccepted;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 std::array<uint32_t,90>vtable{};vtable[89]=uint32_t(&getAttribute);uint32_t object=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(&object);
 std::array<uint32_t,5>ov{};ov[4]=uint32_t(&isType);owner[1]=uint32_t(ov.data());
 using Predicate=void*(__thiscall*)(void*,uint32_t,bool,int);std::array<uint32_t,2>iterator{1,77};unsigned cases=0;
 for(unsigned kind=0;kind<2;++kind)for(unsigned mode=0;mode<5;++mode)for(unsigned flags=0;flags<4;++flags)for(unsigned bits=0;bits<128;++bits){attributePresent=bits&1;bool ownerPresent=bits&2;classAccepted=bits&4;bool nameMatches=bits&8,checkConsistency=bits&16,consistent=bits&32;
 attribute[2]=ownerPresent?uint32_t(owner.data()+1):0;attribute[4]=((flags&1)?0x800:0)|((flags&2)?0x1000:0);attribute[5]=9;owner[15]=consistent?77:78;owner[15-1]=consistent?77:78;
 // Channel uses owner-adjusted+3C (owner base+3C); slave uses unadjusted+38 (also base+3C).
 auto fn=reinterpret_cast<Predicate>(kind?0x101A4B90:0x100BBCF0);void*result=fn(iterator.data(),(bits&64)?0xffffffffu:(nameMatches?9:10),checkConsistency!=0,int(mode));uint32_t accepted=result?1:0;oracle.write(reinterpret_cast<const char*>(&accepted),4);++cases;}
 std::cout<<"native_reference_predicate_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
