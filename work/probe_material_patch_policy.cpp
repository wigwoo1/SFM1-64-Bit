#include "source_material_patch_policy_candidate.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#ifdef NATIVE_MATERIAL_PATCH_POLICY
#define NOMINMAX
#include <windows.h>
#endif
template<class T>static T read(std::istream&in){T v;if(!in.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("patch fixture truncated");return v;}
static std::string text(std::istream&in){auto n=read<uint32_t>(in);if(n>1u<<20)throw std::runtime_error("patch fixture string extent");std::string value(n,'\0');if(!in.read(value.data(),n))throw std::runtime_error("patch fixture string truncated");return value;}
template<class T>static void put(std::ostream&out,T v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static SourceMaterialPatchNodeCandidate readNode(std::istream&in,size_t depth,size_t&total){if(depth>16||++total>65536)throw std::runtime_error("patch fixture tree extent");SourceMaterialPatchNodeCandidate n;n.key=text(in);n.kind=read<uint32_t>(in);if(n.kind==1)n.text=text(in);else if(n.kind==2||n.kind==3)n.bits=read<uint32_t>(in);else if(n.kind==4)n.pointer=read<uint64_t>(in);else if(n.kind)throw std::runtime_error("patch fixture type");auto children=read<uint32_t>(in);if(children>65536)throw std::runtime_error("patch fixture child count");for(uint32_t i=0;i<children;++i)n.children.push_back(readNode(in,depth+1,total));return n;}
static void writeNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);if(n.kind==1)putText(out,n.text);else if(n.kind==2||n.kind==3)put(out,n.bits);else if(n.kind==4)put(out,n.pointer);put(out,uint32_t(n.children.size()));for(const auto&child:n.children)writeNode(out,child);}
#ifdef NATIVE_MATERIAL_PATCH_POLICY
struct NativeMaterialPatchFixture {
 HMODULE tier=nullptr,standard=nullptr,module=nullptr;
 std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>code;
 explicit NativeMaterialPatchFixture(const char*directory){std::string dir=directory;if(!SetDllDirectoryA(dir.c_str()))throw std::runtime_error("DLL directory");tier=LoadLibraryExA((dir+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);standard=LoadLibraryExA((dir+"\\vstdlib.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);module=LoadLibraryExA((dir+"\\materialsystem.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!tier||!standard||!module)throw std::runtime_error("normal material library initialization");const auto base=reinterpret_cast<const uint8_t*>(module);auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)code.push_back({base+s.VirtualAddress,std::vector<uint8_t>(base+s.VirtualAddress,base+s.VirtualAddress+s.Misc.VirtualSize)});}}
 uintptr_t at(size_t offset)const{return uintptr_t(module)+offset;}
 void verify()const{for(const auto&s:code)if(std::memcmp(s.first,s.second.data(),s.second.size()))throw std::runtime_error("original material code changed");}
 using Construct=void*(__thiscall*)(void*,const char*);using Allocate=void*(__cdecl*)(uint32_t);using Append=void(__thiscall*)(void*,void*);using Destroy=void(__thiscall*)(void*);
 void*build(const SourceMaterialPatchNodeCandidate&n){auto p=reinterpret_cast<Allocate>(at(0x709f0))(36);if(!p)throw std::runtime_error("native KeyValues allocation");reinterpret_cast<Construct>(at(0x70a10))(p,n.key.c_str());if(n.kind==1){using Set=void(__thiscall*)(void*,const char*,const char*);reinterpret_cast<Set>(at(0x70db0))(p,nullptr,n.text.c_str());}else if(n.kind==2){using Set=void(__thiscall*)(void*,const char*,uint32_t);reinterpret_cast<Set>(at(0x70de0))(p,nullptr,n.bits);}else if(n.kind==3){using Set=void(__thiscall*)(void*,const char*,float);float value;std::memcpy(&value,&n.bits,4);reinterpret_cast<Set>(at(0x70e00))(p,nullptr,value);}else if(n.kind==4){if(n.pointer>UINT32_MAX)throw std::runtime_error("native pointer fixture outside32bits");using Set=void(__thiscall*)(void*,const char*,void*);reinterpret_cast<Set>(at(0x70e30))(p,nullptr,reinterpret_cast<void*>(uintptr_t(n.pointer)));}for(const auto&child:n.children)reinterpret_cast<Append>(at(0x70c00))(p,build(child));return p;}
 SourceMaterialPatchNodeCandidate extract(void*p,size_t depth=0)const{if(depth>32)throw std::runtime_error("native tree cycle/depth");SourceMaterialPatchNodeCandidate n;using Name=const char*(__thiscall*)(void*);n.key=reinterpret_cast<Name>(at(0x705f0))(p);auto b=static_cast<const uint8_t*>(p);n.kind=b[16];if(n.kind==1){const char*str;std::memcpy(&str,b+4,4);n.text=str?str:"";}else if(n.kind==2||n.kind==3)std::memcpy(&n.bits,b+12,4);else if(n.kind==4){uint32_t token;std::memcpy(&token,b+12,4);n.pointer=token;}else if(n.kind)throw std::runtime_error("unexpected native result type");void*child;std::memcpy(&child,b+24,4);while(child){n.children.push_back(extract(child,depth+1));std::memcpy(&child,static_cast<uint8_t*>(child)+20,4);}return n;}
 void destroy(void*p){reinterpret_cast<Destroy>(at(0x71280))(p);}
 ~NativeMaterialPatchFixture(){if(module)FreeLibrary(module);if(standard)FreeLibrary(standard);if(tier)FreeLibrary(tier);SetDllDirectoryA(nullptr);}
};
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"material KV fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void apply(uintptr_t function,void*base,void*patch){__asm{mov eax,patch
 push base
 call function
 add esp,4
}}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("patch fixture files");auto count=read<uint32_t>(in);if(!count||count>100000)throw std::runtime_error("patch case count");put(out,count);
#ifdef NATIVE_MATERIAL_PATCH_POLICY
 static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativeMaterialPatchFixture native(argv[1]);
#else
 static_assert(sizeof(void*)==8,"owned x64");
#endif
 for(uint32_t i=0;i<count;++i){size_t total=0;const auto base=readNode(in,0,total),patch=readNode(in,0,total);size_t checked=0;sourceMaterialPatchValidateCandidate(base,0,checked);sourceMaterialPatchValidateCandidate(patch,0,checked);
#ifdef NATIVE_MATERIAL_PATCH_POLICY
  auto target=native.build(base),source=native.build(patch);apply(native.at(0x4f80),target,source);const auto result=native.extract(target);std::ostringstream before,after;writeNode(before,patch);writeNode(after,native.extract(source));if(before.str()!=after.str())throw std::runtime_error("native input patch mutated");writeNode(out,result);native.destroy(target);native.destroy(source);
#else
  writeNode(out,sourceMaterialPatchPolicyCandidate(base,patch));
#endif
 }
 if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing patch fixture");out.flush();if(!out)throw std::runtime_error("patch output failure");
#ifdef NATIVE_MATERIAL_PATCH_POLICY
 native.verify();
#endif
 std::cout<<"PASS material patch policy cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
