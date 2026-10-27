#ifdef SFM64_NATIVE_FIND_CACHED
#define main preservedCachedFindMain
#include "probe_material_find_cached.cpp"
#undef main
#else
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <stdexcept>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t readWord(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static void put(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static std::string text(std::istream&i){auto n=readWord(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
#endif
#include <deque>
#include "material_find_file_request_candidate.h"
#ifdef SFM64_NATIVE_FIND_CACHED
struct UncachedFindFixture:CachedFindFixture{
 std::array<uintptr_t,146>systemMethods{};std::array<uintptr_t,88>fsPrimary{};std::array<uintptr_t,8>fsBase{};std::array<uintptr_t,10>kvMethods{};std::array<uintptr_t,94>errorMethods{};std::array<uintptr_t,6>hardwareMethods{};
 std::array<uintptr_t,2>filesystem{};uintptr_t kvObject=0,errorObject=0,hardwareObject=0;
 std::map<std::string,int>ci;std::deque<std::string>cs;
 std::array<std::array<uint8_t,0xa8>,4>materials{};std::array<std::array<uint8_t,0x2c0>,4>states{};
 std::vector<std::array<std::string,3>>requests;std::string contents="UnlitGeneric { \"$basetexture\" \"fixture/white\" }";
 unsigned reads=0,closes=0,buffers=0,bufferFrees=0,depth=0;bool available=true;
 static UncachedFindFixture&self(){return *static_cast<UncachedFindFixture*>(MaterialConstructionNativeFixture::active);}
 static void*__cdecl kv(){return &self().kvObject;}
 static void*__fastcall kvAlloc(void*,void*,int n){need(n==36,"KV allocation extent");return allocate(nullptr,nullptr,36);}
 static void __fastcall safeFree(void*,void*,void*p){if(p)free(nullptr,nullptr,p);}
 static int __fastcall ciSymbol(void*,void*,const char*name,bool create){auto&f=self();std::string key=name?name:"";for(char&c:key)if(c>='A'&&c<='Z')c=char(c+32);auto i=f.ci.find(key);if(i!=f.ci.end())return i->second;if(!create)return -1;int id=int(f.ci.size()+1);f.ci.emplace(key,id);return id;}
 static int __fastcall csSymbol(void*,void*,int*out,const char*name,bool create){if(!name){*out=-1;return -1;}auto&f=self();*out=ciSymbol(nullptr,nullptr,name,create);for(size_t i=0;i<f.cs.size();++i)if(f.cs[i]==name)return int(i);if(!create)return -1;f.cs.emplace_back(name);return int(f.cs.size()-1);}
 static const char*__fastcall csName(void*,void*,int id){auto&f=self();need(id>=0&&size_t(id)<f.cs.size(),"KV string symbol");return f.cs[size_t(id)].c_str();}
 static void*__fastcall open(void*,void*,const char*name,const char*mode,const char*pathID){auto&f=self();need(name&&mode&&!std::strcmp(mode,"rb")&&(!pathID||!std::strcmp(pathID,"GAME")),"native open mode/pathID");f.requests.push_back({name,mode,pathID?pathID:""});return f.available?reinterpret_cast<void*>(1):nullptr;}
 static void __fastcall close(void*,void*,void*h){need(h==reinterpret_cast<void*>(1),"close handle");++self().closes;}
 static uint32_t __fastcall size(void*,void*,void*h){need(h==reinterpret_cast<void*>(1),"size handle");return uint32_t(self().contents.size());}
 static bool __fastcall alignment(void*,void*,void*h,uint32_t*,void*,void*){need(h==reinterpret_cast<void*>(1),"alignment handle");return false;}
 static void*__fastcall buffer(void*,void*,void*h,uint32_t n,uint32_t flags){need(h==reinterpret_cast<void*>(1)&&!flags&&n>=self().contents.size()+2,"buffer request");++self().buffers;return allocate(nullptr,nullptr,n);}
 static int __fastcall read(void*,void*,void*out,int capacity,int n,void*h){auto&f=self();need(h==reinterpret_cast<void*>(1)&&n==int(f.contents.size())&&capacity>=n+2,"read extent");std::memcpy(out,f.contents.data(),n);++f.reads;return n;}
 static void __fastcall freeBuffer(void*,void*,void*p){++self().bufferFrees;safeFree(nullptr,nullptr,p);}
 static uint32_t __fastcall acquire(void*,void*){need(self().depth++==0,"context scope");return 0x1234;}
 static void __fastcall release(void*,void*,uint32_t token){need(self().depth==1&&token==0x1234,"context release");--self().depth;}
 static uint32_t __fastcall mode(void*,void*){return 0;}
 static void __fastcall unbind(void*,void*,void*){}
 static bool __fastcall tools(void*,void*){return false;}
 static void*__fastcall fallback(void*,void*){return reinterpret_cast<void*>(0x12345678);}
 template<size_t I>static void __fastcall unexpectedFS(void*,void*){std::cerr<<"Unexpected filesystem slot "<<I<<'\n';ExitProcess(98);}
 template<size_t...I>void initializeFS(std::index_sequence<I...>){fsPrimary={reinterpret_cast<uintptr_t>(&unexpectedFS<I>)...};}
 void kernelImports(const char*path){std::ifstream in(path,std::ios::binary);std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(in)),{});auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);auto imp=reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(image.base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);for(;imp->Name;++imp)if(!_stricmp(reinterpret_cast<const char*>(image.base+imp->Name),"KERNEL32.dll")){auto original=reinterpret_cast<const uint32_t*>(image.base+imp->OriginalFirstThunk);auto target=reinterpret_cast<uint32_t*>(image.base+imp->FirstThunk);for(size_t i=0;original[i];++i)if(target[i]==original[i]){need(!(original[i]&IMAGE_ORDINAL_FLAG32),"kernel ordinal");auto p=GetProcAddress(GetModuleHandleA("KERNEL32.dll"),reinterpret_cast<const char*>(image.base+original[i]+2));need(p!=nullptr,"kernel import");target[i]=uint32_t(reinterpret_cast<uintptr_t>(p));}}}
 explicit UncachedFindFixture(const char*p):CachedFindFixture(p){kernelImports(p);allocators[5]=reinterpret_cast<uintptr_t>(&safeFree);kvMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));kvMethods[1]=reinterpret_cast<uintptr_t>(&kvAlloc);kvMethods[2]=reinterpret_cast<uintptr_t>(&safeFree);kvMethods[3]=reinterpret_cast<uintptr_t>(&ciSymbol);kvMethods[4]=reinterpret_cast<uintptr_t>(&csName);kvMethods[9]=reinterpret_cast<uintptr_t>(&csSymbol);kvObject=reinterpret_cast<uintptr_t>(kvMethods.data());word(0x95208,reinterpret_cast<uintptr_t>(&kv));reinterpret_cast<void(__cdecl*)()>(0x10094370)();reinterpret_cast<void(__cdecl*)()>(0x100943d0)();
  initializeFS(std::make_index_sequence<88>{});fsBase.fill(reinterpret_cast<uintptr_t>(&unexpected));fsBase[2]=reinterpret_cast<uintptr_t>(&open);fsBase[3]=reinterpret_cast<uintptr_t>(&close);fsBase[7]=reinterpret_cast<uintptr_t>(&size);fsPrimary[85]=reinterpret_cast<uintptr_t>(&alignment);fsPrimary[86]=reinterpret_cast<uintptr_t>(&buffer);fsPrimary[72]=reinterpret_cast<uintptr_t>(&read);fsPrimary[87]=reinterpret_cast<uintptr_t>(&freeBuffer);filesystem={reinterpret_cast<uintptr_t>(fsPrimary.data()),reinterpret_cast<uintptr_t>(fsBase.data())};word(0x1255c4,reinterpret_cast<uintptr_t>(filesystem.data()));
  systemMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));systemMethods[106]=reinterpret_cast<uintptr_t>(&acquire);systemMethods[107]=reinterpret_cast<uintptr_t>(&release);systemMethods[142]=reinterpret_cast<uintptr_t>(&mode);systemMethods[145]=reinterpret_cast<uintptr_t>(&unbind);word(0xfc7b0,reinterpret_cast<uintptr_t>(system.data()));word(0x1255cc,reinterpret_cast<uintptr_t>(system.data()));errorMethods[93]=reinterpret_cast<uintptr_t>(&fallback);errorObject=reinterpret_cast<uintptr_t>(errorMethods.data());word(0x1059ec,reinterpret_cast<uintptr_t>(&errorObject));hardwareMethods[5]=reinterpret_cast<uintptr_t>(&tools);hardwareObject=reinterpret_cast<uintptr_t>(hardwareMethods.data());word(0x11b0a0,reinterpret_cast<uintptr_t>(&hardwareObject));
 }
 void begin(){system.fill(0);set(system.data(),0,uint32_t(reinterpret_cast<uintptr_t>(systemMethods.data())));reinterpret_cast<void*(__thiscall*)(void*)>(0x10012910)(tree());for(unsigned i=0;i<4;++i){materials[i].fill(0);states[i].fill(0);if(i+1<4){set(materials[i].data(),0,uint32_t(reinterpret_cast<uintptr_t>(materials[i+1].data())));set(states[i].data(),0,uint32_t(reinterpret_cast<uintptr_t>(states[i+1].data())));}}
  for(auto offset:{0x1058c8u,0x105868u})std::memset(image.base+offset,0,0x28);word(0x1058c8,0xa8);word(0x1058d4,reinterpret_cast<uintptr_t>(materials[0].data()));word(0x105868,0x2c0);word(0x105874,reinterpret_cast<uintptr_t>(states[0].data()));requests.clear();reads=closes=buffers=bufferFrees=0;need(!depth,"initial context");
 }
 uint32_t lookup(const std::string&name){using Find=void*(__thiscall*)(void*,const char*,const char*,bool,const char*);auto p=reinterpret_cast<Find>(0x10016f60)(system.data(),name.c_str(),"fixture group",false,nullptr);if(p==reinterpret_cast<void*>(0x12345678))return 0;for(unsigned i=0;i<4;++i)if(p==materials[i].data()+0x80){need(wordAt(materials[i].data(),0)==0x10095d74&&wordAt(materials[i].data(),0x1c)==0,"actual new raw/zero references");return i+1;}throw std::runtime_error("native lookup result");}
 void finishRequest(std::ostream&o){put(o,uint32_t(requests.size()));for(const auto&r:requests)for(const auto&s:r)text(o,s);for(auto n:{reads,closes,buffers,bufferFrees})put(o,n);for(auto&record:materials)if(wordAt(record.data())==0x10095d74){const auto name=reinterpret_cast<const char*(__thiscall*)(void*)>(0x10004190)(record.data());text(o,name);reinterpret_cast<void*(__thiscall*)(void*,unsigned)>(0x10006d20)(record.data(),1);}put(o,UINT32_MAX);finishCase();need(!depth&&buffers==bufferFrees,"request/context/heap cleanup");need(!wordAt(image.base+0x1058c8,0x10)&&!wordAt(image.base+0x105868,0x10),"real raw/state pools empty after destruction");}
};
// Characterize the original formatter in fixture-owned storage with a readable
// guard. Never read a C string from a full native260-byte destination.
static int formatBoundary(const char*image,const char*destination){
 need(!std::ifstream(destination).good(),"fresh format output");UncachedFindFixture f(image);std::ofstream out(destination,std::ios::binary);need(bool(out),"format output");put(out,18);
 for(bool brace:{false,true})for(unsigned size:{244u,245u,246u,247u,254u,255u,256u,257u,4095u}){
  std::string name(size,'a');if(brace)name[0]='{';std::array<char,272>storage;storage.fill('Z');
  using Format=int(__cdecl*)(char*,int,const char*,...);
  const auto returned=reinterpret_cast<Format>(0x1006e590)(storage.data(),260,reinterpret_cast<const char*>(brace?0x10095fc4:0x10095fcc),name.c_str());
  put(out,size);put(out,brace?1:0);put(out,uint32_t(returned));out.write(storage.data(),storage.size());f.image.verify();
 }
 out.flush();need(bool(out),"format output complete");f.finishAll();std::cout<<"PASS native_format_boundary_cases=18 allocations="<<f.allocations<<" releases="<<f.releases<<'\n';return 0;
}
#endif
int main(int argc,char**argv){try{
#ifdef SFM64_NATIVE_FIND_CACHED
 if(argc==4&&std::string(argv[2])=="--format-boundary"){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&faultFind);return formatBoundary(argv[1],argv[3]);}
#endif
#ifndef SFM64_NATIVE_FIND_CACHED
 if(argc==2&&std::string(argv[1])=="--self-test"){
  MaterialFindFileRequestCandidate out;out.key="keep-key";out.file="keep-file";out.pathID="keep-id";out.mode="keep-mode";
  auto preserved=[&](){return out.key=="keep-key"&&out.file=="keep-file"&&out.pathID=="keep-id"&&out.mode=="keep-mode";};
  need(!materialFindFileRequestCandidate(std::string(4096,'x'),out)&&preserved(),"oversized input unchanged");
  need(!materialFindFileRequestCandidate(std::string(246,'x'),out)&&preserved(),"exact-full native buffer refused unchanged");
  need(!materialFindFileRequestCandidate(std::string(247,'x'),out)&&preserved(),"native truncation refused unchanged");
  need(!materialFindFileRequestCandidate("{"+std::string(255,'x'),out)&&preserved(),"brace exact-full buffer refused unchanged");
  need(materialFindFileRequestCandidate(std::string(245,'A'),out)&&out.file.size()==259&&out.key==std::string(245,'a'),"last safely terminated ordinary filename");
  need(materialFindFileRequestCandidate("{"+std::string(254,'A'),out)&&out.file.size()==259&&out.pathID.empty(),"last safely terminated brace filename");
  need(materialFindFileRequestCandidate("/MODELS\\A.VMT",out)&&out.key=="/models/a"&&out.file=="materials//models/a.vmt"&&out.pathID=="GAME","leading slash resource identity");
  need(materialFindFileRequestCandidate(std::string("A.VMT\0tail",10),out)&&out.key=="a"&&out.file=="materials/a.vmt","NUL boundary");
  need(materialFindFileRequestCandidate("{F/X}.VMT",out)&&out.key=="{f/x}"&&out.file=="{f/x}.vmt"&&out.pathID.empty(),"brace null path-ID");
  need(materialFindFileRequestCandidate("",out)&&out.key.empty()&&out.file=="materials/.vmt"&&out.pathID=="GAME","empty lookup request");
  std::cout<<"PASS uncached_find_safety_checks=10\n";return 0;
 }
#endif
 need(argc==4,"image query fresh-output");need(!std::ifstream(argv[3]).good(),"fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");
#ifdef SFM64_NATIVE_FIND_CACHED
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&faultFind);UncachedFindFixture f(argv[1]);
#else
 need(!std::ifstream(argv[1]).good(),"owned engine absent");
