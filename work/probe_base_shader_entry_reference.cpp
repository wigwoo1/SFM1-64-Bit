// Actual stdshader slot6 + shadow setup vs existing owned helpers.
// Concrete slot14 remains a controlled observer. No original code is embedded.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "studio_shader_shadow_setup_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,shadow,flags,mutation,snapshot,compression;};
struct Object{uintptr_t table;uint32_t id;};
struct Parameter{uint32_t pad[2],flags;};
struct Event{std::array<uint32_t,17>words;};
static Query q;static std::array<Object,2>shaders,shadows,apis,contexts,instances;
static std::array<Parameter,2>parameters;static std::array<Parameter*,2>lists;
static StudioBaseShaderDrawStateCandidate shared;
static std::vector<Event>events;static uint32_t depth=0,draws=0,checks=0;
static constexpr std::array<size_t,6>offsets{0xdb944,0xdb940,0xdb948,0xdb94c,0xdb958,0xdb95c};
#ifndef _WIN64
static uint8_t*imageBase;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
static uintptr_t field(size_t i){
#ifdef _WIN64
 switch(i){case 0:return shared.parameters;case 1:return shared.shadow;case 2:return shared.api;case 3:return shared.snapshot;case 4:return shared.pass;case 5:return shared.instanceData;default:throw std::runtime_error("field index");}
#else
 uint32_t v;std::memcpy(&v,imageBase+offsets.at(i),4);return v;
#endif
}
static void field(size_t i,uintptr_t v){
#ifdef _WIN64
 switch(i){case 0:shared.parameters=v;break;case 1:shared.shadow=v;break;case 2:shared.api=v;break;case 3:shared.snapshot=uint32_t(v);break;case 4:shared.pass=uint32_t(v);break;case 5:shared.instanceData=v;break;default:throw std::runtime_error("field index");}
#else
 uint32_t w=uint32_t(v);std::memcpy(imageBase+offsets.at(i),&w,4);
#endif
}
static uint32_t identity(uintptr_t p,const std::array<Object,2>&objects){if(!p)return 0;for(const auto&o:objects)if(reinterpret_cast<uintptr_t>(&o)==p)return o.id;throw std::runtime_error("object identity");}
static uint32_t listId(uintptr_t p){if(!p)return 0;for(uint32_t i=0;i<2;++i)if(reinterpret_cast<uintptr_t>(&lists[i])==p)return i+1;throw std::runtime_error("list identity");}
static uintptr_t address(const std::array<Object,2>&objects,uint32_t id){return id?reinterpret_cast<uintptr_t>(&objects.at(id-1)):0;}
static uintptr_t list(uint32_t id){return reinterpret_cast<uintptr_t>(&lists.at(id-1));}
static void record(uint32_t kind,uint32_t receiver=0,uint32_t slot=0,uint32_t a=0,uint32_t b=0,uint32_t suppliedParams=0,uint32_t suppliedShadow=0,uint32_t suppliedApi=0,uint32_t compression=0,uint32_t context=0){
 events.push_back({{kind,receiver,slot,a,b,depth,listId(field(0)),identity(field(1),shadows),identity(field(2),apis),uint32_t(field(3)),uint32_t(field(4)),identity(field(5),instances),suppliedParams,suppliedShadow,suppliedApi,compression,context}});
}
static void reset(uintptr_t receiver){need(identity(receiver,shadows)==1&&depth==0,"outer initial shadow reset");record(0,1);
 if(q.mutation&1){field(0,list(2));field(1,address(shadows,2));field(2,address(apis,2));field(3,uint32_t(field(3))^0x11223344);field(4,17);field(5,address(instances,2));}
 if(q.mutation&4)for(auto&p:parameters)p.flags^=0xffffffffu;
}
static void shadowCall(uintptr_t receiver,uint32_t slot,uint32_t a,uint32_t b){record(1,identity(receiver,shadows),slot,a,b);
 if(q.mutation&2){field(1,address(shadows,identity(receiver,shadows)==1?2:1));for(auto&p:parameters)p.flags^=0xffffffffu;field(3,uint32_t(field(3))+5);field(4,uint32_t(field(4))+3);}
}
static void invoke(uint32_t id,uintptr_t params,uint32_t snapshot,uintptr_t shadow,uintptr_t api,uint32_t compression,uintptr_t context,uintptr_t data);
static void concreteDraw(uintptr_t receiver,uintptr_t params,uintptr_t shadow,uintptr_t api,uint32_t compression,uintptr_t context){
 const auto id=depth?2u:1u;
 need(identity(receiver,shaders)==id&&listId(params)==id&&identity(shadow,shadows)==(depth?0u:q.shadow)&&identity(api,apis)==id&&compression==(depth?(q.compression^0x12345678u):q.compression)&&identity(context,contexts)==id,"original supplied arguments retained across setup callbacks");
 record(2,id,14,0,0,listId(params),identity(shadow,shadows),identity(api,apis),compression,identity(context,contexts));++draws;++checks;
 if(!depth&&(q.mutation&8)){
  depth=1;invoke(2,list(2),0xfedcba98u,0,address(apis,2),q.compression^0x12345678u,address(contexts,2),address(instances,2));depth=0;
  for(size_t i=0;i<6;++i)need(field(i)==0,"nested call clears shared globals instead of restoring outer state");
  record(3,1);++checks;
 }
 if(q.mutation&16){field(0,list(2));field(1,address(shadows,2));field(2,address(apis,2));field(3,0xffffffff);field(4,0x80000000);field(5,address(instances,2));}
}
#ifdef _WIN64
struct Ops{
 void reset(uintptr_t p){::reset(p);}uint32_t primaryFlags(uintptr_t p){return parameters.at(listId(p)-1).flags;}
 void one(uintptr_t p,uint32_t slot,uint32_t a){shadowCall(p,slot,a,0xffffffff);}
 void two(uintptr_t p,uint32_t slot,uint32_t a,uint32_t b){shadowCall(p,slot,a,b);}
 void shadowSetup(uintptr_t,StudioBaseShaderDrawStateCandidate&s){need(&s==&shared,"shared shadow state");studioShaderShadowSetupCandidate(s,*this);}
 void draw(uintptr_t s,uintptr_t p,uintptr_t sh,uintptr_t a,uint32_t c,uintptr_t ctx,StudioBaseShaderDrawStateCandidate&st){need(&st==&shared,"shared draw state");concreteDraw(s,p,sh,a,c,ctx);}
};
#else
static void __fastcall resetNative(void*p,void*){reset(reinterpret_cast<uintptr_t>(p));}
template<uint32_t Slot>static void __fastcall oneNative(void*p,void*,uint32_t a){shadowCall(reinterpret_cast<uintptr_t>(p),Slot,a,0xffffffff);}
static void __fastcall twoNative(void*p,void*,uint32_t a,uint32_t b){shadowCall(reinterpret_cast<uintptr_t>(p),14,a,b);}
static void __fastcall drawNative(void*p,void*,void*params,void*shadow,void*api,uint32_t compression,void*context){concreteDraw(reinterpret_cast<uintptr_t>(p),reinterpret_cast<uintptr_t>(params),reinterpret_cast<uintptr_t>(shadow),reinterpret_cast<uintptr_t>(api),compression,reinterpret_cast<uintptr_t>(context));}
#endif
static void invoke(uint32_t id,uintptr_t params,uint32_t snapshot,uintptr_t shadow,uintptr_t api,uint32_t compression,uintptr_t context,uintptr_t data){
#ifdef _WIN64
 Ops ops;studioBaseShaderDrawCandidate(shared,address(shaders,id),params,snapshot,shadow,api,compression,context,data,ops);
#else
 auto*receiver=reinterpret_cast<void*>(address(shaders,id));auto*table=*reinterpret_cast<uintptr_t**>(receiver);using Call=void(__thiscall*)(void*,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t);reinterpret_cast<Call>(table[6])(receiver,params,snapshot,shadow,api,compression,context,data);
#endif
}
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==3&&!std::ifstream(argv[2]).good(),"query fresh owned trace");const char*in=argv[1],*out=argv[2];
#else
 need(argc==4&&!std::ifstream(argv[3]).good(),"private shader image query fresh trace");const char*in=argv[2],*out=argv[3];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0xe0000);imageBase=image.base;
 std::array<uintptr_t,25>shadowMethods{};shadowMethods[0]=reinterpret_cast<uintptr_t>(&resetNative);shadowMethods[1]=reinterpret_cast<uintptr_t>(&oneNative<1>);shadowMethods[2]=reinterpret_cast<uintptr_t>(&oneNative<2>);shadowMethods[3]=reinterpret_cast<uintptr_t>(&oneNative<3>);shadowMethods[4]=reinterpret_cast<uintptr_t>(&oneNative<4>);shadowMethods[15]=reinterpret_cast<uintptr_t>(&oneNative<15>);shadowMethods[24]=reinterpret_cast<uintptr_t>(&oneNative<24>);shadowMethods[14]=reinterpret_cast<uintptr_t>(&twoNative);
 for(auto&o:shadows)o.table=reinterpret_cast<uintptr_t>(shadowMethods.data());for(auto&o:shaders)o.table=0x1009c39c;
 uint32_t leaf=uint32_t(reinterpret_cast<uintptr_t>(&drawNative));std::memcpy(image.base+0x9c39c+14*4,&leaf,4);image.seal();
