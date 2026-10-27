// Whole original builder + actual element copy/file transition, with explicit
// factory-preseeded scalar attributes and typed value-copy boundary services.
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
#include <algorithm>
#ifdef SFM64_NATIVE_BACKUP_CONSTRUCTION
#include "native_private_world_image_fixture.h"
#else
#include "material_session_backup_construction_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&in){uint32_t v=0;need(bool(in.read(reinterpret_cast<char*>(&v),4)),"Query word");return v;}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
struct Input{uint32_t available=1,base=1,material=1,factory=0,file=UINT32_MAX,type=2,flags=0,hasAttribute=0;std::array<uint32_t,16>words{};};
struct Clone{uint32_t source=0,handle=0,base=1,material=1,file=0,type=2,flags=0,hasAttribute=0;std::array<uint32_t,16>words{};};
struct Event{uint32_t kind=0,a=0,b=0,c=0;};
static std::vector<Input>inputs;static std::vector<Clone>clones;static std::vector<Event>events;
static void event(uint32_t k,uint32_t a,uint32_t b=0,uint32_t c=0){events.push_back({k,a,b,c});}
static unsigned dataBytes(uint32_t type){switch(type){case 2:case 3:case 8:return 4;case 4:return 1;case 9:return 8;case 10:return 12;case 11:return 16;case 14:return 64;default:throw std::runtime_error("Only explicit leaf-value copy services");}}
static Clone cloneMetadata(uint32_t source){const auto&i=inputs.at(source-1);Clone c;c.source=source;c.handle=uint32_t(1000+clones.size());c.base=i.factory!=3;c.material=i.material&&i.factory!=4;c.file=i.file;c.type=i.type;c.flags=i.type|i.flags;c.hasAttribute=i.hasAttribute;return c;}
#ifdef SFM64_NATIVE_BACKUP_CONSTRUCTION
struct NativeRecord{std::array<uint8_t,0x64>block{};std::array<uint8_t,24>attribute{};std::array<uint32_t,16>data{};std::string name;};
static std::vector<NativeRecord>records;static std::array<NativeRecord,256>copies;
static std::map<std::string,std::string>strings;
static const char*intern(const char*s){need(s!=nullptr,"Explicit nonnull intern service");return strings.emplace(s,s).first->second.c_str();}
template<size_t N>static void field(std::array<uint8_t,N>&b,size_t at,uintptr_t value){need(at+4<=N,"Native field extent");auto w=uint32_t(value);std::memcpy(b.data()+at,&w,4);}
static std::array<uintptr_t,5>elementMethods{};
static void initialize(NativeRecord&r,uint32_t handle,uint32_t typeSymbol,const char*name,uint32_t file){r.block.fill(0);r.attribute.fill(0);r.data.fill(0);r.name=name;field(r.block,0,reinterpret_cast<uintptr_t>(elementMethods.data()));field(r.block,0x14,reinterpret_cast<uintptr_t>(intern(name)));field(r.block,0x1c,handle);field(r.block,0x34,typeSymbol);field(r.block,0x38,file);}
static std::pair<bool,uint32_t>idOf(void*p){for(size_t i=0;i<records.size();++i)if(records[i].block.data()==p)return {false,uint32_t(i+1)};for(size_t i=0;i<clones.size();++i)if(copies[i].block.data()==p)return {true,uint32_t(i)};throw std::runtime_error("Element pointer identity");}
static bool __fastcall isType(void*p,void*,uint32_t symbol){auto[copy,id]=idOf(p);if(symbol==0x11111111){const auto result=copy?clones[id].base:inputs[id-1].base;if(!copy)event(1,id,result);return result!=0;}need(symbol==0x22222222&&copy,"Material cast boundary");event(3,clones[id].handle,clones[id].material);return clones[id].material!=0;}
static std::array<uint8_t,0x64>model{};static uint32_t modelPresent=1;
static bool factoryResolvePending=false;
static void*__fastcall resolve(void*,void*,uint32_t handle){if(factoryResolvePending){factoryResolvePending=false;if(handle==UINT32_MAX)return nullptr;need(handle>=1000&&handle<1000+clones.size(),"Factory resolve identity");const auto id=handle-1000;return inputs[clones[id].source-1].factory==2?nullptr:copies[id].block.data();}if(handle==900){event(0,900,modelPresent?900:0);return modelPresent?model.data():nullptr;}const auto id=handle&&handle<=inputs.size()&&inputs[handle-1].available?handle:0;event(0,handle,id);return id?records[id-1].block.data():nullptr;}
static uintptr_t dmPointer=0;static uint32_t valueCopies=0,profileEnters=0,profileLeaves=0;
static uint32_t*__fastcall internService(void*,void*,uint32_t*out,const char*s){*out=uint32_t(reinterpret_cast<uintptr_t>(intern(s)));return out;}
static uint32_t __fastcall create(void*,void*,uint32_t type,const char*name,uint32_t file,void*id){need(!id,"Copy factory default id");uint32_t source=0;for(size_t i=0;i<records.size();++i)if(records[i].name==name){source=uint32_t(i+1);break;}need(source!=0&&type==(inputs[source-1].material?0x22222222u:0x11111111u)&&file==inputs[source-1].file,"Actual create type/name/file contract");const auto mode=inputs[source-1].factory;factoryResolvePending=true;if(mode==1){event(2,source,UINT32_MAX,mode);return UINT32_MAX;}auto c=cloneMetadata(source);need(clones.size()<copies.size(),"Clone bounds");const auto index=clones.size();clones.push_back(c);auto&r=copies[index];initialize(r,c.handle,type,name,file);
 if(c.hasAttribute){field(r.block,0x30,reinterpret_cast<uintptr_t>(r.attribute.data()));field(r.attribute,4,reinterpret_cast<uintptr_t>(r.data.data()));field(r.attribute,8,reinterpret_cast<uintptr_t>(r.block.data()));field(r.attribute,0x10,c.flags);field(r.attribute,0x14,reinterpret_cast<uintptr_t>(intern("$value")));}
 event(2,source,c.handle,mode);return c.handle;}
