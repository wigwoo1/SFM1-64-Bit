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
static std::array<float,12> frame{};
static void* __fastcall resolve(void*,void*,uint32_t h){if(h>=1&&h<=3)return slaves[h-1].data();if(h>=11&&h<=13)return dags[h-11].data();return nullptr;}
static void* __fastcall getTransform(void*self,void*){for(unsigned i=0;i<3;++i)if(self==dags[i].data())return transforms[i].data();return nullptr;}
static void* __fastcall getControl(void*,void*){return nullptr;}
static void __fastcall getFrame(void*,void*,float*out){std::memcpy(out,frame.data(),48);}
static float __fastcall getAverage(void*,void*,float*out){std::memcpy(out,target.data(),12);return .75f;}
static void __fastcall getPole(void*,void*,float*out){std::memcpy(out,pole.data(),12);}
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 redirect(0x104C27D0,reinterpret_cast<void*>(&getTransform));redirect(0x104C65C0,reinterpret_cast<void*>(&getControl));redirect(0x104C6100,reinterpret_cast<void*>(&getFrame));redirect(0x105CF4A0,reinterpret_cast<void*>(&getAverage));redirect(0x104C5900,reinterpret_cast<void*>(&getPole));
 std::array<uint32_t,128> model{},table{},op{};table[0x48/4]=uint32_t(reinterpret_cast<uintptr_t>(&resolve));model[0]=uint32_t(reinterpret_cast<uintptr_t>(table.data()));*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(reinterpret_cast<uintptr_t>(model.data()));
 for(unsigned i=0;i<3;++i){slaves[i][0x5C/4]=11+i;op[(0xD8+i*12)/4]=1+i;}op[0xCC/4]=11;
 unsigned checks=0;using Solve=float(__thiscall*)(void*,float*,float*,const float*,const float*);
 for(uint32_t pattern=0;pattern<1024;++pattern){frame.fill(0);const float scale=(pattern&1)?2.0f:.5f;frame[1]=-scale;frame[4]=scale;frame[10]=scale;frame[3]=float(int(pattern%9)-4)*.125f;frame[7]=.25f;frame[11]=-.5f;target={.25f+float(pattern%31)*.125f,float(int(pattern%17)-8)*.125f,.375f};pole={.25f,.5f,1};
 std::array<float,3>root{float(int(pattern%13)-6)*.125f,.375f,-.25f};std::memcpy(reinterpret_cast<char*>(transforms[0].data())+0x5C,root.data(),12);float upper=.5f+float(pattern%11)*.125f,lower=.75f+float(pattern%7)*.125f;std::memcpy(reinterpret_cast<char*>(transforms[1].data())+0x5C,&upper,4);std::memcpy(reinterpret_cast<char*>(transforms[2].data())+0x5C,&lower,4);
 std::array<float,4>a{},b{},offset{},midOffset{};for(unsigned i=0;i<4;++i){offset[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;midOffset[i]=float(int((pattern*23+i*7)%113)-56)*.0625f;}float influence=reinterpret_cast<Solve>(0x105D0730)(op.data(),a.data(),b.data(),offset.data(),midOffset.data());
 oracle.write(reinterpret_cast<const char*>(&pattern),4);oracle.write(reinterpret_cast<const char*>(a.data()),16);oracle.write(reinterpret_cast<const char*>(b.data()),16);oracle.write(reinterpret_cast<const char*>(&influence),4);++checks;}
 std::cout<<"native_ik_solver_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
