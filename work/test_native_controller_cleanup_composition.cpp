#include "controller_autosave_cleanup_candidate.h"
#include "controller_path_prefix_candidate.h"
#include "native_buffer_composition_candidate.h"
#include <string>
#include <vector>
#include <iostream>
struct Services{
 char cache[260]={};std::vector<std::string> calls;std::string contents=" \tC:\\fixture\\session.dmx\n";
 NativeBufferCompositionCandidate* active=nullptr;bool success=true;unsigned frees=0;
 char* pathCache(){return cache;}
 void mainInterfaceSlot16(const char* id,std::nullptr_t,char* out,unsigned n){assert(std::string(id)=="GAMEROOT" && n==260);calls.push_back("query16");strcpy_s(out,n,"F:/SFM/game/;G:/fallback/");}
 char* findCharacter(char* p,char c){return std::strchr(p,c);}
 char* locatorPathService(){return controllerPathPrefixCandidate(*this);}
 void formatLocatorPath(std::uint8_t* dst,unsigned cap,const char* prefix,const char* suffix){sprintf_s(reinterpret_cast<char*>(dst),cap,"%s%s",prefix,suffix);}
 NativeBufferCompositionCandidate createBuffer(unsigned grow,unsigned cap,unsigned flags){return NativeBufferCompositionCandidate(grow,cap,flags);}
 bool readSubinterfaceSlot14(const std::uint8_t* p,std::nullptr_t,NativeBufferCompositionCandidate& b,unsigned n,unsigned x,unsigned y){
 assert(n==260 && !x && !y);calls.push_back("read14:"+std::string(reinterpret_cast<const char*>(p)));active=&b;
 if(!success)return false;
 // Simulated service fill; the filesystem implementation is not recovered.
 const auto count=static_cast<unsigned>(contents.size());assert(checkBufferWriteCandidate(count,b));std::memmove(b.sourceData(),contents.data(),count);b.setPutPosition(count);b.terminateBuffer();return true;
 }
 void mainInterfaceSlot18(const std::uint8_t* p,std::nullptr_t){calls.push_back("slot18:"+std::string(reinterpret_cast<const char*>(p)));}
 void freeBufferStorage(std::uint8_t* p){assert(active && active->sourceData()==p);std::free(p);active->setStorage(nullptr);active=nullptr;++frees;calls.push_back("free");}
};
int main(){static_assert(sizeof(void*)==8);
 Services a;cleanupControllerAutosaveCandidate(a);assert((a.calls==std::vector<std::string>{"query16","read14:F:\\SFM\\game\\.sfm_autosave.loc","slot18:F:\\SFM\\game\\.sfm_autosave.loc","slot18:C:\\fixture\\session.dmx","free"}) && a.frees==1);
 a.calls.clear();cleanupControllerAutosaveCandidate(a);assert(a.calls.size()==4 && a.frees==2);
 Services b;b.success=false;cleanupControllerAutosaveCandidate(b);assert(b.calls.size()==3 && b.calls.back()=="free" && b.frees==1);
 Services c;c.contents=std::string(4097,'x');cleanupControllerAutosaveCandidate(c);assert(c.calls[3]=="slot18:"+std::string(259,'x') && c.frees==1);
 Services d;d.contents=" \t\n";cleanupControllerAutosaveCandidate(d);assert(d.calls[3]=="slot18:" && d.frees==1);
 Services e;e.contents="C:\\folder with spaces\\session.dmx";cleanupControllerAutosaveCandidate(e);assert(e.calls[3]=="slot18:C:\\folder");
 std::cout<<"native x64 controller composition: GAMEROOT cache, suffix, real buffer growth/extraction/release, failure, truncation and whitespace semantics passed; no disk operations\n";
}
