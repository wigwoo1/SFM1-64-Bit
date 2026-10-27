// Unchanged private DmeMaterial factory, member attribute construction, actual
// string handler and copy context versus an owned x64 name/cache contract.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#ifdef SFM64_NATIVE_ELEMENT_BINDING
#include "native_private_world_image_fixture.h"
#else
#include "material_element_binding_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t get(std::istream&i){uint32_t v=0;need(bool(i.read(reinterpret_cast<char*>(&v),4)),"query word");return v;}
static void put(std::ostream&o,uint32_t v){o.write(reinterpret_cast<const char*>(&v),4);}
static const char*names[]={"","matA","matB","missing"};
static uint32_t nameId(const char*s){for(unsigned i=0;i<4;++i)if(!std::strcmp(s,names[i]))return i;throw std::runtime_error("name identity");}
static std::vector<std::array<uint32_t,2>>events;
static std::array<uint32_t,3>refs{};
static uint32_t lookupMode=0;
static uint32_t lookup(const char*s){const auto id=nameId(s);const auto token=lookupMode==1?1u:id==1?1u:id==2?2u:0u;events.push_back({0,token});return token;}
static void retain(uint32_t id){need(id&&id<3,"resource retain identity");++refs[id];events.push_back({1,id});}
static void releaseRef(uint32_t id){need(id&&id<3&&refs[id],"resource release balance");--refs[id];events.push_back({2,id});}
#ifdef SFM64_NATIVE_ELEMENT_BINDING
static uint32_t word(const void*p,size_t at=0){uint32_t v;std::memcpy(&v,static_cast<const uint8_t*>(p)+at,4);return v;}
static void field(void*p,size_t at,uintptr_t v){const auto w=uint32_t(v);std::memcpy(static_cast<uint8_t*>(p)+at,&w,4);}
static std::map<std::string,std::string>strings;
static const char*intern(const char*s){need(s!=nullptr,"nonnull canonical string service");return strings.emplace(s,s).first->second.c_str();}
static uint32_t*__fastcall internService(void*,void*,uint32_t*out,const char*s){*out=uint32_t(reinterpret_cast<uintptr_t>(intern(s)));return out;}
static bool __fastcall undo(void*,void*){return false;}
static bool __fastcall undoElement(void*,void*,void*){return false;}
static bool __fastcall suppressed(void*,void*){return true;}
static void __fastcall setUndo(void*,void*,bool b){need(!b,"disabled undo state");}
static int currentPhase=0;
static int __fastcall phase(void*,void*){return currentPhase;}
static unsigned enters=0,leaves=0;
static void __fastcall enter(void*,void*,const char*,int,const char*,int,int){++enters;}
static void __fastcall leave(void*,void*){++leaves;}
static HANDLE heap=nullptr;
static std::map<void*,size_t>allocations;
static void*__fastcall alloc(void*,void*,size_t n){need(n&&n<=16*1024*1024,"bounded heap service");auto p=HeapAlloc(heap,HEAP_ZERO_MEMORY,n);need(p!=nullptr,"heap allocation");allocations[p]=n;return p;}
static void*__fastcall reallocBlock(void*,void*,void*p,size_t n){if(!p)return alloc(nullptr,nullptr,n);need(allocations.count(p)&&n&&n<=16*1024*1024,"reallocation provenance");auto q=HeapReAlloc(heap,HEAP_ZERO_MEMORY,p,n);need(q!=nullptr,"heap realloc");allocations.erase(p);allocations[q]=n;return q;}
static void __fastcall freeBlock(void*,void*,void*p){if(!p)return;need(allocations.erase(p)==1&&HeapFree(heap,0,p),"free provenance");}
struct Mat{uintptr_t methods;uint32_t id;};
static std::array<Mat,2>mats{};
static void*__fastcall findMaterial(void*,void*,const char*s,const char*group,bool complain,const char*prefix){need(!group&&!complain&&!prefix,"FindMaterial contract");const auto id=lookup(s);return id?&mats[id-1]:nullptr;}
static void __fastcall add(void*p,void*){retain(static_cast<Mat*>(p)->id);}
static void __fastcall drop(void*p,void*){releaseRef(static_cast<Mat*>(p)->id);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){std::cerr<<"private exception "<<std::hex<<p->ExceptionRecord->ExceptionCode<<" at "<<p->ExceptionRecord->ExceptionAddress;if(p->ExceptionRecord->NumberParameters>1)std::cerr<<" target "<<reinterpret_cast<void*>(p->ExceptionRecord->ExceptionInformation[1]);std::cerr<<'\n';return EXCEPTION_CONTINUE_SEARCH;}
struct Backend{
 NativePrivateWorldImageFixture image;
 std::array<uint8_t,0xc1b0>dm{};std::array<uintptr_t,120>dmMethods{};
 std::array<uint8_t,0x80>queue{};std::array<uint32_t,16>queueStorage{};
 std::array<uintptr_t,9>phaseMethods{};uintptr_t phaseObject=0;
 std::array<uintptr_t,6>allocMethods{};uintptr_t allocator=0,allocatorPointer=0;
 std::array<uintptr_t,78>systemMethods{};uintptr_t system=0;
 std::array<uintptr_t,14>matMethods{};
 uintptr_t type5=0x10b37a14;
 std::array<std::array<uint8_t,0x68>,2>objects{};
 std::array<std::array<uint8_t,24>,8>attributes{};
 std::array<uint8_t,0x1c>factory{};
 std::array<uint8_t*,2>element{},extra{};
 explicit Backend(const char*path):image(path){
  heap=HeapCreate(0,0,0);need(heap!=nullptr,"private heap");
  dmMethods[50]=reinterpret_cast<uintptr_t>(&internService);dmMethods[60]=reinterpret_cast<uintptr_t>(&setUndo);dmMethods[61]=reinterpret_cast<uintptr_t>(&undo);dmMethods[62]=reinterpret_cast<uintptr_t>(&undoElement);dmMethods[119]=reinterpret_cast<uintptr_t>(&suppressed);field(dm.data(),0,reinterpret_cast<uintptr_t>(dmMethods.data()));
  data(0xe9a518,reinterpret_cast<uintptr_t>(dm.data()));data(0xe9a51c,reinterpret_cast<uintptr_t>(dm.data()));data(0xe9ccf4,reinterpret_cast<uintptr_t>(queue.data()));
  phaseMethods[8]=reinterpret_cast<uintptr_t>(&phase);phaseObject=reinterpret_cast<uintptr_t>(phaseMethods.data());data(0xe9ccf8,reinterpret_cast<uintptr_t>(&phaseObject));
  allocMethods[1]=reinterpret_cast<uintptr_t>(&alloc);allocMethods[3]=reinterpret_cast<uintptr_t>(&reallocBlock);allocMethods[5]=reinterpret_cast<uintptr_t>(&freeBlock);allocator=reinterpret_cast<uintptr_t>(allocMethods.data());allocatorPointer=reinterpret_cast<uintptr_t>(&allocator);data(0xa7e484,reinterpret_cast<uintptr_t>(&allocatorPointer));data(0xa7e460,reinterpret_cast<uintptr_t>(&enter));data(0xa7e48c,reinterpret_cast<uintptr_t>(&leave));data(0xf49be4,reinterpret_cast<uintptr_t>(&type5));
  systemMethods[77]=reinterpret_cast<uintptr_t>(&findMaterial);system=reinterpret_cast<uintptr_t>(systemMethods.data());data(0xf4c778,reinterpret_cast<uintptr_t>(&system));data(0xf4c774,reinterpret_cast<uintptr_t>(&system));
  matMethods[12]=reinterpret_cast<uintptr_t>(&add);matMethods[13]=reinterpret_cast<uintptr_t>(&drop);for(unsigned i=0;i<2;++i)mats[i]={reinterpret_cast<uintptr_t>(matMethods.data()),i+1};image.seal();
 }
 void data(size_t offset,uintptr_t value){field(image.base,offset,value);}
 void reset(){need(allocations.empty(),"previous case cleanup");strings.clear();queue.fill(0);queueStorage.fill(0);field(queue.data(),0x64,reinterpret_cast<uintptr_t>(queueStorage.data()));field(queue.data(),0x68,queueStorage.size());
  for(auto&o:objects)o.fill(0);for(auto&a:attributes)a.fill(0);
  for(size_t i=0;i+1<objects.size();++i)field(objects[i].data(),0,reinterpret_cast<uintptr_t>(objects[i+1].data()));
  for(size_t i=0;i+1<attributes.size();++i)field(attributes[i].data(),0,reinterpret_cast<uintptr_t>(attributes[i+1].data()));
  std::memset(image.base+0xf62cbc,0,0x28);data(0xf62cbc,0x68);data(0xf62cc8,reinterpret_cast<uintptr_t>(objects[0].data()));
  std::memset(image.base+0xf49cac,0,0x28);data(0xf49cac,24);data(0xf49cb8,reinterpret_cast<uintptr_t>(attributes[0].data()));
  field(factory.data(),0x18,reinterpret_cast<uintptr_t>("DmeMaterial"));currentPhase=0;std::array<uint32_t,4>id{};
  using Factory=void*(__thiscall*)(void*,uint32_t,uint32_t,const char*,uint32_t,const void*);
  for(unsigned i=0;i<2;++i){element[i]=static_cast<uint8_t*>(reinterpret_cast<Factory>(0x105c13b0)(factory.data(),i+1,0,"object",UINT32_MAX,id.data()));need(element[i]==objects[i].data(),"original factory pool identity");need(word(element[i],0x5c)==0&&word(element[i],0x60)==UINT32_MAX&&word(element[i],0x64)==0,"factory defaults before OnConstruction");reinterpret_cast<void(__thiscall*)(void*)>(0x105c1380)(element[i]);auto a=word(element[i],0x64);need(a&&word(reinterpret_cast<void*>(a),4)==reinterpret_cast<uintptr_t>(element[i]+0x60),"mtlName actual member backing");}
  extra.fill(nullptr);image.verify();
 }
 void createExtra(){extra[0]=reinterpret_cast<uint8_t*(__thiscall*)(void*,const char*,uint32_t)>(0x103614f0)(element[0],"$probe",5);need(extra[0]!=nullptr,"unbound attribute creation");const auto s=uint32_t(reinterpret_cast<uintptr_t>(intern("matB")));reinterpret_cast<void(__thiscall*)(void*,const uint32_t*)>(0x10366100)(extra[0],&s);}
 void set(unsigned id,unsigned name,int p,uint16_t flags){currentPhase=p;auto a=reinterpret_cast<void*>(uintptr_t(word(element[id],0x64)));field(a,0x10,flags);reinterpret_cast<void(__thiscall*)(void*,const char*)>(0x105c11c0)(element[id],names[name]);image.verify();}
 void copy(){currentPhase=0;reinterpret_cast<void(__thiscall*)(void*,void*,uint32_t,uint8_t)>(0x103621f0)(element[0],element[1],2,0);extra[1]=reinterpret_cast<uint8_t*>(uintptr_t(word(element[1],0x30)));need(std::strcmp(reinterpret_cast<const char*>(uintptr_t(word(extra[1],0x14))),"$probe")==0,"new attributes prepended");need(extra[0]!=extra[1]&&word(extra[0],4)!=word(extra[1],4),"unbound copied value has independent storage");need(word(reinterpret_cast<void*>(uintptr_t(word(element[1],0x64))),4)==reinterpret_cast<uintptr_t>(element[1]+0x60),"copy preserves destination member backing");image.verify();}
 void clearDirty(unsigned id){auto a=reinterpret_cast<void*>(uintptr_t(word(element[id],0x64)));field(a,0x10,word(a,0x10)&~0x80u);}
 void resolve(unsigned id,bool available){data(0xf4c774,available?reinterpret_cast<uintptr_t>(&system):0);reinterpret_cast<void(__thiscall*)(void*)>(0x105c11f0)(element[id]);image.verify();}
 uint32_t acquire(unsigned id){auto p=reinterpret_cast<Mat*(__thiscall*)(void*)>(0x105c1220)(element[id]);image.verify();return p?p->id:0;}
 void snapshot(std::ostream&o){for(unsigned i=0;i<2;++i){const auto p=reinterpret_cast<const char*(__thiscall*)(void*)>(0x105c1210)(element[i]);put(o,nameId(p));auto r=reinterpret_cast<Mat*>(uintptr_t(word(element[i],0x5c)));put(o,r?r->id:0);put(o,word(reinterpret_cast<void*>(uintptr_t(word(element[i],0x64))),0x10)&0xffff);if(extra[i]){put(o,word(extra[i],0x10)&0xffff);put(o,nameId(reinterpret_cast<const char*>(uintptr_t(word(reinterpret_cast<void*>(uintptr_t(word(extra[i],4))))))));}else{put(o,0);put(o,0);}}}
 void finish(){image.verify();need(word(image.base+0xf62cbc,0x10)==2&&word(image.base+0xf49cac,0x10)==6,"actual native pool object/attribute counts");
  // Explicit fixture cleanup: unbound string values use the real type5 free
  // handler; pool object retirement and DataModel GC are NOT claimed here.
  for(auto a:extra)if(a){auto p=reinterpret_cast<void*>(uintptr_t(word(a,4)));reinterpret_cast<void(__thiscall*)(void*,void*)>(0x103643d0)(&type5,p);}
  need(allocations.empty(),"copy-context allocations and explicit string cleanup balanced");}
 ~Backend(){if(image.base)VirtualFree(image.base,0,MEM_RELEASE);if(heap)HeapDestroy(heap);}
};
#else
struct Services:MaterialElementBindingServicesCandidate{
 std::optional<uintptr_t>findMaterial(std::string_view s)override{return lookup(std::string(s).c_str());}
 void addReference(uintptr_t p)override{retain(uint32_t(p));}
 void releaseReference(uintptr_t p)override{releaseRef(uint32_t(p));}
};
struct Backend{
 std::array<MaterialElementBindingCandidate,2>element{};std::array<bool,2>extra{};Services services;
 explicit Backend(const char*path){need(!std::ifstream(path).good(),"owned engine image must be absent");}
 void reset(){element={};extra.fill(false);}
 void createExtra(){extra[0]=true;}
 void set(unsigned id,unsigned name,int phase,uint16_t flags){element[id].nameFlags=flags;need(setMaterialElementNameCandidate(element[id],names[name],phase),"owned name write");}
 void copy(){need(setMaterialElementNameCandidate(element[1],*element[0].binding.name,0),"owned member name copy");extra[1]=extra[0];}
 void clearDirty(unsigned id){element[id].nameFlags=uint16_t(element[id].nameFlags&~0x80u);}
 void resolve(unsigned id,bool available){resolveMaterialElementBindingCandidate(element[id],available,services);}
 uint32_t acquire(unsigned id){const auto r=acquireMaterialSessionResourceCandidate(element[id].binding,services);need(r.has_value(),"known resource lookup");return uint32_t(*r);}
 void snapshot(std::ostream&o){for(unsigned i=0;i<2;++i){put(o,nameId(element[i].binding.name->c_str()));put(o,uint32_t(element[i].binding.resource));put(o,element[i].nameFlags);put(o,extra[i]?0x4085:0);put(o,extra[i]?2:0);}}
 void finish(){}
};
static int safety(){
 Services s;MaterialElementBindingCandidate value;events.clear();refs.fill(0);lookupMode=0;
 need(value.invalidName&&value.nameFlags==0x205&&!value.binding.resource,"constructor defaults");
 need(setMaterialElementNameCandidate(value,"matA",0)&&!value.invalidName&&value.nameFlags==0x4285,"name write");
 value.nameFlags=0x205;need(setMaterialElementNameCandidate(value,"matB",3)&&*value.binding.name=="matA"&&value.nameFlags==0x205,"phase rejection");
 value.nameFlags=0x225;need(setMaterialElementNameCandidate(value,"matB",0)&&*value.binding.name=="matA"&&value.nameFlags==0x225,"readonly rejection");
 value.nameFlags=0x205;need(setMaterialElementNameCandidate(value,"matA",0)&&value.nameFlags==0x205,"same name short circuit");
 need(setMaterialElementNameCandidate(value,std::string("matA\0matB",9),0)&&value.nameFlags==0x205,"C string extent");
 need(!setMaterialElementNameCandidate(value,std::string(4096,'x'),0)&&*value.binding.name=="matA"&&value.nameFlags==0x205,"oversized request unchanged");
 need(acquireMaterialSessionResourceCandidate(value.binding,s)==uintptr_t(1)&&refs[1]==1,"acquire retained");
 value.nameFlags|=0x80;const auto before=events.size();resolveMaterialElementBindingCandidate(value,false,s);need(value.binding.resource==1&&events.size()==before,"unavailable system leaves cache intact");
 resolveMaterialElementBindingCandidate(value,true,s);need(!value.binding.resource&&!refs[1]&&(value.nameFlags&0x80),"release retains dirty flag");
 resolveMaterialElementBindingCandidate(value,true,s);need(events.size()==before+1,"null cache no duplicate release");
 std::cout<<"PASS element_binding_safety_checks=11\n";return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_ELEMENT_BINDING
 if(argc==2&&!std::strcmp(argv[1],"--self-test"))return safety();
#endif
 need(argc==4,"private-or-absent-image query fresh-output");need(!std::ifstream(argv[3]).good(),"fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
#ifdef SFM64_NATIVE_ELEMENT_BINDING
 static_assert(sizeof(void*)==4,"private x86 only");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&fault);
#else
 static_assert(sizeof(void*)==8,"owned x64 only");
#endif
 Backend b(argv[1]);const auto count=get(in);need(count&&count<100000,"case count");put(out,count);
 for(unsigned ci=0;ci<count;++ci){const auto initial=get(in),replacement=get(in),p=get(in),flags=get(in),available=get(in),clear=get(in);lookupMode=get(in);need(initial<4&&replacement<4&&p<7&&available<2&&clear<2&&lookupMode<2&&(flags&31)==5&&flags<65536,"case shape");events.clear();refs.fill(0);b.reset();b.createExtra();b.set(0,initial,0,0x205);b.clearDirty(0);b.acquire(0);b.copy();b.acquire(1);b.snapshot(out);b.set(0,replacement,int(p),uint16_t(flags));b.snapshot(out);b.resolve(0,available!=0);b.snapshot(out);b.acquire(0);b.snapshot(out);if(clear)b.clearDirty(0);b.resolve(0,true);b.acquire(0);b.snapshot(out);for(auto r:refs)put(out,r);put(out,uint32_t(events.size()));for(auto e:events){put(out,e[0]);put(out,e[1]);}b.finish();}
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output complete");
#ifdef SFM64_NATIVE_ELEMENT_BINDING
 need(enters==leaves&&allocations.empty(),"native profile and heap balance");std::cout<<"profile_pairs="<<enters<<'\n';
#endif
 std::cout<<"PASS element_binding_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