static bool __fastcall getUndo(void*,void*){return false;}
static void __fastcall setUndo(void*,void*,bool value){need(!value,"Explicit disabled undo restore");}
static void __fastcall copyValue(void*,void*,uint8_t*target,uint32_t sourceType,const void*data){uint32_t flags=0;std::memcpy(&flags,target+0x10,4);need((flags&31)==sourceType&&!(flags&0x20),"Readonly temporarily cleared / exact copy type");uint32_t dest=0;std::memcpy(&dest,target+4,4);std::memcpy(reinterpret_cast<void*>(uintptr_t(dest)),data,dataBytes(sourceType));++valueCopies;}
static HANDLE privateHeap=nullptr;static std::map<void*,size_t>allocations;
static void*__fastcall allocate(void*,void*,size_t n){need(n&&n<=16*1024*1024,"Private allocation bounds");void*p=HeapAlloc(privateHeap,HEAP_ZERO_MEMORY,n);need(p!=nullptr,"Private allocation");allocations[p]=n;return p;}
static void*__fastcall reallocate(void*,void*,void*p,size_t n){need(n&&n<=16*1024*1024&&(!p||allocations.count(p)),"Private reallocation");if(!p)return allocate(nullptr,nullptr,n);void*q=HeapReAlloc(privateHeap,HEAP_ZERO_MEMORY,p,n);need(q!=nullptr,"Private realloc result");allocations.erase(p);allocations[q]=n;return q;}
static void __fastcall release(void*,void*,void*p){if(!p)return;need(allocations.erase(p)==1&&HeapFree(privateHeap,0,p)!=0,"Private free provenance");}
static void __fastcall enterProfile(void*,void*,const char*,int,const char*,int,int){++profileEnters;}
static void __fastcall leaveProfile(void*,void*){++profileLeaves;}
static LONG CALLBACK exceptionEvidence(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION){std::cerr<<"Private reference access violation at "<<p->ExceptionRecord->ExceptionAddress<<" target "<<reinterpret_cast<void*>(p->ExceptionRecord->ExceptionInformation[1])<<'\n';}return EXCEPTION_CONTINUE_SEARCH;}
#else
struct Services:MaterialSessionBackupConstructionServicesCandidate{
 uintptr_t resolve(uint32_t h)override{const auto id=h&&h<=inputs.size()&&inputs[h-1].available?h:0;event(0,h,id);return id;}
 bool isElement(uintptr_t p)override{event(1,uint32_t(p),inputs[p-1].base);return inputs[p-1].base!=0;}
 std::optional<uintptr_t>copyElement(uintptr_t p,uint32_t mode)override{need(mode==2,"Actual copy mode2");const auto&i=inputs[p-1];if(i.factory==1){event(2,uint32_t(p),UINT32_MAX,i.factory);return uintptr_t(0);}auto c=cloneMetadata(uint32_t(p));clones.push_back(c);event(2,uint32_t(p),c.handle,i.factory);if(i.factory==2||i.factory==3)return uintptr_t(0);if(c.hasAttribute)std::memcpy(clones.back().words.data(),i.words.data(),dataBytes(i.type));return uintptr_t(clones.size());}
 bool isMaterial(uintptr_t p)override{auto&c=clones[p-1];event(3,c.handle,c.material);return c.material!=0;}
 bool setFileId(uintptr_t p,uint32_t file,uint32_t mode,uint8_t same)override{need(file==UINT32_MAX&&mode==2&&!same,"Native file transition arguments");clones[p-1].file=file;return true;}
 uint32_t handle(uintptr_t p)override{return clones[p-1].handle;}
};
#endif
int main(int argc,char**argv){try{
 need(argc==4,"private-or-absent-image query fresh-output");need(!std::ifstream(argv[3]).good(),"Fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Probe IO");
#ifdef SFM64_NATIVE_BACKUP_CONSTRUCTION
 static_assert(sizeof(void*)==4,"Separate private reference x86");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&exceptionEvidence);NativePrivateWorldImageFixture image(argv[1]);privateHeap=HeapCreate(0,0,0);need(privateHeap!=nullptr,"Private heap");
 std::array<uintptr_t,62>dmMethods{};dmMethods[16]=reinterpret_cast<uintptr_t>(&create);dmMethods[18]=reinterpret_cast<uintptr_t>(&resolve);dmMethods[50]=reinterpret_cast<uintptr_t>(&internService);dmMethods[60]=reinterpret_cast<uintptr_t>(&setUndo);dmMethods[61]=reinterpret_cast<uintptr_t>(&getUndo);std::array<uint8_t,0xc1b0>dm{};field(dm,0,reinterpret_cast<uintptr_t>(dmMethods.data()));dmPointer=reinterpret_cast<uintptr_t>(dm.data());std::memcpy(image.base+0xe9a51c,&dmPointer,4);std::memcpy(image.base+0xe9a518,&dmPointer,4);
 const uint32_t baseSymbol=0x11111111,materialSymbol=0x22222222;std::memcpy(image.base+0xe9a684,&baseSymbol,4);std::memcpy(image.base+0xeb4ec8,&materialSymbol,4);elementMethods[4]=reinterpret_cast<uintptr_t>(&isType);
 std::array<uintptr_t,6>allocatorMethods{};allocatorMethods[1]=reinterpret_cast<uintptr_t>(&allocate);allocatorMethods[3]=reinterpret_cast<uintptr_t>(&reallocate);allocatorMethods[5]=reinterpret_cast<uintptr_t>(&release);uintptr_t allocator=reinterpret_cast<uintptr_t>(allocatorMethods.data()),allocatorPointer=reinterpret_cast<uintptr_t>(&allocator);const auto allocatorAddress=reinterpret_cast<uintptr_t>(&allocatorPointer);std::memcpy(image.base+0xa7e484,&allocatorAddress,4);
 const auto entry=reinterpret_cast<uintptr_t>(&enterProfile),leave=reinterpret_cast<uintptr_t>(&leaveProfile);std::memcpy(image.base+0xa7e460,&entry,4);std::memcpy(image.base+0xa7e48c,&leave,4);
 std::array<uintptr_t,9>typeMethods{};typeMethods[8]=reinterpret_cast<uintptr_t>(&copyValue);uintptr_t typeObject=reinterpret_cast<uintptr_t>(typeMethods.data());const auto typePointer=reinterpret_cast<uintptr_t>(&typeObject);for(unsigned t:{2u,3u,4u,8u,9u,10u,11u,14u})std::memcpy(image.base+0xf49bd0+4*t,&typePointer,4);image.seal();
#else
 static_assert(sizeof(void*)==8,"Owned builder x64");need(!std::ifstream(argv[1]).good(),"Owned must not read an engine image");
#endif
 const auto cases=read(in);need(cases&&cases<100000,"Case count");put(out,cases);unsigned total=0;
 for(uint32_t ci=0;ci<cases;++ci){const auto present=read(in),attributeType=read(in),count=read(in),n=read(in),repeats=read(in);need(present<2&&count<=64&&n<=64&&repeats&&repeats<=2,"Case shape");inputs.assign(n,{});events.clear();clones.clear();for(auto&i:inputs){i.available=read(in);i.base=read(in);i.material=read(in);i.factory=read(in);i.file=read(in);i.type=read(in);i.flags=read(in);i.hasAttribute=read(in);need(i.available<2&&i.base<2&&i.material<2&&i.factory<5&&i.hasAttribute<2&&i.flags<=0xffe0&&!(i.flags&31u),"Input shape");dataBytes(i.type);for(auto&w:i.words)w=read(in);}std::vector<uint32_t>source(count),result;for(auto&h:source)h=read(in);
#ifdef SFM64_NATIVE_BACKUP_CONSTRUCTION
 strings.clear();records.resize(n);for(size_t i=0;i<n;++i){auto&r=records[i];const auto&q=inputs[i];initialize(r,uint32_t(i+1),q.material?materialSymbol:baseSymbol,("e"+std::to_string(i+1)).c_str(),q.file);if(q.hasAttribute){r.data=q.words;field(r.block,0x30,reinterpret_cast<uintptr_t>(r.attribute.data()));field(r.attribute,4,reinterpret_cast<uintptr_t>(r.data.data()));field(r.attribute,8,reinterpret_cast<uintptr_t>(r.block.data()));field(r.attribute,0x10,q.type|q.flags);field(r.attribute,0x14,reinterpret_cast<uintptr_t>(intern("$value")));}}
 model.fill(0);modelPresent=present;std::array<uint8_t,24>modelAttribute{};std::array<uint8_t,20>array{};field(array,0,reinterpret_cast<uintptr_t>(source.data()));field(array,12,source.size());field(modelAttribute,4,reinterpret_cast<uintptr_t>(array.data()));field(modelAttribute,0x10,attributeType);field(modelAttribute,0x14,reinterpret_cast<uintptr_t>(intern("materials")));if(attributeType!=UINT32_MAX)field(model,0x30,reinterpret_cast<uintptr_t>(modelAttribute.data()));
 std::array<uint8_t,0xe4>puppet{};std::array<uint32_t,256>storage{};storage.fill(0xaabbccdd);field(puppet,0x2c,900);field(puppet,0x98,reinterpret_cast<uintptr_t>(storage.data()));field(puppet,0x9c,storage.size());field(puppet,0xa4,7);field(puppet,0xa8,reinterpret_cast<uintptr_t>(storage.data()));using Build=void(__thiscall*)(void*);
 for(unsigned pass=0;pass<repeats;++pass){reinterpret_cast<Build>(0x10240950)(puppet.data());uint32_t size=0;std::memcpy(&size,puppet.data()+0xa4,4);need(size<=source.size(),"Backup output count");result.assign(storage.begin(),storage.begin()+size);image.verify();}
 for(size_t i=0;i<clones.size();++i){auto&c=clones[i];auto&r=copies[i];std::memcpy(&c.file,r.block.data()+0x38,4);if(c.hasAttribute){c.words=r.data;std::memcpy(&c.flags,r.attribute.data()+0x10,4);}}
 need(!factoryResolvePending&&allocations.empty(),"Factory resolution complete; native temporary copy/owner allocations released");
#else
 for(unsigned pass=0;pass<repeats;++pass){event(0,900,present?900:0);Services service;need(constructMaterialSessionBackupsCandidate(present&&(attributeType&31)==15?&source:nullptr,result,service),"Owned construction");}
#endif
 put(out,ci);put(out,uint32_t(result.size()));for(auto h:result)put(out,h);put(out,uint32_t(clones.size()));for(auto&c:clones){put(out,c.source);put(out,c.handle);put(out,c.base);put(out,c.material);put(out,c.file);put(out,c.type);put(out,c.flags);put(out,c.hasAttribute);for(auto w:c.words)put(out,w);}put(out,uint32_t(events.size()));for(auto&e:events){put(out,e.kind);put(out,e.a);put(out,e.b);put(out,e.c);}total+=unsigned(events.size());
 }
 need(in.peek()==std::char_traits<char>::eof(),"Query tail");out.flush();need(bool(out),"Output complete");
#ifdef SFM64_NATIVE_BACKUP_CONSTRUCTION
 image.verify();need(profileEnters==profileLeaves&&allocations.empty(),"Scope/temp cleanup");need(VirtualFree(image.base,0,MEM_RELEASE)!=0&&HeapDestroy(privateHeap)!=0,"Private image/heap release");std::cout<<"actual_copy_value_services="<<valueCopies<<" scope_pairs="<<profileEnters<<'\n';
#endif
 std::cout<<"PASS backup_construction_cases="<<cases<<" events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
