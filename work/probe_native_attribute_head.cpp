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



static std::array<std::array<uint32_t,8>,16>attributePool{};static unsigned used=0;
static bool __fastcall disabled(void*,void*){return false;}
static void __fastcall boolSetter(void*,void*,bool){}
static bool __fastcall noUndo(void*,void*,void*){return false;}
static void __fastcall simpleNoop(void*,void*){}
static void __fastcall reasonNoop(void*,void*,int){}
static void __fastcall attrNoop(void*,void*,void*){}
static void* __cdecl createAttribute(void*,const char*,int){auto&attr=attributePool.at(used++);attr.fill(0);attr[6]=used;return attr.data();}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 std::vector<uint32_t>dm(13000);std::array<uint32_t,64>vt{};vt[0xf4/4]=uint32_t(&disabled);vt[0xf0/4]=uint32_t(&boolSetter);vt[0xf8/4]=uint32_t(&noUndo);dm[0]=uint32_t(vt.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(dm.data());*reinterpret_cast<uint32_t*>(0x10E9A518)=uint32_t(dm.data());
 redirect(0x10365000,reinterpret_cast<void*>(&createAttribute));redirect(0x10007370,reinterpret_cast<void*>(&simpleNoop));redirect(0x1035E400,reinterpret_cast<void*>(&reasonNoop));redirect(0x10360800,reinterpret_cast<void*>(&attrNoop));
 using Create=void*(__thiscall*)(void*,const char*,int);auto create=reinterpret_cast<Create>(0x103614F0);unsigned cases=0;
 for(unsigned seq=0;seq<128;++seq){std::array<uint32_t,32>element{};reinterpret_cast<unsigned char*>(element.data())[0x3e]=static_cast<unsigned char>((seq*2)|1);used=0;for(unsigned step=0;step<8;++step){auto*made=create(element.data(),"test",1);if(made!=attributePool[step].data())throw std::runtime_error("created pointer mismatch");std::array<uint32_t,12>out{};auto*attr=reinterpret_cast<uint32_t*>(element[0x30/4]);while(attr){out[1+out[0]++]=attr[6];attr=reinterpret_cast<uint32_t*>(attr[0]);}out[10]=reinterpret_cast<unsigned char*>(element.data())[0x3e];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}}
 std::cout<<"native_attribute_head_insertion_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
