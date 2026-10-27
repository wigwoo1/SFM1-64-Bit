#define NOMINMAX
#include <windows.h>
#include <array>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#ifdef SFM64_NATIVE_FIND_CACHED
#include "material_typed_dispatch_native_fixture.h"
#else
#include "material_find_cache_key_candidate.h"
#include "material_dictionary_tree_candidate.h"
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_material_overrides_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t readWord(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static void put(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static std::string text(std::istream&i){auto n=readWord(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
#ifdef SFM64_NATIVE_FIND_CACHED
struct CachedFindFixture:MaterialTypedDispatchNativeFixture{
 std::array<uint8_t,0x1c8>system{};std::array<uint8_t,0x48>symbols{};
 std::array<std::array<uint8_t,0x88>,2>raw{};
 static uint32_t wordAt(const void*p,size_t at=0){uint32_t n;std::memcpy(&n,static_cast<const uint8_t*>(p)+at,4);return n;}
 static void set(void*p,size_t at,uint32_t n){std::memcpy(static_cast<uint8_t*>(p)+at,&n,4);}
 explicit CachedFindFixture(const char*p):MaterialTypedDispatchNativeFixture(p){using Symbols=void*(__thiscall*)(void*,int,int,bool);reinterpret_cast<Symbols>(0x10073600)(symbols.data(),0,32,false);word(0x13dcd4,reinterpret_cast<uintptr_t>(symbols.data()));image.base[0x13dcf4]=1;word(0x125600,0);word(0x13eaec,0);}
 uint8_t*tree(){return system.data()+0x188;}
 void prepare(const std::string&key){system.fill(0);using Ctor=void*(__thiscall*)(void*);reinterpret_cast<Ctor>(0x10012910)(tree());uint16_t sym=0xffff;reinterpret_cast<void*(__thiscall*)(void*,const char*)>(0x100738e0)(&sym,key.c_str());
  for(unsigned i=0;i<2;++i){raw[i].fill(0);set(raw[i].data(),0,0x10095d74);set(raw[i].data(),0x1c,123);std::array<uint8_t,8>entry{};set(entry.data(),0,uint32_t(reinterpret_cast<uintptr_t>(raw[i].data())));std::memcpy(entry.data()+4,&sym,2);entry[6]=uint8_t(i==0);uint16_t parent=0xffff;bool direction=false;reinterpret_cast<void(__thiscall*)(void*,void*,uint16_t*,bool*)>(0x10007e20)(tree(),entry.data(),&parent,&direction);auto index=reinterpret_cast<uint16_t(__thiscall*)(void*)>(0x10007f10)(tree());reinterpret_cast<void(__thiscall*)(void*,uint16_t,uint16_t,bool)>(0x10008c30)(tree(),index,parent,direction);uint16_t count;std::memcpy(&count,tree()+0x12,2);++count;std::memcpy(tree()+0x12,&count,2);auto nodes=reinterpret_cast<uint8_t*>(uintptr_t(wordAt(tree(),4)));std::memcpy(nodes+size_t(index)*16+8,entry.data(),7);}
 }
 std::string normalize(const std::string&s){std::vector<char>copy(s.begin(),s.end());copy.push_back(0);reinterpret_cast<char*(__cdecl*)(char*)>(0x1006e380)(copy.data());std::vector<char>target(copy.size());reinterpret_cast<void(__cdecl*)(const char*,char*,int)>(0x1006e7e0)(copy.data(),target.data(),int(target.size()));reinterpret_cast<void(__cdecl*)(char*,char)>(0x1006e880)(target.data(),'/');return target.data();}
 uint32_t find(const std::string&s,uint32_t option){using Find=void*(__thiscall*)(void*,const char*,const char*,bool,const char*);auto p=reinterpret_cast<Find>(0x10016f60)(system.data(),s.c_str(),option&1?"ignored group":nullptr,(option&2)!=0,option&4?"ignored prefix":nullptr);need(p==raw[1].data()+0x80,"whole cached lookup selects category0 and returns forwarding identity");for(auto&r:raw)need(wordAt(r.data(),0x1c)==123,"FindMaterial cache hit does not AddRef");return 2;}
 void finishCase(){reinterpret_cast<void(__thiscall*)(void*)>(0x10012840)(tree());reinterpret_cast<void(__thiscall*)(void*)>(0x100128b0)(tree()+0x1c);image.verify();}
 void finishAll(){reinterpret_cast<void(__thiscall*)(void*)>(0x10073580)(symbols.data());word(0x13dcd4,0);finish();}
};
static LONG CALLBACK faultFind(EXCEPTION_POINTERS*p){std::cerr<<"private lookup fault="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" at "<<p->ExceptionRecord->ExceptionAddress<<'\n';return EXCEPTION_CONTINUE_SEARCH;}
#else
static int benchmark(const char*path,const char*destination){need(!std::ifstream(destination).good(),"fresh benchmark output");auto graph=loadGraph(path);std::ofstream out(destination,std::ios::binary);need(bool(out),"benchmark output");uint32_t materials=0,owners=0;for(unsigned i=0;i<graph.elements.size();++i){const auto&e=graph.elements[i];if(graph.strings.at(e.type)=="DmeMaterial"){const auto*a=namedAttribute(graph,e,"mtlName");need(a&&a->type==5&&a->values.size()==1,"material name shape");std::string key;need(materialFindCacheKeyCandidate(a->values[0].text,key),"benchmark name key");out<<"M\t"<<i<<'\t'<<a->values[0].text<<'\t'<<key<<'\n';try{normalizeSessionMaterialNameCandidate(a->values[0].text);}catch(const std::runtime_error&error){out<<"R\t"<<i<<'\t'<<error.what()<<'\n';}++materials;}if(graph.strings.at(e.type)=="DmeGameModel"){const auto*a=namedAttribute(graph,e,"materials");if(!a)continue;need(a->type==15,"model material array");for(size_t j=0;j<a->values.size();++j){out<<"O\t"<<i<<'\t'<<j<<'\t'<<a->values[j].reference<<'\n';++owners;}}}out.flush();need(bool(out),"benchmark output complete");std::cout<<"PASS benchmark_materials="<<materials<<" owner_positions="<<owners<<'\n';return 0;}
static int safetyFind(){std::string key="preserve";need(!materialFindCacheKeyCandidate(std::string(4096,'x'),key)&&key=="preserve","oversized key unchanged");need(materialFindCacheKeyCandidate(std::string("A.VMT\0tail",10),key)&&key=="a","C string boundary");need(materialFindCacheKeyCandidate(".Hidden",key)&&key==".hidden","leading dot preserved");need(materialFindCacheKeyCandidate("Dir/.Hidden",key)&&key=="dir/","nonleading extension dot");need(materialFindCacheKeyCandidate("A\\B.PNG",key)&&key=="a/b","general extension removal");std::cout<<"PASS cached_find_safety_checks=5\n";return 0;}
#endif
int main(int argc,char**argv){try{
#ifdef SFM64_NATIVE_FIND_CACHED
 static_assert(sizeof(void*)==4,"private x86 reference");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&faultFind);
#else
 static_assert(sizeof(void*)==8,"owned x64");if(argc==2&&std::string(argv[1])=="--self-test")return safetyFind();if(argc==4&&std::string(argv[1])=="--benchmark")return benchmark(argv[2],argv[3]);
#endif
 need(argc==4,"image-or-absent query fresh-output");need(!std::ifstream(argv[3]).good(),"fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"query IO");
#ifdef SFM64_NATIVE_FIND_CACHED
 CachedFindFixture f(argv[1]);
#else
 need(!std::ifstream(argv[1]).good(),"owned engine must be absent");
#endif
 const auto cases=readWord(in);need(cases&&cases<20000,"case extent");put(out,cases);
 for(unsigned i=0;i<cases;++i){const auto input=text(in),expected=text(in);const auto option=readWord(in);need(option<8,"lookup options");std::string key;
#ifdef SFM64_NATIVE_FIND_CACHED
  key=f.normalize(input);f.prepare(expected);auto id=f.find(input,option);f.finishCase();
#else
  need(materialFindCacheKeyCandidate(input,key),"owned key");MaterialDictionaryTreeCandidate tree;MaterialDictionaryTreeCandidate::Index index=0;constexpr uintptr_t high=0x6244000000000000ull;need(tree.insert({high+1,7,true},index)&&tree.insert({high+2,7,false},index),"owned categories");const auto found=key==expected?tree.find(7,false):MaterialDictionaryTreeCandidate::none;need(found!=MaterialDictionaryTreeCandidate::none&&tree.slot(found)->entry.material==high+2,"owned 64bit cached identity");const uint32_t id=2;
#endif
  text(out,key);put(out,id);put(out,123);
 }
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output complete");
#ifdef SFM64_NATIVE_FIND_CACHED
 f.finishAll();std::cout<<"allocations="<<f.allocations<<" releases="<<f.releases<<'\n';
#endif
 std::cout<<"PASS cached_find_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
