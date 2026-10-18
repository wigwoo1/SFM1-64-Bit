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


static std::array<uint32_t,6> candidates{};
static std::array<std::array<uint32_t,2>,6> objects{};
static std::array<unsigned,6> states{};
static unsigned count=0,lookups=0,classTests=0;
static void* __fastcall allocate(void*,void*,unsigned){return candidates.data();}
static void __fastcall release(void*,void*,void*){}
static void __fastcall destroyList(void*,void*){}
static void __fastcall gather(void*,void*,uint32_t*out){out[0]=uint32_t(candidates.data());out[1]=6;out[2]=0;out[3]=count;out[4]=out[0];}
static void* __fastcall channelTransform(void*self,void*){++lookups;unsigned i=unsigned(reinterpret_cast<uintptr_t>(self))-1;return states.at(i)==1?nullptr:objects.at(i).data();}
static bool __fastcall isControl(void*self,void*,uint32_t){++classTests;return states.at(static_cast<uint32_t*>(self)[1])==3;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




 std::array<uint32_t,6> allocatorVtable{},objectVtable{};allocatorVtable[1]=uint32_t(&allocate);allocatorVtable[5]=uint32_t(&release);uint32_t allocatorObject=uint32_t(allocatorVtable.data());uint32_t allocatorPointer=uint32_t(&allocatorObject);*reinterpret_cast<uint32_t*>(0x10A7E484)=uint32_t(&allocatorPointer);
 objectVtable[4]=uint32_t(&isControl);for(unsigned i=0;i<6;++i){objects[i][0]=uint32_t(objectVtable.data());objects[i][1]=i;}
 redirect(0x104C6510,reinterpret_cast<void*>(&gather));redirect(0x105C6060,reinterpret_cast<void*>(&channelTransform));redirect(0x10A38EC0,reinterpret_cast<void*>(&destroyList));
 using Select=void*(__thiscall*)(void*);auto select=reinterpret_cast<Select>(0x104C65C0);std::array<char,256>dag{};unsigned cases=0;
 for(count=0;count<=6;++count){unsigned patterns=1u<<(count*2);for(unsigned pattern=0;pattern<patterns;++pattern){for(unsigned i=0;i<count;++i){states[i]=(pattern>>(i*2))&3;candidates[i]=states[i]?i+1:0;}
 lookups=classTests=0;auto chosen=select(dag.data());uint32_t result=0xffffffffu;for(unsigned i=0;i<count;++i)if(chosen==objects[i].data())result=i;
 oracle.write(reinterpret_cast<const char*>(&result),4);oracle.write(reinterpret_cast<const char*>(&lookups),4);oracle.write(reinterpret_cast<const char*>(&classTests),4);++cases;}}
 std::cout<<"native_control_selection_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
