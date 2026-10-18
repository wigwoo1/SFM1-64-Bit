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



static std::array<uint32_t,32>live{};static unsigned queryCase=0;
static void* __fastcall resolveLive(void*,void*,uint32_t handle){return queryCase&1&&handle==3?live.data():nullptr;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 std::vector<uint32_t>dm(13000);std::array<uint32_t,19>vt{};vt[18]=uint32_t(&resolveLive);dm[0]=uint32_t(vt.data());std::array<std::array<uint32_t,5>,2>buckets{};std::array<std::array<std::array<uint32_t,9>,3>,2>pending{};dm[0xc1ec/4]=uint32_t(buckets.data());dm[0xc1f8/4]=2;
 using Lookup=void*(__thiscall*)(void*,uint32_t,void**);auto lookup=reinterpret_cast<Lookup>(0x103596D0);unsigned cases=0;
 for(queryCase=0;queryCase<128;++queryCase){for(unsigned b=0;b<2;++b){buckets[b][0]=uint32_t(pending[b].data());buckets[b][3]=(queryCase>>(1+b*2))&3;for(unsigned i=0;i<3;++i){pending[b][i].fill(0);pending[b][i][4]=(i+b+(queryCase>>5))%5;}}
 for(unsigned handle=0;handle<8;++handle)for(unsigned wantOutput=0;wantOutput<2;++wantOutput){void*output=reinterpret_cast<void*>(0x1234);void*result=lookup(dm.data(),handle,wantOutput?&output:nullptr);uint32_t code=0xffffffffu,outCode=0xffffffffu;if(result==reinterpret_cast<char*>(live.data())+0x1c)code=0xfffffffeu;for(unsigned b=0;b<2;++b)for(unsigned i=0;i<3;++i){if(result==pending[b][i].data()+4)code=b*3+i;if(output==pending[b][i].data())outCode=b*3+i;}if(wantOutput&&output==nullptr)outCode=0xfffffffeu;if(!wantOutput&&output==reinterpret_cast<void*>(0x1234))outCode=0xfffffffdu;std::array<uint32_t,2>out{code,outCode};oracle.write(reinterpret_cast<const char*>(out.data()),8);++cases;}}
 std::cout<<"native_reference_record_lookup_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