#endif
 for(uint32_t i=0;i<2;++i){shaders[i].id=shadows[i].id=apis[i].id=contexts[i].id=instances[i].id=i+1;lists[i]=&parameters[i];}
 std::ifstream input(in,std::ios::binary);std::ofstream output(out,std::ios::binary);need(bool(input)&&bool(output),"query trace IO");uint32_t cases=0,eventCount=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.shadow<2&&q.mutation<32,"query domain");events.clear();depth=draws=0;parameters[0].flags=q.flags;parameters[1].flags=q.flags^0x3001a400u;
  field(0,list(2));field(1,address(shadows,2));field(2,address(apis,2));field(3,0x12345678);field(4,99);field(5,address(instances,2));
  invoke(1,list(1),q.snapshot,address(shadows,q.shadow),address(apis,1),q.compression,address(contexts,1),address(instances,1));
  need(draws==1u+uint32_t(bool(q.mutation&8)),"exact outer/nested draws");for(size_t i=0;i<6;++i)need(field(i)==0,"successful-return globals clear");++checks;
  const uint32_t header[11]{cases,draws,parameters[0].flags,parameters[1].flags,uint32_t(events.size()),uint32_t(field(0)),uint32_t(field(1)),uint32_t(field(2)),uint32_t(field(3)),uint32_t(field(4)),uint32_t(field(5))};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(e.words.data()),sizeof(e.words));eventCount+=uint32_t(events.size());++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS base_shader_entry_cases="<<cases<<" events="<<eventCount<<" independent_checks="<<checks<<" concrete_shader14=OBSERVER";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