#endif
 const auto cases=readWord(in);need(cases&&cases<20000,"case extent");put(out,cases);for(unsigned i=0;i<cases;++i){const auto name=text(in),second=text(in);const auto available=readWord(in);need(available<2,"availability");
  // Admission only; the reference's actual request/result comes from original
  // instructions and explicit filesystem callbacks, never from this helper.
  MaterialFindFileRequestCandidate admission;need(materialFindFileRequestCandidate(name,admission)&&materialFindFileRequestCandidate(second,admission),"safe terminated request scope");
#ifdef SFM64_NATIVE_FIND_CACHED
  f.available=available!=0;f.begin();put(out,f.lookup(name));put(out,f.lookup(second));f.finishRequest(out);
#else
  std::map<std::string,uint32_t>cache;std::vector<MaterialFindFileRequestCandidate>requests;std::vector<std::string>created;
  for(const auto&input:{name,second}){MaterialFindFileRequestCandidate request;need(materialFindFileRequestCandidate(input,request),"owned request");auto old=cache.find(request.key);if(old!=cache.end()){put(out,old->second);continue;}requests.push_back(request);if(!available){put(out,0);continue;}std::string stored;need(materialFindCacheKeyCandidate(request.key+".vmt",stored),"created material name");created.push_back(stored);const auto token=uint32_t(created.size());cache.emplace(stored,token);put(out,token);}
  put(out,uint32_t(requests.size()));for(const auto&r:requests){text(out,r.file);text(out,r.mode);text(out,r.pathID);}for(unsigned j=0;j<4;++j)put(out,available?uint32_t(requests.size()):0);for(const auto&s:created)text(out,s);put(out,UINT32_MAX);
#endif
 }need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output");
#ifdef SFM64_NATIVE_FIND_CACHED
 f.finishAll();std::cout<<"allocations="<<f.allocations<<" releases="<<f.releases<<'\n';
#endif
 std::cout<<"PASS uncached_find_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
