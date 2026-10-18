#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
// Reference-only metadata reader. No Connect/Init/Draw calls or fake services.
struct OriginalShaderRegistry {
 HMODULE tier=nullptr,shader=nullptr;
 uintptr_t base=0;size_t span=0;
 std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>code;
 explicit OriginalShaderRegistry(const std::string&dir){
  if(!SetDllDirectoryA(dir.c_str()))throw std::runtime_error("DLL directory");
  tier=LoadLibraryExA((dir+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
  shader=LoadLibraryExA((dir+"\\stdshader_dx9.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
  if(!tier||!shader)throw std::runtime_error("normal original shader library initialization");
  base=reinterpret_cast<uintptr_t>(shader);const auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  const auto*nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);span=nt->OptionalHeader.SizeOfImage;
  for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE){const auto*b=reinterpret_cast<const uint8_t*>(base+s.VirtualAddress);code.push_back({b,{b,b+s.Misc.VirtualSize}});}}
 }
 uint32_t rva(const void*p,size_t n=1)const{const auto v=reinterpret_cast<uintptr_t>(p);if(v<base||v-base>=span||n>span-(v-base))throw std::runtime_error("pointer outside original shader image");return static_cast<uint32_t>(v-base);}
 std::string string(const char*p)const{if(!p)return {};rva(p);size_t n=0;while(n<4096){rva(p+n);if(!p[n])return {p,n};if(static_cast<unsigned char>(p[n])>127)throw std::runtime_error("non-ASCII shader metadata");++n;}throw std::runtime_error("shader metadata string bound");}
 void verify()const{for(const auto&s:code)if(std::memcmp(s.first,s.second.data(),s.second.size()))throw std::runtime_error("original shader executable section changed");}
 ~OriginalShaderRegistry(){if(shader)FreeLibrary(shader);if(tier)FreeLibrary(tier);SetDllDirectoryA(nullptr);}
};
template<class T>static void put(std::ostream&out,const T&v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));}
static void putString(std::ostream&out,const std::string&s){put(out,static_cast<uint32_t>(s.size()));out.write(s.data(),static_cast<std::streamsize>(s.size()));}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"Original reference metadata reader must be x86");
 if(argc!=3)throw std::runtime_error("copied-library-directory output-file required");OriginalShaderRegistry native(argv[1]);
 using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(native.shader,"CreateInterface"));if(!factory)throw std::runtime_error("shader factory export");
 int status=-1;void*registry=factory("ShaderDLL004",&status);if(!registry||status)throw std::runtime_error("real ShaderDLL004 unavailable");
 auto table=*static_cast<void***>(registry);if(native.rva(table)!=0x9eae4||native.rva(table[2])!=0x65c40||native.rva(table[3])!=0x65c50)throw std::runtime_error("registry ABI identity");
 using Count=int(__thiscall*)(void*);using Get=void*(__thiscall*)(void*,int);using Name=const char*(__thiscall*)(void*);
 const auto count=reinterpret_cast<Count>(table[2])(registry);if(count<1||count>4096)throw std::runtime_error("shader registry count bound");
 std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("metadata output stream");put(out,uint32_t(count));size_t parameters=0;
 for(int i=0;i<count;++i){void*shader=reinterpret_cast<Get>(table[3])(registry,i);native.rva(shader);auto methods=*static_cast<void***>(shader);native.rva(methods,16);
  for(int j=0;j<4;++j)native.rva(methods[j]);put(out,native.rva(shader));put(out,native.rva(methods));for(int j=0;j<4;++j)put(out,native.rva(methods[j]));
  const auto name=native.string(reinterpret_cast<Name>(methods[0])(shader));if(name.empty())throw std::runtime_error("empty shader name");putString(out,name);
  // Only execute statically proven literal-return fallbacks. Conditional/hardware
  // methods are reported unverified and are never called with fabricated state.
  const auto*f=static_cast<const uint8_t*>(methods[1]);native.rva(f,8);
  const bool constantString=f[0]==0xb8&&f[5]==0xc2&&f[6]==4&&f[7]==0;
  const bool constantNull=f[0]==0x33&&f[1]==0xc0&&f[2]==0xc2&&f[3]==4&&f[4]==0;
  put(out,uint32_t(constantString||constantNull));
  if(constantString||constantNull){using Fallback=const char*(__thiscall*)(void*,void*);const char*value=reinterpret_cast<Fallback>(methods[1])(shader,nullptr);put(out,uint32_t(value!=nullptr));if(value)putString(out,native.string(value));}
  const auto n=reinterpret_cast<Count>(methods[2])(shader);if(n<13||n>2048)throw std::runtime_error("shader parameter count bound");put(out,uint32_t(n));
  for(int k=0;k<n;++k){auto*info=static_cast<const uint32_t*>(reinterpret_cast<Get>(methods[3])(shader,k));put(out,native.rva(info,20));
   putString(out,native.string(reinterpret_cast<const char*>(info[0])));putString(out,native.string(reinterpret_cast<const char*>(info[1])));put(out,info[2]);put(out,uint32_t(info[3]!=0));if(info[3])putString(out,native.string(reinterpret_cast<const char*>(info[3])));put(out,info[4]);++parameters;
  }
 }
 if(reinterpret_cast<Get>(table[3])(registry,-1)||reinterpret_cast<Get>(table[3])(registry,count))throw std::runtime_error("registry outside index did not refuse");
 if(reinterpret_cast<Count>(table[2])(registry)!=count)throw std::runtime_error("registry count changed");native.verify();out.flush();if(!out)throw std::runtime_error("metadata output write");
 std::cout<<"PASS normal original ShaderDLL004 metadata shaders="<<count<<" parameter_records="<<parameters<<" code_unchanged=1 invalid_indexes_refused=2\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
